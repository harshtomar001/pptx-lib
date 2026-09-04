# PHASE 0/1 KICKOFF — STANDALONE C++ PPTX LIBRARY

## CONTEXT

This session works from the full master architecture prompt (`Standalone_CPP_PPTX_Library_Master_Prompt.txt`), which remains the standing reference for the project's overall scope, layered architecture, and phased roadmap. This project is a **pure, standalone C++ library**. No Android, JNI, NDK, Java/Kotlin, Gradle, or PDF Studio code or reasoning belongs anywhere in this phase — not as implementation, and not as justification for a design decision. Any future host application is a later concern and must not shape the public C++ API now.

**This session's scope is Phase 0 (binding decisions) and Phase 1 (package + presentation parsing) ONLY.** Do not implement rendering, text layout, themes, tables, or charts in this session — those are later phases. The goal is a small, real, tested slice: open a `.pptx`, resolve relationships correctly, and enumerate slides.

If the repository already contains a correct implementation of any piece described below, reuse it rather than rebuilding it unnecessarily.

---

## PHASE 0 — DECISIONS REQUIRED BEFORE WRITING ANY CODE

The master spec deferred several choices to "Phase 0 research." Do not leave them as open bullet points — resolve each one and write the rationale into `ARCHITECTURE.md` before any implementation commit.

### 1. Rendering backend
Recommended default direction, to be confirmed (not assumed) in Phase 0:
- **Graphics:** Skia
- **Text shaping:** HarfBuzz
- **Font handling/rasterization:** FreeType
- Pipeline: PowerPoint model → PowerPoint-specific layout engine → our rendering abstraction → Skia. Text: PowerPoint text → our text-layout layer → HarfBuzz (shaping) → glyph positioning → FreeType/Skia (font handling) → Skia.
- **Before locking this in, spike it.** Skia in particular is one of the more painful C++ dependencies to stand up from source (its own build tooling — `depot_tools`/GN/Ninja — a large checkout, and real portability quirks across Linux/Windows/macOS). Do not assume it will build cleanly; actually build a minimal "hello triangle" against it in Phase 0. If the build cost is unworkable, the honest fallback is Cairo + Pango + HarfBuzz + FreeType, which is far more turnkey via standard package managers at the cost of more manual work for gradients/effects.
- Verify current license, version, build complexity, and portability for each of the three before locking in — do not treat "it's the industry-standard choice" as a substitute for actually checking.
- Deliverable: one paragraph in `ARCHITECTURE.md` naming the final choice, the spike result, and why.

### 2. XML parser
- Evaluate **pugixml** first: namespace-aware, permissively licensed (MIT), no external entity/network loading by default, lightweight.
- Only look further if pugixml fails a hard requirement.
- Deliverable: decision + rationale in `ARCHITECTURE.md`.

### 3. ZIP/package reader
- Needs random-access reading without full extraction, and enforceable size/entry limits at the API boundary.
- Candidates: libzip, minizip-ng.
- Deliverable: decision + rationale in `ARCHITECTURE.md`.

### 4. Error-handling model
- Choose **`Result<T, Error>`**, not exceptions. Rationale: this is a reusable systems library, and explicit structured errors give predictable, inspectable failure handling without forcing every caller into exception-based control flow. (Do not justify this with JNI/Android reasoning — see the standalone-library rule above.)
- Define `Error` as a struct with at minimum: an error code enum (`InvalidPackage`, `InvalidXml`, `MissingPart`, `InvalidRelationship`, `ResourceLimitExceeded`, etc.), a human-readable message, and optional context (e.g. the offending part name).
- Deliverable: `errors.h` skeleton + one paragraph in `ARCHITECTURE.md` explaining the choice.

### 5. Concurrency & caching strategy
The master spec requires both thread-safe concurrent rendering *and* bounded LRU caches — these collide if left vague. Adopt this concrete strategy:
- The parsed presentation model becomes **immutable** after parsing (`shared_ptr<const Model>` handed out to callers): parse once → immutable model → multiple concurrent readers → future concurrent slide rendering. Avoid global mutable state.
- Immutable, read-only data does not need locking beyond what `shared_ptr`'s reference counting already gives you — don't add defensive mutexes around data nothing ever mutates.
- Resource caches (image, font, thumbnail, rendered-slide) are a *separate*, later concern — each would be protected independently (sharded/striped locks or per-cache fine-grained locking, never one global lock) once they exist. **Do not implement a rendering cache or thread pool in Phase 1** — there's no rendering yet for either to serve.
- Deliverable: a "Threading Model" section in `ARCHITECTURE.md` stating this explicitly.

### 6. Parsing leniency principle
Real-world `.pptx` files are often valid-enough-for-PowerPoint but not strictly spec-conformant. Adopt "lenient by default":
- Unknown elements/attributes are skipped, not fatal.
- Missing optional relationships fall back to sane defaults.
- Only these are hard failures: unreadable ZIP, missing required parts (`presentation.xml`, `[Content_Types].xml`), malformed core relationships.
- Deliverable: one paragraph in `ARCHITECTURE.md`.

### 7. Loading model
Design `Package::open` around an abstract, generic reader interface — not a platform-specific one — so other input sources can be added later without redesigning the parser.
- Define `IPackageReader` (read/seek) now, described in general terms (a byte-range-readable source), not in terms of any specific future host or platform.
- Implement **only** the file-path-backed version in Phase 1. Nothing beyond that — no stream, buffer, or platform-specific reader yet.
- Deliverable: interface header + file-backed implementation.

### 8. Public API boundary
Third-party types must never leak into public headers or the model. Not `SkCanvas*`, `SkPaint`, `hb_buffer_t*`, `FT_Face`, `pugi::xml_node`, `zip_file_t*`, or anything equivalent. The public surface uses only library-owned abstractions: `Canvas`, `RenderTarget`, `Paint`, `Path`, `Font`, `Image`, `Matrix`. The parser/model stays independent of Skia, HarfBuzz, and FreeType entirely — those only appear inside the rendering backend's implementation files.
- Example — bad: `struct Slide { SkRect rect; SkPaint paint; };`
- Example — good: `struct Slide { SlideId id; PackagePartPath partPath; };`
- Deliverable: stated as a hard rule in `ARCHITECTURE.md`; enforced by code review, not just intention.

### 9. Dependency licensing
For every dependency actually selected, record in `THIRD_PARTY_LICENSES.md`: library, exact version, license, source, why it was selected, and commercial-redistribution compatibility — based on the actual versions being built, not generic assumptions about the project. Do not introduce a dependency with per-user royalties, per-document fees, mandatory commercial licensing, evaluation-only restrictions, or any other incompatible licensing obligation.

**Phase 0 is complete when `ARCHITECTURE.md` documents all nine decisions with rationale — before the first line of Phase 1 code is written.**

---

## PHASE 1 — IMPLEMENTATION SCOPE

### Implement
- `IPackageReader` interface + file-path-backed implementation, using the ZIP library chosen in Phase 0.
- ZIP/package-layer safety limits, all **configurable**, enforced against untrusted input: max uncompressed size, max individual entry size, max entry count, path-traversal guard, decompression-bomb guard, max XML document size. Violations return `ResourceLimitExceeded` / `InvalidPackage` — never a crash.
- `[Content_Types].xml` parsing.
- A **generic, reusable** relationship parser that operates over any part's `_rels`, not one hardcoded to `presentation.xml` — it needs to resolve presentation→slide, slide→layout, layout→master, master→theme, and slide→media relationships alike. **Never assume relationship IDs follow a numbering scheme** (e.g. never assume `rId1` = `slide1.xml`) — always resolve the actual target through the relationship data.
- `presentation.xml` parsing: slide list resolved via relationships (never filename guessing or assumed numbering), slide dimensions.
- Enumerate `ppt/fonts/` embedded font parts — list only, extraction deferred to a later phase, but the capability exists in the model from day one.
- A minimal normalized model: `Presentation`, and a `Slide` stub holding only its id and resolved part path — no shape/text parsing yet, and no rendering-engine types anywhere in it (see Phase 0 §8).
- All fallible operations return the `Result<T, Error>` type from Phase 0.

### Explicitly OUT of scope this phase
Skia/HarfBuzz/FreeType integration, text rendering, text shaping, shape rendering, themes, complete slide masters, layout inheritance, tables, charts, image rendering, animations, transitions, font extraction beyond enumeration.

### Acceptance criteria (must pass against real files, not just compile)
- Given a real sample `.pptx`, `Package::open()` succeeds and `presentation.slideCount()` returns the correct count.
- Given a corrupted/truncated ZIP, `open()` returns `InvalidPackage` — does not throw, does not crash.
- Given a crafted test fixture with a path-traversal ZIP entry, the ZIP layer rejects it.
- Unit tests exist for: ZIP reader, relationship parser, `presentation.xml` parsing, and at least one corrupted-file case.
- `CMake` build succeeds cleanly with `-Wall -Wextra`; sanitizers (ASan/UBSan) are clean on the test binary.

### End-of-phase report (hard stop — do not continue into Phase 2)
When Phase 1 is implemented and all tests pass, **stop** and report: what changed, files created/modified, dependencies and exact versions, licenses, tests added, sanitizer results, security-test results, build results, known limitations, and recommended next phase. Wait for explicit instruction before implementing anything from Phase 2 (rendering).
