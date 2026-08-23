# Testing Strategy

## Current state

No automated test project or test framework is present. The main application is tied to BDS 2006, VCL, Borland extensions (`__fastcall`, `PACKAGE`, form resources), and Win32 APIs. The audit environment lacks the required compilers, so no baseline build or executable test was possible.

The practical first framework is a minimal native C++ console test executable compatible with the compiler used for the production source, using a small assertion harness and binary fixtures. This avoids imposing a modern framework that BDS 2006 may not compile. In parallel, new portable analysis code may use a current C++ test runner only after the unchanged legacy build is reproducible and a compiler compatibility boundary is documented.

## Test layers

| Layer | Runs without hardware/admin | Purpose |
|---|---:|---|
| Binary fixture tests | Yes | Parse known EDID bytes and assert semantic fields. |
| Round-trip tests | Yes | Parse → serialize and compare bytes/checksums, including unknown data. |
| Pure timing tests | Yes | Exercise timing snapshots, calculations, validation, and boundary values. |
| Registry adapter tests | Yes, with fake/in-memory adapter | Verify planned writes, deletion, errors, and rollback without touching HKLM. |
| Build smoke tests | Yes | Compile Debug/Release with the archived legacy toolchain and launch with registry writes disabled. |
| Hardware lab tests | No | Validate real AMD/NVIDIA/other GPUs, HDMI/DP paths, restart, recovery, and rollback. |

## EDID fixture matrix

- Valid base EDID with zero and multiple extension blocks.
- Base detailed, standard, and established timings.
- Correct and incorrect checksums for each 128-byte block.
- Declared extension count smaller/larger than available data.
- Truncated files and malformed data-block lengths.
- Unknown CEA extended tags, vendor blocks, and DisplayID blocks that must survive unchanged.
- Import/export of `.bin`, `.dat`, and monitor-INF paths supported by `DisplayClass`.

For round trips, distinguish two expectations: exact byte preservation for an unmodified document, and canonical expected bytes for an intentionally edited supported field. A checksum-only assertion is insufficient.

## Timing matrix

- EDID 18-byte detailed timings and DisplayID detailed timings.
- Progressive and interlaced modes.
- Manual timing at minimum/maximum legal field and clock values.
- Existing automatic algorithms: LCD standard, LCD native, LCD reduced, CRT standard, and old standard.
- Standards claimed later: CVT, CVT-RB, and CVT-RB2 only after the existing formulas are identified against published reference vectors.
- Derived horizontal rate, vertical rate, totals, blanking, and pixel clock with explicit rounding assertions.

## TimingSnapshot tests

`tests/TimingSnapshotTest.cpp` is a dependency-free C++ console test. It uses a getter-compatible fixture type so the header-only `TimingSnapshotClass` can be compiled without VCL or BDS 2006. The fixture values cover 1920×1080 at 60 and 144 Hz, 2560×1440 at 144 and 240 Hz, CRU's automatic LCD-reduced mode, and 1920×1080 interlaced at 60 Hz. The 1080p/i values use CTA timings; high-refresh and reduced values are characterization vectors produced by the current CRU CVT-RB fallback and integer rounding formulas.

The test verifies every copied property and then mutates the source fixture to prove the snapshot owns its values. It does not reproduce timing mathematics: expected totals, blanking, rates, and clocks are fixed characterization values, while production construction calls the existing `DetailedResolutionClass` getters. This test is intentionally portable and requires only a C++ compiler; the real-class fixture below supplies the BDS integration coverage.

`tests/TimingSnapshotIntegrationTest.cpp` adds that real-class fixture without changing the original BDS project. Each fixed 18-byte descriptor is passed to a type-0 `DetailedResolutionClass`; its existing `Read` and `Init` methods decode fields and derive back porches, totals, and rates before `TimingSnapshotClass` copies the results. Expected values are fixed constants, not independently recalculated by the test. Coverage includes CTA 1920×1080p60, CRU CVT-RB 1920×1080p144, CRU CVT-RB 2560×1440p144, CRU CVT-RB 1920×1080p60, and CTA 1920×1080i60. All parsed EDID descriptors correctly retain timing type 0 (`Manual`); the DTD itself does not encode CRU's UI calculation preset.

The fixture is designed for a BDS 2006 Win32 console target that compiles the test together with the production `DetailedResolutionClass.cpp`. BDS is unavailable in the current environment. A local MSVC attempt reached the real production source but could not compile its classic C++Builder pointer-to-member initializer syntax, confirming that MSVC is not an equivalent execution path without source changes.

The requested 2560×1440p240 case cannot be represented by an 18-byte EDID DTD: its 16-bit 10 kHz pixel-clock field tops out at 655.35 MHz, while active pixels alone at that mode require 884.736 MHz before blanking. A future DisplayID type-1 integration fixture can cover this mode without falsifying an EDID DTD.

## CEA/CTA matrix

- Video and YCbCr 4:2:0 video blocks.
- Audio and speaker allocation.
- HDMI VSDB and HDMI Forum VSDB/TMDS fields.
- AMD FreeSync vendor block: min/max, reversed range, truncation, and unrelated vendor OUI.
- Colorimetry and video capability blocks.
- HDR static and dynamic metadata recognition and byte preservation; semantic tests when typed decoders are introduced.
- Data collection overflow and maximum 32-byte block handling.

## DisplayID matrix

- Valid extension header and payload length.
- Detailed timing and tiled-display topology typed blocks.
- Recognized-but-uneditable and unknown blocks.
- Truncated block header/payload, invalid length, serialization capacity, and checksum.

## Safety matrix

- Export original active EDID before any proposed change.
- Render a registry change plan without applying it.
- Simulate access denied, key creation failure, and failure after one extension value is written.
- Verify rollback restores the exact backup, not a regenerated approximation.
- Verify invalid timing/configuration is rejected before persistence.
- In a disposable hardware VM/lab: apply, restart, confirmation timeout, F8 recovery, and `reset-all` recovery.

## Baseline build procedure

1. Provision an isolated Windows VM with licensed BDS 2006/C++Builder (BDS 4.0), required Win32 SDK headers/libraries, and no source modifications.
2. Open `CRU/CRU.bdsproj`; build Debug and Release for Win32. Record compiler/linker versions, full logs, output hashes, and runtime dependencies.
3. Build `RESTART-DISPLAY-DRIVER/restart.vcxproj` for Win32/x64 with the Windows 7.1 SDK-compatible toolset; confirm `restart.exe` can locate `restart64.exe` on 64-bit Windows.
4. Build `RESET-ALL/reset-all.sln` in its declared configurations.
5. Perform read-only launch/import/export smoke tests in a snapshot-enabled VM. Do not press OK against a physical display during baseline verification.

## Current blocking error and solution

The audit machine returns “Could not find files for the given pattern(s)” for `bcc32`, BDS, MSBuild, Visual Studio (`devenv`/`cl`), and `dotnet`. Root cause: none of the required legacy or helper build toolchains is installed or on `PATH`.

Possible solutions are: install BDS 2006 in a VM; attempt import into a newer C++Builder after preserving the original build; or port to MSVC/another compiler. The recommended solution is the first option for milestone one. Import/port work changes the build and potentially language/runtime behavior, so it should follow a verified unchanged baseline.
