# pptxlib

A standalone C++ library for parsing and (eventually) rendering `.pptx` files.

## Where things live

- `docs/Standalone_CPP_PPTX_Library_Master_Prompt.txt` — the full project
  architecture and phased roadmap (Phase 0 through Phase 8). Standing
  reference for overall scope.
- `docs/Phase0-1_Kickoff_Prompt.md` — the active work order. **Start here.**
  Scopes this stage of work to Phase 0 (binding architecture decisions) and
  Phase 1 (package parsing + slide enumeration) only.
- `ARCHITECTURE.md` — where Phase 0 decisions get recorded. Currently all
  9 sections are unfilled; that's the first thing to change.
- `THIRD_PARTY_LICENSES.md` — dependency/license record, filled in as
  Phase 0 picks libraries.
- `include/pptxlib/`, `src/` — public headers and implementation. Empty
  until Phase 1 starts writing code.
- `tests/` — unit tests, CMake-integrated via CTest.

## How to start

1. Open this repo in an agentic coding session (Claude Code or similar) and
   point it at `docs/Phase0-1_Kickoff_Prompt.md`.
2. It should read the master prompt for context, then work through the 9
   Phase 0 decisions in `ARCHITECTURE.md` — including actually spiking the
   Skia build before committing to it, not just picking it on paper.
3. Only once `ARCHITECTURE.md` is fully filled in does Phase 1 implementation
   begin, against real `.pptx` sample files.
4. After Phase 1's acceptance criteria pass, the agent stops and reports —
   Phase 2 (rendering) is a separate, deliberate next session.

## Build (once there's something to build)

```
cmake -S . -B build -DPPTXLIB_ENABLE_SANITIZERS=ON
cmake --build build
ctest --test-dir build
```
