# Third-Party Licenses

This file records every third-party dependency actually selected in Phase 0,
per `ARCHITECTURE.md`. Versions and license terms below were verified via
upstream/package-registry sources on 2026-09-04; re-verify before each major
release since upstream projects do occasionally change license terms across
versions.

Legend: ✅ Decided and locked · 🟡 Provisional, pending the Phase 0 §1 build
spike.

---

## ✅ pugixml — XML parser

- **Version:** 1.16 (released 2026-06-16)
- **License:** MIT
- **Source:** https://github.com/zeux/pugixml (upstream site: pugixml.org)
- **Why selected:** MIT is unconditionally compatible with commercial,
  closed-source redistribution — no copyleft, no required attribution in
  shipped UI (an acknowledgment is *suggested* by upstream, not mandatory).
  No external entity/network resolution by default. Small, fast, in-memory
  DOM; actively maintained (1.16 is a 2026 release).
- **Commercial redistribution:** No restriction of any kind.
- **Rejected alternatives:**
  - tinyxml2 (zlib license) — license also fine, but no compelling technical
    advantage over pugixml for this project; pugixml has a faster parser and
    a more ergonomic traversal API for relationship/part-list parsing.
  - libxml2 (MIT-family) — real namespace-URI resolution and DTD/XPath 2
    support, but a materially heavier dependency (libiconv, zlib, sometimes
    liblzma) for capabilities not needed at this phase.
- **Known limitation to design around:** pugixml does not resolve `xmlns`
  prefixes to namespace URIs; it exposes literal qualified names
  (`"a:off"`, `"p:sp"`). See `ARCHITECTURE.md` §2 for the full note — code
  must match on literal qualified names, not assume namespace-URI resolution.

---

## ✅ libzip — ZIP/package reading

- **Version:** 1.11.4
- **License:** BSD-3-Clause (three-clause BSD, sometimes labeled "BSD" in
  package indexes — text confirmed at libzip.org/license)
- **Source:** https://libzip.org / https://github.com/nih-at/libzip
- **Why selected:** BSD-3-Clause is unconditionally compatible with
  commercial redistribution. Supports custom `zip_source` callbacks (lets the
  ZIP layer be backed by this project's `IPackageReader` rather than a raw
  file descriptor), random-access per-entry reads without full extraction
  (needed to enforce per-entry size limits while streaming), and is mature
  and extremely widely deployed (e.g. backs PHP's `ZipArchive`).
- **Mandatory runtime dependency:** zlib (for Deflate/Store, the only
  compression methods valid `.pptx` packages use). Optional
  bzip2/LZMA/zstd/OpenSSL support is **not** required and should be disabled
  at build time to minimize dependency surface.
- **Commercial redistribution:** No restriction beyond standard BSD-3-Clause
  attribution-preservation (reproduce copyright notice + disclaimer; do not
  use the authors' names to endorse derived products without permission).
- **Rejected alternative:**
  - minizip-ng 4.2.1 (Zlib license) — also license-compatible and technically
    capable, but its general-purpose feature set (disk splitting, WinZip AES
    encryption, legacy codepage support) and larger default optional-
    dependency surface (bzip2, xz, zstd, openssl) are unneeded for a
    read-mostly, OOXML-specific ZIP layer. libzip's narrower API is a better
    match.

---

## 🟡 Rendering stack — PROVISIONAL, pending local build spike (ARCHITECTURE.md §1)

None of the four libraries below are locked in yet. This is the desk-research
comparison and the *recommended* default; do not add build-system dependencies
on any of them until the local verification step in `ARCHITECTURE.md` §1 has
actually been run and its result recorded here.

### Cairo (recommended default — vector graphics + rasterization)
- **Version:** 1.18.4 (released 2025-03-08)
- **License:** dual **LGPL-2.1-only OR MPL-1.1** — integrator must pick one.
  **Recommendation: MPL-1.1**, because it is a file-level copyleft that does
  not carry LGPL's "must permit relinking against a different version of the
  library" obligation for statically-linked proprietary consumers — a
  materially safer choice for a library meant to be embedded in arbitrary,
  possibly closed-source host applications.
- **Source:** https://www.cairographics.org /
  https://gitlab.freedesktop.org/cairo/cairo
- **Commercial redistribution:** Compatible under either license option, with
  the MPL-1.1 path being the lower-friction one for static linking into
  closed-source hosts (see above).

### Pango (recommended default — text layout/shaping orchestration)
- **Version:** 1.57.1 (MacPorts `pango-devel`, 2026)
- **License:** LGPL-2.0-or-later
- **Source:** https://gitlab.gnome.org/GNOME/pango
- **Commercial redistribution:** Compatible with dynamic linking (standard
  LGPL terms — host application must permit relinking against a modified
  Pango). Pulls in GLib and typically fontconfig as transitive dependencies.
- **Note:** this is the component most likely to complicate a *future*
  Android port (fontconfig is not a natural fit there) — noted as a known
  future-porting cost, not a Phase 0/1 blocker.

### HarfBuzz (recommended default — text shaping)
- **Version:** 13.2.1 (per Mozilla's vendoring record, 2026-03-19)
- **License:** MIT ("Old MIT" style — functionally equivalent permissive
  terms; full text in HarfBuzz's `COPYING`)
- **Source:** https://github.com/harfbuzz/harfbuzz
- **Commercial redistribution:** No restriction of any kind.

### FreeType (recommended default — font parsing/rasterization)
- **Version:** 2.14.1
- **License:** dual **FTL (FreeType License) OR GPLv2(+)** — integrator must
  pick one. **Recommendation: FTL.** FTL is a BSD-style license with an
  attribution-in-documentation clause (compatible with GPLv3, notably
  *incompatible* with GPLv2 due to that clause) — the correct pick for a
  permissively-licensed, commercially-redistributable library; do not accept
  the GPLv2 option by default.
- **Source:** https://freetype.org
- **Commercial redistribution:** Compatible under FTL, subject to the
  attribution-in-documentation requirement (credit FreeType somewhere in
  shipped product documentation — this is a documentation credit, not a
  royalty or fee).

### Skia (evaluated, not currently recommended — see ARCHITECTURE.md §1)
- **Version:** No stable numbered releases; tracked by git revision only
  (this itself is a build/maintainability consideration, not just a
  licensing one — there is no version to pin THIRD_PARTY_LICENSES.md against
  until a specific commit is chosen).
- **License:** New BSD License (permissive)
- **Source:** https://skia.org / https://skia.googlesource.com/skia.git
- **Commercial redistribution:** No restriction.
- **Why not currently recommended:** not a licensing objection — a build-cost
  one. Requires Google's `depot_tools`/GN/Ninja toolchain and a large
  dependency checkout (Skia vendors its own copies of FreeType, HarfBuzz,
  libpng, libwebp, zlib under `third_party/externals/`). Best Android story
  of any option evaluated (Skia is Android's own graphics stack), which is
  the main reason it remains under consideration rather than eliminated
  outright — see the spike instructions in ARCHITECTURE.md §1.

### Blend2D (evaluated, noted as fallback-of-the-fallback)
- **Version:** 0.21.2
- **License:** Zlib (permissive); its JIT dependency AsmJit is also Zlib
- **Source:** https://blend2d.com / https://github.com/blend2d/blend2d
- **Commercial redistribution:** No restriction.
- **Why not the current recommendation:** solves vector/raster drawing with a
  more turnkey build than Skia, but has **no built-in text shaping** —
  HarfBuzz+FreeType would still be required alongside it, so it doesn't
  remove a dependency, only replaces Cairo's role. Kept as a fallback in case
  Cairo's LGPL/MPL dual licensing proves unworkable for a specific
  downstream redistribution scenario not yet known.

---

## Summary table

| Dependency | Version | License | Status |
|---|---|---|---|
| pugixml | 1.16 | MIT | ✅ Decided |
| libzip | 1.11.4 | BSD-3-Clause | ✅ Decided |
| Cairo | 1.18.4 | LGPL-2.1 or MPL-1.1 (use MPL-1.1) | 🟡 Provisional |
| Pango | 1.57.1 | LGPL-2.0-or-later | 🟡 Provisional |
| HarfBuzz | 13.2.1 | MIT | 🟡 Provisional |
| FreeType | 2.14.1 | FTL or GPLv2 (use FTL) | 🟡 Provisional |
| Skia | (no version tag; git revision only) | New BSD | 🟡 Provisional, evaluated only |
| Blend2D | 0.21.2 | Zlib | 🟡 Provisional, evaluated only |

No dependency evaluated — decided or provisional — carries per-user
royalties, per-document fees, mandatory commercial licensing, or
evaluation-only restrictions.
