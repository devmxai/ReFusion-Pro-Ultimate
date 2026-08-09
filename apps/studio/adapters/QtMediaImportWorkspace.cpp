#include "adapters/QtMediaImportWorkspace.hpp"

#include <QCryptographicHash>
#include <QByteArrayView>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using refusion::application::CompressedSourceReadResult;
using refusion::application::CompressedSourceReadState;
using refusion::application::ImmutableCompressedSourceLease;
using refusion::application::MediaAssetMaterializationReceipt;
using refusion::application::MediaCancellationToken;
using refusion::application::PreparedMediaAsset;

constexpr qsizetype kCopyChunkBytes = 1024 * 1024;

[[nodiscard]] QString sha256_text(const QByteArray& digest) {
  return QStringLiteral("sha256:") + QString::fromLatin1(digest.toHex());
}

[[nodiscard]] bool write_atomically(const QString& path,
                                    const QByteArray& bytes) {
  QSaveFile output(path);
  output.setDirectWriteFallback(false);
  return output.open(QIODevice::WriteOnly) &&
         output.write(bytes) == bytes.size() && output.commit();
}

[[nodiscard]] QString transaction_token(const std::string& transaction_id) {
  const auto digest = QCryptographicHash::hash(
      QByteArray::fromStdString(transaction_id), QCryptographicHash::Sha256);
  return QString::fromLatin1(digest.toHex().first(24));
}

[[nodiscard]] bool safe_relative_path(const QString& value) {
  if (value.isEmpty() || QDir::isAbsolutePath(value) || value.contains(u'\\')) {
    return false;
  }
  const auto clean = QDir::cleanPath(value);
  return clean == value && clean != QStringLiteral("..") &&
         !clean.startsWith(QStringLiteral("../"));
}

[[nodiscard]] bool matches_file(const QString& path,
                                const std::string& expected_digest,
                                const std::uint64_t expected_size) {
  QFileInfo info(path);
  if (!info.isFile() || info.size() < 0 ||
      static_cast<std::uint64_t>(info.size()) != expected_size) {
    return false;
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return false;
  QCryptographicHash hash(QCryptographicHash::Sha256);
  while (!file.atEnd()) {
    const auto bytes = file.read(kCopyChunkBytes);
    if (bytes.isEmpty() && file.error() != QFileDevice::NoError) return false;
    hash.addData(bytes);
  }
  return sha256_text(hash.result()).toStdString() == expected_digest;
}

[[nodiscard]] QJsonObject manifest(
    const MediaAssetMaterializationReceipt& receipt, const QString& state) {
  return QJsonObject{
      {QStringLiteral("schema"), QStringLiteral("refusion.import-journal.v1")},
      {QStringLiteral("state"), state},
      {QStringLiteral("asset_id"),
       QString::fromStdString(receipt.asset_id.value)},
      {QStringLiteral("content_digest"),
       QString::fromStdString(receipt.content_digest)},
      {QStringLiteral("byte_size"),
       QString::number(static_cast<qulonglong>(receipt.byte_size))},
      {QStringLiteral("project_relative_original"),
       QString::fromStdString(receipt.project_relative_original)},
  };
}

class QtCompressedFileSource final : public ImmutableCompressedSourceLease {
 public:
  QtCompressedFileSource(QString path, std::string digest,
                         const std::uint64_t byte_size)
      : path_(std::move(path)),
        digest_(std::move(digest)),
        byte_size_(byte_size) {}

  [[nodiscard]] std::string content_digest() const override { return digest_; }
  [[nodiscard]] std::uint64_t byte_size() const noexcept override {
    return byte_size_;
  }
  [[nodiscard]] CompressedSourceReadResult read_at(
      const std::uint64_t offset,
      const std::span<std::uint8_t> destination) noexcept override {
    if (offset > byte_size_ ||
        offset > static_cast<std::uint64_t>(
                     std::numeric_limits<qint64>::max())) {
      return {.state = CompressedSourceReadState::failed};
    }
    if (offset == byte_size_) {
      return {.state = CompressedSourceReadState::end_of_source};
    }
    QFile input(path_);
    if (!input.open(QIODevice::ReadOnly) ||
        !input.seek(static_cast<qint64>(offset))) {
      return {.state = CompressedSourceReadState::failed};
    }
    const auto maximum = std::min<std::uint64_t>(
        destination.size(), byte_size_ - offset);
    const auto count = input.read(
        reinterpret_cast<char*>(destination.data()),
        static_cast<qint64>(maximum));
    if (count <= 0) {
      return {.state = count == 0 ? CompressedSourceReadState::end_of_source
                                 : CompressedSourceReadState::failed};
    }
    return {.state = CompressedSourceReadState::read,
            .bytes_read = static_cast<std::size_t>(count)};
  }

 private:
  QString path_;
  std::string digest_;
  std::uint64_t byte_size_{0};
};

class QtPreparedMediaAsset final : public PreparedMediaAsset {
 public:
  QtPreparedMediaAsset(QString transaction_directory, QString staged_path,
                       QString final_path,
                       MediaAssetMaterializationReceipt receipt,
                       const bool final_already_present)
      : transaction_directory_(std::move(transaction_directory)),
        staged_path_(std::move(staged_path)),
        final_path_(std::move(final_path)),
        receipt_(std::move(receipt)),
        final_already_present_(final_already_present) {}

  ~QtPreparedMediaAsset() override {
    if (retained_) return;
    if (owns_final_) QFile::remove(final_path_);
    QDir(transaction_directory_).removeRecursively();
    const auto parent = QFileInfo(final_path_).absolutePath();
    QDir().rmdir(parent);
  }

  [[nodiscard]] const MediaAssetMaterializationReceipt& receipt()
      const noexcept override {
    return receipt_;
  }

  [[nodiscard]] bool commit() noexcept override {
    if (committed_) return true;
    if (final_already_present_) {
      committed_ = true;
    } else {
      const auto parent = QFileInfo(final_path_).absolutePath();
      if (!QDir().mkpath(parent) || !QFile::rename(staged_path_, final_path_)) {
        return false;
      }
      owns_final_ = true;
      committed_ = true;
    }
    const auto bytes = QJsonDocument(manifest(receipt_, QStringLiteral("asset_committed")))
                           .toJson(QJsonDocument::Compact);
    if (!write_atomically(transaction_directory_ + QStringLiteral("/manifest.json"),
                          bytes)) {
      if (owns_final_) {
        QFile::remove(final_path_);
        owns_final_ = false;
      }
      committed_ = false;
      return false;
    }
    return true;
  }

  void retain() noexcept override {
    if (!committed_) return;
    retained_ = true;
    QDir(transaction_directory_).removeRecursively();
  }

 private:
  QString transaction_directory_;
  QString staged_path_;
  QString final_path_;
  MediaAssetMaterializationReceipt receipt_;
  bool final_already_present_{false};
  bool owns_final_{false};
  bool committed_{false};
  bool retained_{false};
};

}  // namespace

QtCompressedFileSourceResult open_immutable_compressed_file_source(
    const QString& selected_file) noexcept {
  const QFileInfo info(selected_file);
  const auto canonical = info.canonicalFilePath();
  if (canonical.isEmpty() || !info.isFile() || info.size() <= 0) {
    return {.diagnostic = QStringLiteral(
                "RFX-MEDIA-IMPORT-SOURCE-OPEN: selected file is unavailable")};
  }
  QFile input(canonical);
  if (!input.open(QIODevice::ReadOnly)) {
    return {.diagnostic = QStringLiteral("RFX-MEDIA-IMPORT-SOURCE-OPEN: %1")
                              .arg(input.errorString())};
  }
  QCryptographicHash hash(QCryptographicHash::Sha256);
  while (!input.atEnd()) {
    const auto bytes = input.read(kCopyChunkBytes);
    if (bytes.isEmpty() && input.error() != QFileDevice::NoError) {
      return {.diagnostic = QStringLiteral("RFX-MEDIA-IMPORT-SOURCE-READ: %1")
                                .arg(input.errorString())};
    }
    hash.addData(bytes);
  }
  const auto byte_size = static_cast<std::uint64_t>(info.size());
  return {
      .source = std::make_shared<QtCompressedFileSource>(
          canonical, sha256_text(hash.result()).toStdString(), byte_size),
  };
}

QtMediaImportWorkspace::QtMediaImportWorkspace(QString project_directory)
    : project_directory_(QDir::cleanPath(std::move(project_directory))) {}

std::unique_ptr<PreparedMediaAsset> QtMediaImportWorkspace::prepare_copy(
    const std::string& transaction_id,
    const MediaAssetMaterializationReceipt& expected,
    ImmutableCompressedSourceLease& source,
    const MediaCancellationToken* cancellation) {
  const auto relative =
      QString::fromStdString(expected.project_relative_original);
  const auto required_prefix =
      QStringLiteral("Assets/Media/") +
      QString::fromStdString(expected.asset_id.value) + QLatin1Char('/');
  if (!safe_relative_path(relative) || !relative.startsWith(required_prefix) ||
      source.content_digest() != expected.content_digest ||
      source.byte_size() != expected.byte_size) {
    return nullptr;
  }

  const auto transaction_directory =
      project_directory_ + QStringLiteral("/.refusion/Journal/Import/") +
      transaction_token(transaction_id);
  QDir(transaction_directory).removeRecursively();
  if (!QDir().mkpath(transaction_directory)) return nullptr;
  const auto staged_path =
      transaction_directory + QStringLiteral("/original.partial");
  const auto final_path = project_directory_ + QLatin1Char('/') + relative;
  if (QFileInfo::exists(final_path)) {
    if (!matches_file(final_path, expected.content_digest, expected.byte_size)) {
      QDir(transaction_directory).removeRecursively();
      return nullptr;
    }
    const auto bytes =
        QJsonDocument(manifest(expected, QStringLiteral("prepared_existing")))
            .toJson(QJsonDocument::Compact);
    if (!write_atomically(transaction_directory + QStringLiteral("/manifest.json"),
                          bytes)) {
      QDir(transaction_directory).removeRecursively();
      return nullptr;
    }
    return std::make_unique<QtPreparedMediaAsset>(
        transaction_directory, staged_path, final_path, expected, true);
  }

  QFile staged(staged_path);
  if (!staged.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    QDir(transaction_directory).removeRecursively();
    return nullptr;
  }
  QCryptographicHash hash(QCryptographicHash::Sha256);
  // Import runs on a bounded worker thread whose platform stack can be much
  // smaller than the main thread. Keep the transfer chunk on the heap.
  std::vector<std::uint8_t> buffer(
      static_cast<std::size_t>(kCopyChunkBytes));
  std::uint64_t offset = 0;
  while (offset < expected.byte_size) {
    if (cancellation != nullptr && cancellation->cancelled()) {
      staged.close();
      QDir(transaction_directory).removeRecursively();
      return nullptr;
    }
    const auto maximum = std::min<std::uint64_t>(
        buffer.size(), expected.byte_size - offset);
    const auto read = source.read_at(
        offset, std::span<std::uint8_t>(buffer.data(),
                                       static_cast<std::size_t>(maximum)));
    if (read.state != CompressedSourceReadState::read ||
        read.bytes_read == 0 || read.bytes_read > maximum ||
        staged.write(reinterpret_cast<const char*>(buffer.data()),
                     static_cast<qint64>(read.bytes_read)) !=
            static_cast<qint64>(read.bytes_read)) {
      staged.close();
      QDir(transaction_directory).removeRecursively();
      return nullptr;
    }
    hash.addData(QByteArrayView(
        reinterpret_cast<const char*>(buffer.data()),
        static_cast<qsizetype>(read.bytes_read)));
    offset += read.bytes_read;
  }
  staged.close();
  if (sha256_text(hash.result()).toStdString() != expected.content_digest ||
      static_cast<std::uint64_t>(QFileInfo(staged_path).size()) !=
          expected.byte_size) {
    QDir(transaction_directory).removeRecursively();
    return nullptr;
  }
  const auto bytes =
      QJsonDocument(manifest(expected, QStringLiteral("copy_staged")))
          .toJson(QJsonDocument::Compact);
  if (!write_atomically(transaction_directory + QStringLiteral("/manifest.json"),
                        bytes)) {
    QDir(transaction_directory).removeRecursively();
    return nullptr;
  }
  return std::make_unique<QtPreparedMediaAsset>(
      transaction_directory, staged_path, final_path, expected, false);
}

bool recover_incomplete_media_imports(
    const QString& project_directory,
    const refusion::core::ProjectSnapshot& accepted_project,
    QString* diagnostic) noexcept {
  const auto clean_root = QDir::cleanPath(project_directory);
  QDir imports(clean_root + QStringLiteral("/.refusion/Journal/Import"));
  if (!imports.exists()) return true;
  const auto transactions = imports.entryList(
      QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
  for (const auto& transaction : transactions) {
    const auto transaction_directory = imports.filePath(transaction);
    QFile input(transaction_directory + QStringLiteral("/manifest.json"));
    if (input.open(QIODevice::ReadOnly)) {
      const auto document = QJsonDocument::fromJson(input.readAll());
      if (document.isObject()) {
        const auto object = document.object();
        const auto relative =
            object.value(QStringLiteral("project_relative_original")).toString();
        const auto digest =
            object.value(QStringLiteral("content_digest")).toString().toStdString();
        const auto byte_size_text =
            object.value(QStringLiteral("byte_size")).toString();
        bool converted = false;
        const auto byte_size = byte_size_text.toULongLong(&converted);
        if (converted && safe_relative_path(relative)) {
          const auto referenced = std::any_of(
              accepted_project.assets.begin(), accepted_project.assets.end(),
              [&](const refusion::core::AssetRecord& asset) {
                return asset.project_relative_original == relative.toStdString() &&
                       asset.content_digest == digest &&
                       asset.byte_size == byte_size;
              });
          if (!referenced) QFile::remove(clean_root + QLatin1Char('/') + relative);
        }
      }
    }
    if (!QDir(transaction_directory).removeRecursively()) {
      if (diagnostic != nullptr) {
        *diagnostic = QStringLiteral(
            "RFX-MEDIA-IMPORT-RECOVERY: cannot remove %1")
                          .arg(transaction_directory);
      }
      return false;
    }
  }
  return true;
}
