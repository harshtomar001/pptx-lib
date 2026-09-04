# Architecture

This document records the binding Phase 0 decisions for the PPTX library, per
`docs/Phase0-1_Kickoff_Prompt.md`. Each decision below is final for Phase 1
**except where explicitly marked "pending local verification"** — those items
are the project's provisional best answer based on desk research alone, and
must be confirmed with a real local build before Phase 1 depends on them.

No Android, JNI, NDK, Java/Kotlin, Gradle, or host-application reasoning was
used to make any of these decisions. All are justified purely on the needs of
a standalone C++ PPTX parsing/rendering library.

---

## 1. Rendering backend

**Status: PROVISIONAL — recommended, pending local build verification (see
"Required local verification" below). Not yet locked in.**

### Requirements this has to satisfy
Vector path/shape fill+stroke, text rendering, text shaping (complex scripts,
bidi, ligatures), font handling/rasterization, raster image compositing,
2D affine/perspective transforms, acceptable performance and memory footprint
for slide-sized surfaces, portability across Linux/Windows/macOS today, a
plausible (not required-now) path to Android later, license compatible with
commercial redistribution, and bounded build complexity.

### Options evaluated

| Option | Vector+text | License | Build complexity | Portability | Android path |
|---|---|---|---|---|---|
| **Skia** | Full (own rasterizer, GPU backends, bundles its own vendored HarfBuzz+FreeType) | New BSD (permissive) | High — requires Google's `depot_tools`, GN, Ninja, a multi-GB checkout via `git-sync-deps`/`gclient`, and has no stable numbered releases or official prebuilt libs for arbitrary embedding (it's tracked by git revision, not semver) | Officially supports Linux/Windows/macOS/Android/iOS | Best-in-class — it *is* Android's system graphics stack |
| **Cairo + Pango + HarfBuzz + FreeType** | Cairo does vector+raster+transforms; Pango does text layout/shaping orchestration on top of HarfBuzz; FreeType rasterizes glyphs | Cairo: dual LGPL-2.1-only **or** MPL-1.1 (pick one); Pango: LGPL-2.0-or-later; HarfBuzz: MIT; FreeType: dual FTL **or** GPLv2 (pick FTL) | Low-to-moderate — all four are packaged by every major Linux distro and available via vcpkg/Conan/MSYS2/Homebrew; no vendored toolchain, standard CMake/pkg-config/Meson builds | Strong on Linux/macOS/Windows (via MSYS2/vcpkg); weaker path to Android — Pango pulls in GLib and typically expects fontconfig, neither of which is a natural fit on Android | Weak — would need real work later (Pango-on-Android is done by some projects but is not turnkey) |
| **Blend2D** | Vector fill/stroke/transform + JIT-compiled rasterizer; **no built-in text shaping** — still needs HarfBuzz+FreeType glued in | Zlib (permissive); its JIT dependency AsmJit is also Zlib | Low — self-contained CMake build, no vendored toolchain, no GN/Ninja/depot_tools | Explicit X86/ARM64 JIT backends plus a portable non-JIT fallback; used on desktop/server/mobile | Plausible (ARM64 JIT path exists) but far less proven than Skia; smaller community |
| Cairo alone (no Pango) | Vector+raster+transforms only, no shaping | Same as Cairo above | Lower than the full Pango stack | Same as Cairo | Same gap — still needs a shaping layer bolted on |

### Decision (provisional)

**Cairo + Pango + HarfBuzz + FreeType**, as the master prompt's stated
fallback, is the recommended default — *provisionally*, pending the local
verification step below.

Rationale:
- All four have long-lived, permissively-combinable open-source licenses with
  no per-seat/per-document fees and no evaluation-only terms.
- All four build through ordinary package managers and standard build systems
  on every target OS this phase cares about (Linux/Windows/macOS) — no
  bespoke fetch/build toolchain, no multi-GB checkout.
- Skia's *technical* fit is excellent (and its Android story is the best of
  any option, since Skia is literally Android's graphics stack), but its
  build cost is real and specifically flagged as a risk by the master
  prompt: no numbered stable releases to pin against, a mandatory
  `depot_tools`/GN/Ninja toolchain, and a checkout that pulls a large
  dependency tree (Skia vendors its own copies of FreeType, HarfBuzz,
  libpng, libwebp, zlib, etc. — see `third_party/externals/` in Skia's own
  tree). None of that is disqualifying, but it is exactly the kind of cost
  the master prompt says must be *proven survivable*, not assumed.
- Blend2D is a credible, more turnkey alternative to Skia's rasterizer, but
  it does not solve text shaping by itself — HarfBuzz+FreeType would still
  be required alongside it, so choosing it doesn't remove the shaping-layer
  decision, only the vector/raster-drawing one. It's noted here as the
  fallback-of-the-fallback if Cairo's licensing (see below) turns out to be
  a problem for a specific downstream redistribution scenario.

### Licensing note specific to this decision
Cairo is dual-licensed **LGPL-2.1-only OR MPL-1.1** — the integrator picks
one. For a library meant to be linked into arbitrary (including closed-source)
host applications, **MPL-1.1** is the safer pick: it is a file-level copyleft
that does not impose LGPL's "must allow relinking against a different Cairo
version" obligation on statically-linked proprietary consumers. This must be
stated explicitly wherever Cairo's license is declared (see
`THIRD_PARTY_LICENSES.md`). FreeType similarly ships as dual **FTL or GPLv2**
— **FTL** (BSD-style with an attribution clause) is the correct pick for the
same reason; do not accept GPLv2 by default.

### Required local verification (not yet performed by any session on record)
The repository's own `ARCHITECTURE.md` (before this update) and its 1-commit
git history show no evidence any build spike has actually been run. This must
happen before section 1 can move from PROVISIONAL to DECIDED:

1. **Cairo/Pango/HarfBuzz/FreeType "hello triangle + hello glyph" spike**
   (the one to actually try first, matching the recommendation above):
   ```bash
   # Debian/Ubuntu
   sudo apt install libcairo2-dev libpango1.0-dev libharfbuzz-dev libfreetype6-dev pkg-config
   # macOS
   brew install cairo pango harfbuzz freetype pkg-config
   ```
   Then compile a minimal program that (a) creates a Cairo image surface,
   (b) draws a filled rectangle and a stroked path, (c) uses
   `pango_cairo_create_layout` to lay out and render one line of shaped text,
   (d) writes the surface to a PNG, and confirm the PNG actually contains the
   expected shapes and legible text. Record: exact package versions installed,
   compiler/OS, and whether it built/ran without patching.

2. **Skia build-cost spike** (only strictly required if step 1 fails some
   hard requirement — but worth doing once for a real comparison point, since
   the master prompt asks for it):
   ```bash
   mkdir skia_spike && cd skia_spike
   git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
   export PATH="$PWD/depot_tools:$PATH"
   git clone https://skia.googlesource.com/skia.git
   cd skia
   python3 tools/git-sync-deps
   bin/fetch-ninja
   gn gen out/Static --args='is_official_build=true is_debug=false'
   ninja -C out/Static skia
   ```
   Record: wall-clock time, disk space consumed, whether it completed without
   manual intervention, and the exact git revision synced (Skia has no
   version tags to cite instead).

3. Whichever spike is chosen as final gets one paragraph here naming the
   spike result (pass/fail, evidence file/log, git revision or package
   versions) before Phase 1 begins.

---

## 2. XML parser

**Status: DECIDED**

**Decision: pugixml 1.16** (MIT license, released 2026-06-16).

Rationale:
- MIT license, fully compatible with commercial redistribution, no
  attribution-in-UI requirement (a courtesy acknowledgment is suggested by
  upstream but is explicitly optional, not required).
- No external entity or network resolution by default — appropriate for
  parsing untrusted `.pptx` content.
- Lightweight, in-memory DOM with very fast parsing; header/source footprint
  is small enough to vendor directly rather than requiring a system package.
- Actively maintained — 1.16 is a 2026 release ("pugixml turns 20"), not an
  abandoned project.

**Correction to the kickoff prompt's framing:** pugixml is commonly described
as "namespace-aware," but that needs a precise caveat for this project.
pugixml does **not** resolve `xmlns` prefixes to namespace URIs the way a
validating/namespace-URI-resolving parser would — it exposes element/attribute
names as literal strings including whatever prefix was used in the source
(e.g. `a:off`, `p:sp`, `r:embed`). This is actually *fine* for OOXML parsing
in practice, because PowerPoint always emits the same conventional prefixes
(`a:` for DrawingML, `p:` for PresentationML, `r:` for the relationships
namespace, etc.), and the relationship-resolution logic this project needs
(§ Phase 1) works off relationship IDs and part paths, not raw namespace URIs.
But the parser/model code must match on literal qualified names
(`"p:sld"`, `"a:off"`, `"r:embed"`) rather than assuming any namespace-URI
resolution is happening underneath — this should be called out in code
comments where XML node/attribute names are matched, so a future contributor
doesn't assume more correctness than pugixml actually provides.

Rejected alternatives:
- **tinyxml2** (zlib license) — smaller and also fine license-wise, but
  slower and with a less ergonomic traversal API than pugixml for this
  volume of relationship/part parsing; no compelling reason to prefer it
  over pugixml here.
- **libxml2** (MIT, historically dual MIT/custom) — full-featured, does do
  real namespace-URI resolution and DTD/schema validation, but is a much
  heavier dependency (needs libiconv, zlib, sometimes liblzma) for
  capabilities (DTD validation, full XPath 2, streaming SAX) this project
  does not need at this phase. Revisit only if a specific correctness gap
  from pugixml's lack of namespace-URI resolution is actually hit in
  practice.

---

## 3. ZIP/package reader

**Status: DECIDED**

**Decision: libzip 1.11.4** (BSD-3-Clause license).

Rationale:
- BSD-3-Clause, fully compatible with commercial redistribution.
- Supports reading via a custom `zip_source` callback, which is exactly the
  hook needed to back `libzip`'s reads with this project's `IPackageReader`
  abstraction (§ 7) instead of forcing a real filesystem path — even though
  Phase 1 only implements the file-backed reader, this keeps the ZIP layer
  from assuming a POSIX file descriptor.
- Random-access, per-entry reading (`zip_fopen_index` / `zip_fread`) without
  requiring full archive extraction — required by the resource-limit rules
  in Phase 1 (per-entry size caps must be enforceable while streaming, not
  only after full decompression).
- Mature and very widely deployed (it's the library behind PHP's `ZipArchive`,
  among many other consumers), which means path-traversal and
  decompression-bomb classes of bugs have had a long time to surface upstream.
- Minimal mandatory dependency surface: only zlib is required for the
  deflate/store methods this project actually needs; libzip's optional
  bzip2/LZMA/zstd/OpenSSL support (for encryption and less-common compression
  methods) can be left disabled since valid `.pptx` packages use only
  Store or Deflate.

Rejected alternative:
- **minizip-ng 4.2.1** (Zlib license) — also acceptable on licensing grounds
  and technically capable (it added an IO-buffering layer and custom stream
  support), but its feature set (disk splitting, WinZip AES encryption,
  legacy codepage support) is aimed at general-purpose zip creation/editing
  that this read-mostly, OOXML-specific use case doesn't need, and it pulls
  in a larger optional-dependency surface (bzip2, xz, zstd, openssl) by
  default in most package-manager builds. libzip's narrower, more mature API
  is a better fit for "read one specific kind of zip safely."

---

## 4. Error-handling model

**Status: DECIDED**

**Decision: `Result<T, Error>`, not exceptions.**

```cpp
// errors.h (skeleton)
#pragma once
#include <string>
#include <optional>

namespace pptxlib {

enum class ErrorCode {
    InvalidPackage,
    InvalidXml,
    MissingPart,
    InvalidRelationship,
    ResourceLimitExceeded,
    Unsupported,
    IoError,
};

struct Error {
    ErrorCode code;
    std::string message;                 // human-readable
    std::optional<std::string> context;  // e.g. offending part path
};

// Result<T, Error> is provided by a small vendored implementation
// (or std::expected under C++23 if the minimum standard is raised later).
template <typename T>
class Result; // = Ok(T) | Err(Error)

} // namespace pptxlib
```

Rationale: this is a reusable systems library with no single owning
application to decide a global exception policy; explicit, inspectable
`Result<T, Error>` return values give every caller predictable, structured
failure handling without forcing exception-based control flow (or the
binary-size and unwind-table costs of exceptions) on consumers who don't want
them. This also composes cleanly with the "lenient by default" parsing
principle (§ 6): most `.pptx` irregularities become `Ok` with a slightly
degraded model, not an `Err`, and the few things that *are* hard failures map
directly onto specific `ErrorCode` values instead of generic exception types.

No JNI/Android reasoning informed this choice — it follows purely from
"reusable C++ systems library, unknown caller exception policy."

---

## 5. Threading model

**Status: DECIDED**

- The parsed presentation model becomes **immutable** after parsing:
  `Package::open()` (and later, presentation parsing) produces a
  `std::shared_ptr<const Presentation>` handed out to callers. Parse once →
  immutable model → any number of concurrent readers.
- Because the model is immutable and read-only after construction,
  `shared_ptr`'s built-in atomic reference counting is sufficient — **no
  additional mutexes are added around data nothing ever mutates.**
- Resource caches (image, font, thumbnail, rendered-slide) are explicitly a
  **separate, later concern**, each to be protected independently (sharded or
  striped locks, never one global lock) once they exist in a rendering phase.
- **Phase 1 implements no cache and no thread pool** — there is no rendering
  yet for either to serve, and adding them now would be unvalidated
  complexity with nothing exercising it.
- No global mutable state is introduced anywhere in the package/parsing layer.

---

## 6. Parsing leniency principle

**Status: DECIDED**

"Lenient by default": real-world `.pptx` files are frequently valid-enough-
for-PowerPoint but not strictly spec-conformant, so the parser must not treat
every deviation as fatal.

- Unknown XML elements and attributes are skipped, not treated as errors.
- Missing *optional* relationships fall back to documented sane defaults
  rather than failing the whole parse.
- Only these are hard failures, surfaced as a specific `Error`:
  - the ZIP container itself is unreadable/corrupt (`InvalidPackage`),
  - a required part is missing — `[Content_Types].xml` or
    `ppt/presentation.xml` (`MissingPart`),
  - the core relationship parts are malformed such that the model cannot be
    built at all (`InvalidRelationship`).

This keeps the library usable against the long tail of real files produced by
PowerPoint, Keynote-exported-as-pptx, Google Slides exports, and older Office
versions, none of which are guaranteed to be byte-perfect against the ECMA-376
spec.

---

## 7. Loading model

**Status: DECIDED**

`Package::open` is designed around an abstract, generic reader interface —
not a platform-specific one — so future input sources (in-memory buffers,
streams, a host-provided content URI) can be added later without redesigning
the parser or the ZIP layer.

```cpp
// io/package_reader.h (interface skeleton)
class IPackageReader {
public:
    virtual ~IPackageReader() = default;
    virtual Result<size_t> Read(uint64_t offset, void* buffer, size_t size) = 0;
    virtual Result<uint64_t> Size() const = 0;
};
```

Phase 1 implements **only** the file-path-backed version
(`FilePackageReader`) on top of this interface — no stream reader, no
in-memory buffer reader, and no platform-specific (Android `AssetManager`,
content-URI, etc.) reader yet. The interface is described purely in terms of
"a byte-range-readable source," with no reference to any specific future host
or platform, consistent with the standalone-library rule for this project.

---

## 8. Public API boundary

**Status: DECIDED — hard rule, enforced by code review**

Third-party types must never leak into public headers or the model:
not `SkCanvas*`/`SkPaint` (if Skia is ultimately chosen in § 1), not
`cairo_t*`/`PangoLayout*` (if Cairo/Pango is chosen), not `hb_buffer_t*`,
`FT_Face`, `pugi::xml_node`, `zip_file_t*`, or any equivalent third-party
handle or pointer.

The public surface uses only library-owned abstractions: `Canvas`,
`RenderTarget`, `Paint`, `Path`, `Font`, `Image`, `Matrix`. The
parser/model layer stays entirely independent of whichever rendering backend
is chosen, and of pugixml/libzip's own types once past the parsing boundary —
those only appear inside the relevant implementation (`.cpp`) files, never in
`include/pptxlib/`.

- Bad: `struct Slide { SkRect rect; SkPaint paint; };`
- Good: `struct Slide { SlideId id; PackagePartPath partPath; };`

This rule is independent of which rendering backend § 1 ultimately locks in —
it holds regardless.

---

## 9. Dependency licensing

**Status: DECIDED for Phase 0/1 dependencies (pugixml, libzip); PROVISIONAL
for the rendering stack, pending § 1** — see `THIRD_PARTY_LICENSES.md` for
the full per-dependency record (exact version, license, source, redistribution
notes, rejected alternatives).

No dependency under consideration carries per-user royalties, per-document
fees, mandatory commercial licensing, or evaluation-only restrictions. The one
point requiring an explicit choice rather than passive acceptance is Cairo's
dual LGPL-2.1/MPL-1.1 licensing and FreeType's dual FTL/GPLv2 licensing — see
§ 1's licensing note. Both must be locked to the MPL-1.1 / FTL side of their
respective choices before any code linking against them ships.
