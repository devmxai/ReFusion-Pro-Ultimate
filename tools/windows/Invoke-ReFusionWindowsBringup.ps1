[CmdletBinding()]
param(
  [ValidateSet("Core", "Graphics", "Visual")]
  [string]$Lane = "Graphics",
  [switch]$FreshDependencies,
  [switch]$CompileOnly,
  [switch]$AllowDirtySource,
  [string]$PythonExe = "python",
  [string]$ReceiptPath = "out/evidence/windows-bringup.json"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repositoryRoot = (Resolve-Path (Join-Path $scriptRoot "../..")).Path
Set-Location $repositoryRoot

$stepReceipts = [System.Collections.Generic.List[object]]::new()
$initialDirty = @(& git status --porcelain=v1)
$sourceCommit = (& git rev-parse HEAD).Trim()
$evidenceDirectory = Join-Path $repositoryRoot "out/evidence"
$trackedWindowsLock = Join-Path $repositoryRoot "deps/locks/skia-transitive-windows-x64.lock.json"
$dependencyLockGenerated = $false
$windowsCapture = Join-Path $evidenceDirectory "xplat-visual-v1-windows-d3d12-640x360.ppm"
$windowsComparison = Join-Path $evidenceDirectory "xplat-visual-v1-metal-vs-d3d12.json"
$windowsQualification = Join-Path $evidenceDirectory "windows-d3d12-visual-qualification.json"
if ($LASTEXITCODE -ne 0) {
  throw "Unable to resolve the ReFusion source commit."
}
if ($initialDirty.Count -ne 0 -and -not $AllowDirtySource) {
  throw "Windows bring-up requires a clean source checkpoint. Use -AllowDirtySource only for an explicitly non-qualifying diagnostic run."
}

function Invoke-NativeCommand {
  param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string[]]$Arguments
  )
  & $Executable @Arguments
  if ($LASTEXITCODE -ne 0) {
    throw "Native command failed ($LASTEXITCODE): $Executable $($Arguments -join ' ')"
  }
}

function Invoke-ReceiptStep {
  param(
    [Parameter(Mandatory = $true)][string]$Name,
    [Parameter(Mandatory = $true)][scriptblock]$Action
  )
  $timer = [System.Diagnostics.Stopwatch]::StartNew()
  try {
    & $Action
    $timer.Stop()
    $stepReceipts.Add([ordered]@{
      name = $Name
      status = "passed"
      elapsed_ms = $timer.ElapsedMilliseconds
    })
  } catch {
    $timer.Stop()
    $stepReceipts.Add([ordered]@{
      name = $Name
      status = "failed"
      elapsed_ms = $timer.ElapsedMilliseconds
      diagnostic = $_.Exception.Message
    })
    throw
  }
}

function Enter-ReFusionMsvcEnvironment {
  $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
  if (-not (Test-Path $vswhere -PathType Leaf)) {
    throw "vswhere.exe was not found; install Visual Studio with the x64 C++ workload."
  }
  $installation = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  if (-not $installation) {
    throw "A supported MSVC x64 C++ toolchain was not found."
  }
  Import-Module "$installation\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
  Enter-VsDevShell -VsInstallPath $installation -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64"
}

$failure = $null
$gpuRows = @()
try {
  Invoke-ReceiptStep "msvc-environment" {
    Enter-ReFusionMsvcEnvironment
    Invoke-NativeCommand "cmake" @("--version")
    Invoke-NativeCommand $PythonExe @("--version")
  }

  Invoke-ReceiptStep "repository-policy" {
    Invoke-NativeCommand $PythonExe @("tools/bootstrap.py", "doctor")
    Invoke-NativeCommand $PythonExe @("tools/rfdev.py", "docs-doctor")
    Invoke-NativeCommand $PythonExe @("tools/rfdev.py", "architecture-check")
  }

  Invoke-ReceiptStep "windows-core" {
    Invoke-NativeCommand "cmake" @("--workflow", "--preset", "windows-core")
  }

  if ($Lane -ne "Core") {
    $fresh = @()
    if ($FreshDependencies) {
      $fresh = @("--fresh")
    }
    Invoke-ReceiptStep "official-fonts" {
      Invoke-NativeCommand $PythonExe (@("tools/bootstrap.py", "sync-font", "noto_sans_latin_baseline") + $fresh)
      Invoke-NativeCommand $PythonExe (@("tools/bootstrap.py", "sync-font", "noto_sans_arabic_baseline") + $fresh)
    }
    Invoke-ReceiptStep "official-skia-materialization" {
      Invoke-NativeCommand $PythonExe (@("tools/bootstrap.py", "sync", "depot_tools") + $fresh)
      Invoke-NativeCommand $PythonExe (@("tools/bootstrap.py", "sync", "skia") + $fresh)
      Invoke-NativeCommand $PythonExe @("tools/bootstrap.py", "hydrate-skia")
      if (-not (Test-Path $trackedWindowsLock -PathType Leaf)) {
        if (-not $CompileOnly) {
          throw "The reviewed Windows Skia transitive lock is absent. Run the CompileOnly lane once, review and commit deps/locks/skia-transitive-windows-x64.lock.json, then rerun the physical qualification from that clean commit."
        }
        Invoke-NativeCommand $PythonExe @("tools/bootstrap.py", "lock-skia-materialization")
        $script:dependencyLockGenerated = $true
      }
      Invoke-NativeCommand $PythonExe @("tools/bootstrap.py", "verify-skia-materialization")
    }
    Invoke-ReceiptStep "windows-skia-profile" {
      Invoke-NativeCommand $PythonExe @("tools/bootstrap.py", "build-skia", "--profile", "windows-x64-d3d12")
    }
    Invoke-ReceiptStep "windows-graphics" {
      if ($CompileOnly) {
        Invoke-NativeCommand "cmake" @("--preset", "windows-graphics")
        Invoke-NativeCommand "cmake" @("--build", "--preset", "windows-graphics")
      } else {
        Invoke-NativeCommand "cmake" @("--workflow", "--preset", "windows-graphics")
      }
    }
    if (-not $CompileOnly) {
      Invoke-ReceiptStep "windows-visual-capture" {
        New-Item -ItemType Directory -Force -Path $evidenceDirectory | Out-Null
        $env:REFUSION_XPLAT_CAPTURE_PPM = $windowsCapture
        try {
          Invoke-NativeCommand "ctest" @(
            "--preset", "windows-graphics",
            "-R", "^refusion.d3d12_fixture_renderer$",
            "--output-on-failure"
          )
        } finally {
          Remove-Item Env:REFUSION_XPLAT_CAPTURE_PPM -ErrorAction SilentlyContinue
        }
        Invoke-NativeCommand $PythonExe @(
          "tools/qualification/compare_visual_captures.py",
          "docs/evidence/reviews/artifacts/xplat-visual-v1-macos-metal-640x360.ppm",
          $windowsCapture,
          "--output", $windowsComparison
        )
      }
    }
  }

  if ($Lane -eq "Visual") {
    Invoke-ReceiptStep "windows-visual" {
      if ($CompileOnly) {
        Invoke-NativeCommand "cmake" @("--preset", "windows-visual")
        Invoke-NativeCommand "cmake" @("--build", "--preset", "windows-visual")
      } else {
        Invoke-NativeCommand "cmake" @("--workflow", "--preset", "windows-visual")
      }
    }
  }

  try {
    $gpuRows = @(Get-CimInstance Win32_VideoController | ForEach-Object {
      [ordered]@{
        name = $_.Name
        driver_version = $_.DriverVersion
        pnp_device_id = $_.PNPDeviceID
      }
    })
  } catch {
    $gpuRows = @([ordered]@{ diagnostic = $_.Exception.Message })
  }
  if (-not $CompileOnly -and $Lane -ne "Core") {
    Invoke-ReceiptStep "windows-qualification-receipt" {
      $primaryGpu = $gpuRows | Select-Object -First 1
      if ($null -eq $primaryGpu -or -not $primaryGpu.name) {
        throw "A named physical GPU is required for the qualification receipt."
      }
      $testInventory = (& ctest --preset windows-graphics --show-only=json-v1) | ConvertFrom-Json
      if ($LASTEXITCODE -ne 0) {
        throw "Unable to enumerate the passed Windows Graphics test suite."
      }
      $compiler = ((& cl 2>&1 | Select-Object -First 1) -join "").Trim()
      $cmakeVersion = ((& cmake --version | Select-Object -First 1) -join "").Trim()
      $ninjaVersion = ((& ninja --version | Select-Object -First 1) -join "").Trim()
      $skiaRevision = (& git -C out/deps-src/skia rev-parse HEAD).Trim()
      Invoke-NativeCommand $PythonExe @(
        "tools/qualification/write_visual_qualification_receipt.py",
        "--output", $windowsQualification,
        "--source-commit", $sourceCommit,
        "--profile", "windows-d3d12-desktop-v1",
        "--os", [System.Environment]::OSVersion.VersionString,
        "--architecture", [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString(),
        "--gpu", $primaryGpu.name,
        "--driver", $primaryGpu.driver_version,
        "--compiler", $compiler,
        "--cmake", $cmakeVersion,
        "--ninja", $ninjaVersion,
        "--skia-revision", $skiaRevision,
        "--font-layout-digest", "skia-harfbuzz-icu-freetype-text-layout-v2:$($skiaRevision):linebreak=icu:spacing=cluster:hinting=none",
        "--capture", $windowsCapture,
        "--suite", "windows-graphics",
        "--passed", $testInventory.tests.Count,
        "--failed", 0,
        "--not-run", "windows-performance-profile",
        "--not-run", "windows-media-foundation-video",
        "--physically-run",
        "--semantic-match",
        "--visual-tolerance"
      )
    }
  }
} catch {
  $failure = $_
} finally {
  $resolvedReceipt = Join-Path $repositoryRoot $ReceiptPath
  $receiptDirectory = Split-Path -Parent $resolvedReceipt
  New-Item -ItemType Directory -Force -Path $receiptDirectory | Out-Null
  $finalDirty = @(& git status --porcelain=v1)
  $receipt = [ordered]@{
    schema_version = 1
    policy_id = "PLAN-XPLAT-FIX-001/XPF-WP05"
    source_commit = $sourceCommit
    qualifying_source = ($initialDirty.Count -eq 0 -and -not $dependencyLockGenerated)
    lane = $Lane.ToLowerInvariant()
    execution_mode = if ($CompileOnly) { "compile-only" } else { "physical" }
    host = [ordered]@{
      computer_name = $env:COMPUTERNAME
      os = [System.Environment]::OSVersion.VersionString
      architecture = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()
      gpu = $gpuRows
    }
    dependencies = [ordered]@{
      skia_profile = if ($Lane -eq "Core") { $null } else { "windows-x64-d3d12" }
      windows_transitive_lock = "deps/locks/skia-transitive-windows-x64.lock.json"
      windows_transitive_lock_generated = $dependencyLockGenerated
      qt_engineering_path_required = ($Lane -eq "Visual")
      qt_release_entitlement_checked = $false
      macos_reference_capture = "docs/evidence/reviews/artifacts/xplat-visual-v1-macos-metal-640x360.ppm"
      windows_capture = if ($CompileOnly -or $Lane -eq "Core") { $null } else { "out/evidence/xplat-visual-v1-windows-d3d12-640x360.ppm" }
      visual_comparison = if ($CompileOnly -or $Lane -eq "Core") { $null } else { "out/evidence/xplat-visual-v1-metal-vs-d3d12.json" }
      qualification_receipt = if ($CompileOnly -or $Lane -eq "Core") { $null } else { "out/evidence/windows-d3d12-visual-qualification.json" }
    }
    steps = $stepReceipts
    initial_dirty_entries = $initialDirty
    final_dirty_entries = $finalDirty
    status = if ($null -eq $failure) { "passed" } else { "failed" }
    diagnostic = if ($null -eq $failure) { $null } else { $failure.Exception.Message }
  }
  $receipt | ConvertTo-Json -Depth 8 | Set-Content -Path $resolvedReceipt -Encoding utf8NoBOM
  Write-Host "ReFusion Windows receipt: $resolvedReceipt"
}

if ($null -ne $failure) {
  throw $failure
}
