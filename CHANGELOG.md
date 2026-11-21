# Changelog

## 1.2.0

### Performance / memory
- Terminal, Source Control, Debugger, Settings, Command Palette, Find in Files, About and the completion popup are now built on first use. No shell process is spawned at startup.
- Syntax highlighting: keyword groups are one regex each (was one regex per keyword, ~100 per C++ file) and compiled patterns are shared between all open files. Lines over 4000 chars are only highlighted up to that point.
- Highlighter and find bar are created per editor only when needed; plain-text files never create a highlighter.
- Files larger than `editor.largeFileKiB` (default 2 MiB) open without highlighting or minimap. Files over 256 MiB are refused instead of freezing the UI.
- Session restore happens after the first paint.
- Removed emoji/pictograph glyphs from the UI (they forced a color-emoji font to load); fold markers are drawn as shapes.
- Build: `-O2`, `-ffunction-sections`, `--gc-sections`, hidden visibility, `--as-needed`, optional LTO.
- Measured (offscreen, Qt 6.4, same machine): empty launch 47.3 MB -> 37.8 MB RSS (anonymous 14.0 -> 9.2 MB), 6 C++ files restored 56.2 -> 43.3 MB, startup CPU time 90 -> 30 ms (empty) and 300 -> ~100 ms (6 files).

### Look and customisation
- New stylesheet: flat tabs with an accent underline, theme-aware hover/selection (works on light themes), no hard-coded colors.
- New themes: LiteForge Daylight (light), Gruvbox Dark, Nord, Catppuccin Mocha.
- **User themes**: drop JSON files into `<config>/themes/`; override any of the 40 colors on top of a `base` theme. *Help -> Export Current Theme as Template* writes a ready-to-edit file.
- **Keybindings**: `keybindings.json` remaps any menu action; *Help -> Open Keybindings (JSON)* seeds it with all current bindings. The Command Palette is generated from the menus and shows your bindings.
- UI settings: density (compact/normal/comfortable), UI font and size, corner radius, accent color override, toolbar/status bar visibility.
- Saving `config.json`, `keybindings.json` or a theme file inside the config directory applies it immediately (or use *Help -> Reload Config, Themes & Keys*).
- Settings dialog changes (theme, density, minimap, ...) now apply immediately; previously only the editor font did.
- Window geometry and splitter sizes are remembered.
- Auto-save (after N seconds and/or on focus loss), reload-on-external-change for clean files, prompt for dirty ones.

### Bug fixes
- **LSP format/rename could destroy the file** (every edit replaced the whole document with the edit's text). Edits are now applied by range, as one undo step. Rename understands `documentChanges` (clangd, rust-analyzer, ...).
- LSP never received document changes after open (`didChange` was never sent); now sent debounced. Files opened before the server finished starting are announced when it is ready. Correct language ids are sent (was always `cpp`). Diagnostics no longer duplicate on every publish.
- Editing a file opened in the *preview* tab never marked it modified, so closing it discarded edits without asking.
- Ctrl+S on an untitled file did nothing, and a modified untitled tab could not be saved on close. Save As now renames the tab in place instead of opening a second copy.
- Saving rewrote CRLF files as LF and dropped UTF-8 BOMs; now both are preserved. Writes are atomic (no truncated file on crash / full disk).
- *Trim trailing whitespace* and *final newline* settings were never applied (and are skipped for Markdown hard line breaks).
- Pinned/preview tab state was tracked by tab index and broke when tabs were closed or dragged.
- Editor signal handlers were re-connected on every tab switch, multiplying LSP requests and cursor updates.
- Quick Open (Ctrl+P) did not open the chosen file.
- Rebuild started a build while the clean was still running and was rejected.
- Config is saved after the unsaved-changes prompt, not before; saved atomically; out-of-range values are clamped.
- Tests used `assert()` in Release builds, where it is compiled out - the suite passed vacuously. Fixed; added 3 core and 8 headless GUI tests.

### Packaging
- AppImage now actually bundles Qt (linuxdeploy + Qt plugin) for x86_64, aarch64 and armv7.
- RPMs are built on Fedora from a proper spec (x86_64, aarch64). tar.gz and deb for x86_64, aarch64, armv7, riscv64, ppc64le, s390x.
- Recipes: Arch (`PKGBUILD`, `PKGBUILD-git`), Alpine (`APKBUILD`), Gentoo (1.2.0 + live ebuild), Nix (flake, `default.nix`, nixpkgs `package.nix`), Void, RPM spec, FreeBSD / OpenBSD / NetBSD ports, generic BSD build script.
- CI: GCC + Clang, x86_64 + aarch64, ASan/UBSan job. `-DENABLE_TESTING=OFF` is now a real option.
- Version is defined once (`CMakeLists.txt`) and shown in the About dialog / `--version`.
