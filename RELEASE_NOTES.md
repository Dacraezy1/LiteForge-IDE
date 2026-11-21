## LiteForge 1.2.0

Lighter, faster, and much more customisable - plus a round of data-loss bug fixes.

**Lighter / faster:** panels are built on first use, no shell at startup, shared syntax-highlighting patterns. In our offscreen measurements: ~20% less RSS on an empty launch (~35% less private memory), ~23% less with several files open, and about a third of the startup CPU time.

**Customisable:** JSON themes (4 new built-ins + your own), JSON keybindings, UI density / font / accent / corner radius, auto-save, instant apply of settings.

**Important fixes:** LSP rename/format no longer overwrites the whole file; edits in preview tabs are tracked; Ctrl+S works on untitled files; CRLF and BOM are preserved; atomic saves.

### Downloads
| Format | Architectures |
|---|---|
| `.AppImage` (Qt bundled; needs glibc 2.39+) | x86_64, aarch64, armv7 |
| `.rpm` (Fedora-built) | x86_64, aarch64 |
| `.deb` | amd64, arm64, armhf, riscv64, ppc64el, s390x |
| `.tar.gz` | x86_64, aarch64, armv7, riscv64, ppc64le, s390x |
| source | `liteforge-1.2.0-source.tar.gz` |

Package recipes for Arch, Alpine, Gentoo, Nix, Void and the BSDs are in `packaging/`. Verify downloads with `SHA256SUMS.txt`. Full details: `CHANGELOG.md`.
