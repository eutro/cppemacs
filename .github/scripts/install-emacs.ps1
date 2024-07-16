# Download the official pre-built Windows binaries.
# (could they be insecure? probably!)

$ErrorActionPreference = "Stop"

$emacsVersion = [Version]$env:EMACS_VERSION

if (!$emacsVersion -or ($emacsVersion -le [Version]"5.0")) {
    echo "Only Emacs 25+ supported"
    Exit
}
echo "Installing Emacs $emacsVersion"

# different ftp directory structures on different versions...
if ($emacsVersion -ge [Version]"28.0") {
    $emacsPrefix = "$env:EMACS_PREFIX\emacs-$emacsVersion"
} else {
    $archPart = "-x86_64"
    $emacsPrefix = $env:EMACS_PREFIX
}

if (Test-Path "$emacsPrefix\bin\emacs.exe") {
    echo "Emacs already installed at $emacsPrefix"
} else {
    if ($env:RUNNER_TEMP) {
        $tmpdir = $env:RUNNER_TEMP
    } else {
        $tmpdir = $env:Temp
    }

    echo "::group::Fetch Emacs"
    $mirrorUrl = "https://ftpmirror.gnu.org/emacs/windows"
    $zipPath = "$tmpdir\emacs-$([System.IO.Path]::GetRandomFileName()).zip"

    $zipUrl = "$mirrorUrl/emacs-$($emacsVersion.Major)/emacs-$emacsVersion$archPart.zip"

    echo "Downloading $zipUrl to: $zipPath"
    Invoke-Webrequest -Uri $zipUrl -OutFile $zipPath
    Expand-Archive -LiteralPath $zipPath -DestinationPath $env:EMACS_PREFIX
    echo "::endgroup::"
}

echo "::group::Add Emacs to PATH"
(Resolve-Path "$emacsPrefix\bin").toString()  >> $env:GITHUB_PATH
echo "::endgroup::"
