# LiteForge IDE

<p align="center">
  <img src="assets/icon.svg" alt="LiteForge Logo" width="128" height="128">
</p>

<p align="center">
  <strong>An ultra-fast, featherweight, and zero-bloat native C++20 & Qt6 IDE for developers.</strong>
</p>

<p align="center">
  <a href="https://github.com/Dacraezy1/LiteForge-IDE/actions"><img src="https://img.shields.io/github/actions/workflow/status/Dacraezy1/LiteForge-IDE/ci.yml?branch=main&label=CI&style=flat-square" alt="CI Status"></a>
  <a href="https://github.com/Dacraezy1/LiteForge-IDE/releases"><img src="https://img.shields.io/github/v/release/Dacraezy1/LiteForge-IDE?style=flat-square&color=blue" alt="Latest Release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-green.svg?style=flat-square" alt="License"></a>
  <img src="https://img.shields.io/badge/RAM-~35MB-informational?style=flat-square" alt="Memory Usage">
  <img src="https://img.shields.io/badge/Startup-%3C40ms-success?style=flat-square" alt="Startup Time">
  <img src="https://img.shields.io/badge/Architectures-x86__64%20%7C%20aarch64-blueviolet?style=flat-square" alt="Supported Architectures">
</p>

---

## Why LiteForge?

Modern code editors and IDEs have become notoriously resource-intensive:
- Electron-based editors frequently idle at **800 MB – 1.5 GB of RAM** and experience perceptible typing latency.
- Heavy IDE suites introduce bloated startup times, complex background telemetry, and gigabytes of disk overhead.

**LiteForge is built from the ground up to solve this:**
- **Instant Cold Boot**: Launches in under 40 milliseconds.
- **Micro Footprint**: Maintains an idle footprint of approximately **35 MB RAM**.
- **100% Native**: Written in pure **C++20** and **Qt6**, compiling directly to optimized machine code for **x86_64** and **aarch64 (ARM64)**.
- **Zero Telemetry, Zero Bloat**: No embedded Chromium, no JavaScript runtime, no background tracking.

---

## Features

### ⚡ Editor Engine
- **Bracket Matching & Auto-Closing**: Automatic pair closing and matching highlights for `()`, `[]`, `{}` and quotes.
- **Smart Indentation**: Auto-indents on Return after code blocks (`{`, `:`); handles tab-to-spaces and shift-unindent.
- **Visual Indent Guides**: Subtle dotted guidelines for nested code blocks.
- **Syntax Highlighting**: High-performance multi-language tokenization for C, C++, Python, Rust, JavaScript, TypeScript, JSON, Markdown, CMake, and Shell.
- **Inline Find & Replace**: Non-intrusive HUD (`Ctrl+F`, `Ctrl+H`) with regex, whole-word, case matching, and instant hit counts.
- **Advanced Editing**: Duplicate line (`Ctrl+D`), delete line (`Ctrl+Shift+K`), move lines (`Alt+Up`/`Alt+Down`), toggle comments (`Ctrl+/`), and go to line (`Ctrl+G`).

### 📂 Navigation & Project Management
- **Subsequence Fuzzy Open (`Ctrl+P`)**: Lightning-fast file search across entire directory trees.
- **Command Palette (`Ctrl+Shift+P`)**: Instant keyboard access to every IDE action.
- **Project Explorer**: Lightweight workspace tree with file icons, context menus, and file filtering.
- **Project-Wide Search (`Ctrl+Shift+F`)**: Multi-threaded text search across all workspace files with direct jump-to-line.

### 🖥️ Integrated Terminal & Build System
- **Embedded Shell Console**: Interactive multi-tab terminal panel (`Ctrl+~`) supporting Bash, Zsh, and Fish.
- **Build Runner**: One-key build integration (`Ctrl+F7`) supporting CMake, Make, Cargo, and custom compilers.
- **Compiler Error Jump**: Click errors (`file:line:col`) in build output to navigate straight to the error line.
- **Problems & Diagnostics Panel**: Aggregated compiler warnings and error diagnostics.

### 🎨 Customization
- **Built-in Dark & Light Themes**: Obsidian, Tokyo Night, One Dark Pro, Monokai Pro, and Cyberpunk Neon.
- **Distraction-Free Mode (`F11`)**: Fullscreen minimal code canvas.
- **Customizable Preferences**: Configurable font family, font size, tab size, indentation mode, and build targets.

---

## Installation

LiteForge provides pre-built binaries for **x86_64** and **aarch64 (ARM64)** on every [GitHub Release](https://github.com/Dacraezy1/LiteForge-IDE/releases), alongside native packaging recipes for all major Linux distributions and BSD operating systems.

### 1. AppImage (Universal Linux - x86_64 & aarch64)

The easiest way to run LiteForge on any modern Linux distribution without installing dependencies:

```bash
# Download for your architecture from GitHub Releases:
# For x86_64:
curl -LO https://github.com/Dacraezy1/LiteForge-IDE/releases/download/v1.0.0/LiteForge-x86_64.AppImage
chmod +x LiteForge-x86_64.AppImage
./LiteForge-x86_64.AppImage

# For aarch64 / ARM64:
curl -LO https://github.com/Dacraezy1/LiteForge-IDE/releases/download/v1.0.0/LiteForge-aarch64.AppImage
chmod +x LiteForge-aarch64.AppImage
./LiteForge-aarch64.AppImage
```

---

### 2. Debian / Ubuntu / Pop!_OS / Linux Mint (`.deb`)

Download and install the native `.deb` package:

```bash
# For x86_64 (amd64):
wget https://github.com/Dacraezy1/LiteForge-IDE/releases/download/v1.0.0/liteforge_1.0.0_amd64.deb
sudo apt install ./liteforge_1.0.0_amd64.deb

# For aarch64 (arm64):
wget https://github.com/Dacraezy1/LiteForge-IDE/releases/download/v1.0.0/liteforge_1.0.0_arm64.deb
sudo apt install ./liteforge_1.0.0_arm64.deb
```

---

### 3. Fedora / RHEL / CentOS / openSUSE (`.rpm`)

Install the native RPM package via DNF or Zypper:

```bash
# Fedora / RHEL / Rocky (x86_64):
sudo dnf install https://github.com/Dacraezy1/LiteForge-IDE/releases/download/v1.0.0/liteforge-1.0.0-1.x86_64.rpm

# Fedora / RHEL / Rocky (aarch64):
sudo dnf install https://github.com/Dacraezy1/LiteForge-IDE/releases/download/v1.0.0/liteforge-1.0.0-1.aarch64.rpm

# openSUSE:
sudo zypper install https://github.com/Dacraezy1/LiteForge-IDE/releases/download/v1.0.0/liteforge-1.0.0-1.x86_64.rpm
```

---

### 4. Portable Tarball (`.tar.gz`)

For portable or rootless usage:

```bash
# Extract and run:
tar -xzf liteforge-1.0.0-linux-x86_64.tar.gz
cd liteforge
./bin/liteforge
```

---

### 5. Arch Linux / Manjaro (`PKGBUILD`)

Install via the provided Arch recipe:

```bash
git clone https://github.com/Dacraezy1/LiteForge-IDE.git
cd LiteForge-IDE/packaging/arch
makepkg -si
```

---

### 6. Nix / NixOS

#### Run with Flakes:
```bash
nix run github:Dacraezy1/LiteForge-IDE
```

#### Build with classic Nix:
```bash
git clone https://github.com/Dacraezy1/LiteForge-IDE.git
cd LiteForge-IDE
nix-build packaging/nix
./result/bin/liteforge
```

---

### 7. Alpine Linux (`APKBUILD`)

Build and install on Alpine Linux using `abuild`:

```bash
# Install build requirements
apk add alpine-sdk cmake qt6-qtbase-dev linux-headers

git clone https://github.com/Dacraezy1/LiteForge-IDE.git
cd LiteForge-IDE/packaging/alpine
abuild -r
```

---

### 8. Gentoo Linux (`ebuild`)

Add LiteForge to your local portage overlay:

```bash
sudo mkdir -p /var/db/repos/localrepo/dev-util/liteforge
sudo cp packaging/gentoo/liteforge-1.0.0.ebuild /var/db/repos/localrepo/dev-util/liteforge/
ebuild /var/db/repos/localrepo/dev-util/liteforge/liteforge-1.0.0.ebuild manifest
sudo emerge --ask dev-util/liteforge
```

---

### 9. Void Linux (`xbps-src`)

Compile using Void's `xbps-src`:

```bash
git clone --depth=1 https://github.com/void-linux/void-packages.git
cd void-packages
cp -r /path/to/LiteForge-IDE/packaging/void srcpkgs/liteforge
./xbps-src pkg liteforge
sudo xbps-install --repository hostdir/binpkgs liteforge
```

---

### 10. BSD Systems (FreeBSD & OpenBSD)

LiteForge compiles cleanly on FreeBSD and OpenBSD using the dedicated BSD build script:

#### FreeBSD:
```bash
# Install dependencies
pkg install cmake qt6-base git

# Clone and build
git clone https://github.com/Dacraezy1/LiteForge-IDE.git
cd LiteForge-IDE
sh packaging/bsd/build-bsd.sh

# Install system-wide
sudo cmake --install build
```

#### OpenBSD:
```bash
# Install dependencies
pkg_add cmake qt6 git

# Clone and build
git clone https://github.com/Dacraezy1/LiteForge-IDE.git
cd LiteForge-IDE
sh packaging/bsd/build-bsd.sh

# Install system-wide
doas cmake --install build
```

---

### 11. Verifying Checksums

Every release includes a `SHA256SUMS.txt` manifest:

```bash
sha256sum -c SHA256SUMS.txt
```

---

## Building from Source

### Requirements
- **C++20 Compiler**: GCC >= 11 or Clang >= 13
- **CMake**: >= 3.20
- **Qt6**: `Core`, `Gui`, `Widgets` (>= 6.2)

```bash
# 1. Clone repository
git clone https://github.com/Dacraezy1/LiteForge-IDE.git
cd LiteForge-IDE

# 2. Configure build with Release optimizations (-O3, LTO)
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=20

# 3. Compile in parallel
cmake --build build --parallel $(nproc)

# 4. Run test suite
ctest --test-dir build --output-on-failure

# 5. Launch LiteForge
./build/liteforge

# 6. Optional: Install system-wide
sudo cmake --install build
```

---

## Keyboard Shortcuts

| Shortcut | Command / Action |
| :--- | :--- |
| `Ctrl+P` | Subsequence Fuzzy File Open |
| `Ctrl+Shift+P` | Command Palette |
| `Ctrl+N` | New Document Tab |
| `Ctrl+O` | Open Existing File |
| `Ctrl+Shift+O` | Open Project / Folder |
| `Ctrl+S` | Save Active Document |
| `Ctrl+Shift+S` | Save All Open Documents |
| `Ctrl+W` | Close Current Document Tab |
| `Ctrl+F` | Document Find Bar |
| `Ctrl+H` | Document Replace Bar |
| `Ctrl+Shift+F` | Project-Wide Grep Search |
| `Ctrl+G` | Jump to Line Number (`:line[:col]`) |
| `Ctrl+/` | Toggle Line / Selection Comment |
| `Ctrl+D` | Duplicate Current Line or Selection |
| `Ctrl+Shift+K` | Delete Current Line |
| `Alt+Up` / `Alt+Down` | Move Active Line Up / Down |
| `Ctrl+B` | Toggle Sidebar (File Explorer) |
| `Ctrl+~` | Toggle Bottom Terminal Panel |
| `F5` | Run Current File / Target |
| `Ctrl+F7` | Build Project (CMake / Make / Cargo) |
| `Ctrl+Shift+B` | Clean and Rebuild Project |
| `F11` | Toggle Distraction-Free Fullscreen |
| `Ctrl+,` | Preferences & Settings Dialog |

---

## Contributing & License

LiteForge is free and open-source software distributed under the terms of the [MIT License](LICENSE). Contributions, bug reports, and pull requests are welcome!
