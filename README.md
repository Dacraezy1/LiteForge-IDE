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
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPLv3-blue.svg?style=flat-square" alt="License"></a>
  <img src="https://img.shields.io/badge/design-minimal%20%26%20unbloated-informational?style=flat-square" alt="Minimal & Unbloated">
  <img src="https://img.shields.io/badge/Startup-%3C40ms-success?style=flat-square" alt="Startup Time">
  <img src="https://img.shields.io/badge/Architectures-x86__64%20%7C%20aarch64-blueviolet?style=flat-square" alt="Supported Architectures">
</p>

---

## Why LiteForge?

Modern code editors and IDEs have become notoriously resource-intensive:
- Electron-based editors frequently consume massive memory overhead and introduce perceptible typing latency.
- Heavy IDE suites introduce bloated startup times, complex background telemetry, and gigabytes of disk overhead.

**LiteForge is built from the ground up to solve this:**
- **Instant Cold Boot**: Launches in under 40 milliseconds.
- **Minimal & Lean**: Designed strictly without bloat, keeping memory footprint low and resource usage minimal.
- **100% Native**: Written in pure **C++20** and **Qt6**, compiling directly to optimized machine code for **x86_64** and **aarch64 (ARM64)**.
- **Zero Telemetry, Zero Bloat**: No embedded Chromium, no JavaScript runtime, no background tracking.

---

## Features

### ⚡ Editor Engine
- **Multiple Cursors & Select Next Occurrence**: Select and edit multiple occurrences simultaneously (`Ctrl+D`), column editing, and multi-caret synchronized typing.
- **Split Editor Views**: Split view horizontally or vertically (`Ctrl+\`) to work on multiple files or different sections of the same file side-by-side.
- **Code Folding**: Fold/unfold functions, loops, and blocks with gutter chevrons (`▼`/`▶`) or keyboard shortcuts (`Ctrl+Shift+[` / `Ctrl+Shift+]`).
- **Minimap & Sticky Scroll**: High-performance overview canvas of document layout and sticky top header tracking the current enclosing scope.
- **Tab Enhancements**: Pinned tabs (`📌`), temporary preview tabs (italics), dirty file indicators (`●`), and split menu actions.
- **Bracket Matching & Auto-Closing**: Automatic pair closing and matching highlights for `()`, `[]`, `{}` and quotes.
- **Smart Indentation**: Auto-indents on Return after code blocks (`{`, `:`); handles tab-to-spaces and shift-unindent.
- **Visual Indent Guides**: Subtle dotted guidelines for nested code blocks.
- **Syntax Highlighting**: High-performance multi-language tokenization for C, C++, Python, Rust, JavaScript, TypeScript, JSON, Markdown, CMake, and Shell.
- **Inline Find & Replace**: Non-intrusive HUD (`Ctrl+F`, `Ctrl+H`) with regex, whole-word, case matching, and instant hit counts.
- **Advanced Editing**: Delete line (`Ctrl+Shift+K`), move lines (`Alt+Up`/`Alt+Down`), toggle comments (`Ctrl+/`), and go to line (`Ctrl+G`).

### 🧠 Language Intelligence (LSP)
- **Go to Definition (`F12`) & Find References (`Shift+F12`)**: Direct jump to declaration or all symbol references across the project.
- **Hover Documentation**: Detailed documentation, markdown tooltips, and type signatures when hovering over symbols.
- **Intelligent Autocompletion**: Real-time context-aware completion popup with auto-closing and signature help.
- **Symbol Renaming (`F2`)**: Safe workspace-wide symbol refactoring.
- **Code Formatting (`Shift+Alt+F`)**: Native LSP document formatting (ClangFormat, rustfmt, Black, etc.).
- **Diagnostics & Error Navigation**: Real-time squiggles, error count badges, and one-click navigation to diagnostics.

### 🌿 Integrated Git & Source Control
- **Async Git Engine**: Zero-lag Git CLI integration running completely in the background.
- **Source Control Sidebar**: View unstaged changes, stage/unstage files (`+`/`-`), and commit directly with keyboard shortcuts.
- **Branch Management & Push/Pull**: Switch branches, create branches, and sync with remote repositories.
- **Visual Diff Viewer**: Dedicated side-by-side / unified diff inspector with syntax-colored additions and deletions.

### 🐞 Native GDB/MI Debugger
- **GDB Async Client**: Integrated GDB Machine Interface backend without external heavy debugger plugins.
- **Breakpoints**: Click gutter or press `F9` to toggle line breakpoints with visual indicator dots.
- **Execution Control**: Continue (`F5`), Step Over (`F10`), Step Into (`F11`), Step Out (`Shift+F11`), Pause, and Stop.
- **Inspector Panels**: Real-time Call Stack frame navigator, Local Variables tree, and GDB console output.

### 📂 Navigation & Project Management
- **Async Background Scanner**: Project files are indexed asynchronously with high-speed `.gitignore` wildcard filtering.
- **Project Templates**: Instant scaffolding for modern C++20 CMake, Rust Cargo, Python App, and Minimal Web projects.
- **Welcome Dashboard**: Minimalist startup screen with quick access to recent projects, templates, and documentation.
- **Subsequence Fuzzy Open (`Ctrl+P`)**: Lightning-fast file search across entire directory trees.
- **Command Palette (`Ctrl+Shift+P`)**: Instant keyboard access to every IDE action.
- **Project Explorer**: Lightweight workspace tree with file icons, context menus, and file filtering.
- **Project-Wide Search (`Ctrl+Shift+F`)**: Multi-threaded text search across all workspace files with direct jump-to-line.

### 🖥️ Integrated Terminal & Build System
- **Embedded Shell Console**: Interactive multi-tab terminal panel (`Ctrl+~`) supporting Bash, Zsh, and Fish.
- **Build Runner**: One-key build integration (`Ctrl+F7`) supporting CMake, Make, Cargo, and custom compilers.
- **Compiler Error Jump**: Click errors (`file:line:col`) in build output to navigate straight to the error line.
- **Problems & Diagnostics Panel**: Aggregated compiler warnings and error diagnostics.

### 🎨 Customization & Workspace Profiles
- **Workspace Profiles**: One-click configuration presets (C/C++, Python, Rust, Web, Minimal Writing) with JSON import/export.
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
curl -LO https://github.com/Dacraezy1/LiteForge-IDE/releases/latest/download/LiteForge-x86_64.AppImage
chmod +x LiteForge-x86_64.AppImage
./LiteForge-x86_64.AppImage

# For aarch64 / ARM64:
curl -LO https://github.com/Dacraezy1/LiteForge-IDE/releases/latest/download/LiteForge-aarch64.AppImage
chmod +x LiteForge-aarch64.AppImage
./LiteForge-aarch64.AppImage
```

---

### 2. Debian / Ubuntu / Pop!_OS / Linux Mint (`.deb`)

Download and install the native `.deb` package:

```bash
# For x86_64 (amd64):
wget https://github.com/Dacraezy1/LiteForge-IDE/releases/latest/download/liteforge_1.1.0_amd64.deb
sudo apt install ./liteforge_1.1.0_amd64.deb

# For aarch64 (arm64):
wget https://github.com/Dacraezy1/LiteForge-IDE/releases/latest/download/liteforge_1.1.0_arm64.deb
sudo apt install ./liteforge_1.1.0_arm64.deb
```

---

### 3. Fedora / RHEL / CentOS / openSUSE (`.rpm`)

Install the native RPM package via DNF or Zypper:

```bash
# Fedora / RHEL / Rocky (x86_64):
sudo dnf install https://github.com/Dacraezy1/LiteForge-IDE/releases/latest/download/liteforge-1.1.0-1.x86_64.rpm

# Fedora / RHEL / Rocky (aarch64):
sudo dnf install https://github.com/Dacraezy1/LiteForge-IDE/releases/latest/download/liteforge-1.1.0-1.aarch64.rpm

# openSUSE:
sudo zypper install https://github.com/Dacraezy1/LiteForge-IDE/releases/latest/download/liteforge-1.1.0-1.x86_64.rpm
```

---

### 4. Portable Tarball (`.tar.gz`)

For portable or rootless usage:

```bash
# Extract and run:
tar -xzf liteforge-1.1.0-Linux.tar.gz
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
sudo cp packaging/gentoo/liteforge-1.1.0.ebuild /var/db/repos/localrepo/dev-util/liteforge/
ebuild /var/db/repos/localrepo/dev-util/liteforge/liteforge-1.1.0.ebuild manifest
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
| `Ctrl+\` | Split Editor View (Toggle Vertical / Horizontal Split) |
| `Ctrl+D` | Select Next Occurrence (Multi-Cursor) |
| `Ctrl+F` | Document Find Bar |
| `Ctrl+H` | Document Replace Bar |
| `Ctrl+Shift+F` | Project-Wide Grep Search |
| `Ctrl+G` | Jump to Line Number (`:line[:col]`) |
| `Ctrl+/` | Toggle Line / Selection Comment |
| `Ctrl+Shift+K` | Delete Current Line |
| `Alt+Up` / `Alt+Down` | Move Active Line Up / Down |
| `Ctrl+Shift+[` | Fold Current Code Block |
| `Ctrl+Shift+]` | Unfold Current Code Block |
| `F12` | Go to Definition (LSP) |
| `Shift+F12` | Find All References (LSP) |
| `F2` | Rename Symbol (LSP) |
| `Shift+Alt+F` | Format Document (LSP) |
| `F9` | Toggle Breakpoint |
| `F5` | Start / Continue Debugging (GDB) |
| `F10` | Debug Step Over |
| `F11` | Debug Step Into / Distraction-Free Toggle |
| `Shift+F11` | Debug Step Out |
| `Shift+F5` | Stop Debugging |
| `Ctrl+B` | Toggle Sidebar |
| `Ctrl+~` | Toggle Bottom Terminal Panel |
| `Ctrl+F7` | Build Project (CMake / Make / Cargo) |
| `Ctrl+Shift+B` | Clean and Rebuild Project |
| `Ctrl+,` | Preferences & Settings Dialog |

---

## Contributing & License

LiteForge is free and open-source software distributed under the terms of the [GNU General Public License v3 (GPLv3)](LICENSE). Contributions, bug reports, and pull requests are welcome!
