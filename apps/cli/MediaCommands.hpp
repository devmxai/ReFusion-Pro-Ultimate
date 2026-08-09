#pragma once

#include <QStringList>

namespace refusion::cli {

// Executes a media commit command from Qt's native Unicode argument view.
// Returns 2 for an invalid media-command spelling/arity.
[[nodiscard]] int run_media_command(const QStringList& arguments);

}  // namespace refusion::cli
