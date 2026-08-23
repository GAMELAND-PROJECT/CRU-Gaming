# Testing Strategy

## Current state

The legacy application remains tied to BDS 2006, VCL, Borland extensions (`__fastcall`, `PACKAGE`, form resources), and Win32 APIs. BDS is not installed, so the unchanged legacy executable still cannot be built in this environment. The independent Portable Core has an MSVC 2022 solution and a dependency-free console test executable.

The practical first framework is a minimal native C++ console test executable compatible with the compiler used for the production source, using a small assertion harness and binary fixtures. This avoids imposing a modern framework that BDS 2006 may not compile. In parallel, new portable analysis code may use a current C++ test runner only after the unchanged legacy build is reproducible and a compiler compatibility boundary is documented.

## Portable Core build and test

Open `PortableCore.sln` in Visual Studio 2022 or build it with MSBuild. The projects use toolset `v143`, C++17, warning level 4, warnings as errors, no precompiled header, and support Debug/Release on Win32/x64.

```text
MSBuild PortableCore.sln /m /t:Build /p:Configuration=Debug /p:Platform=x64
x64\Debug\PortableCoreTests.exe
```

The baseline implementation was built and executed successfully in all four configuration/platform combinations with zero warnings and zero errors. Each run passed five fixed real DTD fixtures (CTA 1080p60, CRU CVT-RB 1080p144, CRU CVT-RB 1440p144, CRU CVT-RB 1080p60, CTA 1080i60) plus invalid zero-clock and invalid-blanking inputs.

The test invokes only `DetailedTimingDescriptor::parse`; expected values are fixed constants derived from the legacy characterization fixtures. It verifies every timing component, explicit pixel-clock/rate units, polarity, scan mode, manual timing type, and `ReducedBlanking::Unknown`. Reduced blanking is intentionally unknown because the 18-byte DTD does not encode which formula produced its numbers.

The same executable also exercises `EdidBaseBlockParser` with a valid 128-byte base block containing a real 1080p60 DTD. It verifies the EDID header, checksum, manufacturer/product identifiers, version/revision, extension count, DTD extraction, and rejection of independently checksum-correct bad headers and bad checksums. No test writes an EDID or accesses a monitor.

CTA structural coverage uses a revision-3 extension containing a Video Data Block and a 1080p60 DTD. Tests assert header flags, raw tag/payload preservation, DTD extraction, and rejection of a wrong extension tag, bad checksum, and a data-block length that crosses the declared DTD offset.

CTA data-block view coverage checks typed Video, Extended, and reserved classification, Extended Tag extraction, empty Extended Data Block handling, and zero-copy access to the original payload.

CTA Video Data Block coverage decodes that real parser output into SVD entries and checks VIC/native handling. It also verifies CRU-compatible treatment of a native VIC in the 1-64 range, a full-byte extended VIC, and rejection of a non-video data block.

CTA VIC catalog coverage checks progressive, interlaced, UHD, and extended VIC records against the existing Legacy `TVResolutionClass::Codes` values. It also covers a known-but-not-supported Legacy entry and unknown/reserved lookup failures.

Advertised CTA video-mode coverage exercises the complete parsed extension to Video Data Block to SVD to VIC catalog path. Direct fixtures also verify native and extended VIC handling, preservation of an unknown VIC without fabricated metadata, and rejection of a non-video block.

Full EDID document coverage uses a two-block fixture with one base DTD and one CTA extension containing both a Video Data Block and a DTD. Tests verify aggregated timings and modes, exact declared block-count enforcement, extension checksum rejection, and checksum-valid preservation of an unknown extension type.

Display capability snapshot coverage verifies copied identity, extension counts, DTDs, and advertised modes. The source `EdidDocument` collections are then cleared to prove that an existing snapshot owns its data and cannot be changed through later source mutation. `EdidInspect` consumes this snapshot in its executable smoke-test path.

Timing analyzer coverage runs every valid DTD fixture through structural consistency checks. A fixed 1080p60 case verifies derived pixel counts, active-pixel ratio, horizontal rate, and refresh rate; a deliberately malformed snapshot verifies reporting of zero clock and inconsistent horizontal/vertical components without touching parser behavior.

Display timing report coverage verifies aggregation of two valid document timings, then adds one deliberately malformed snapshot and checks the resulting 2-consistent/1-inconsistent summary. The CLI smoke test also verifies its displayed aggregate counts.

Display mode inventory coverage combines duplicate 1080p60 DTD/CTA entries with a 1080p50 CTA mode, verifies two unique modes and a 50-60 Hz progressive resolution summary, checks DTD/CTA source flags, and confirms an unknown VIC is counted without fabricated resolution metadata.

Monitor range-limit coverage injects a valid EDID 1.4 `0xFD` descriptor and verifies 48-144 Hz vertical, 30-240 kHz horizontal, 600 MHz maximum pixel clock, and secondary-formula decoding. A direct fixture verifies all four EDID 1.4 extended-offset bits. The values remain explicitly advertised limits, not overclocking conclusions.

Range capability estimator coverage uses fixed 1080p totals to verify independent horizontal-scan and pixel-clock ceilings. One fixture is horizontal-limited and exceeds the advertised vertical maximum; another is pixel-clock-limited below it. Tests verify both the mathematical estimate and the separately EDID-bounded result, without generating or applying a timing.

`EdidInspect` is built as a third project in `PortableCore.sln`. Run `EdidInspect <edid.bin>` for text or `EdidInspect --json <edid.bin>` for schema-versioned JSON. `tests/RunEdidInspectTests.ps1` converts the checked-in hex fixture to temporary binary files and verifies text and parsed JSON for base-only and CTA EDIDs, plus checksum and invalid-size rejection. The smoke test passes for Debug/Release on Win32/x64; parser correctness is also tested directly by `PortableCoreTests`.

For normal local validation, `tools/BuildPortableCore.ps1 -Configuration Debug -Platform x64` discovers MSBuild through Visual Studio Installer, builds the selected configuration, runs `PortableCoreTests`, and runs the `EdidInspect` smoke tests. It never installs or retargets a toolchain. `tools/InspectEdid.ps1` provides a stable wrapper for text output or saving a JSON report.

`EdidViewer` is built in every solution configuration with warnings treated as errors. Its parsing and report logic is the same tested Portable Core path used by the CLI. The GUI acceptance check is manual: launch it, verify the connected-monitor selector is populated when Windows exposes readable EDIDs, switch monitors, use Refresh, choose a binary EDID, resize the window, and verify an invalid file produces an error. Discovery opens only the monitor device registry key with `KEY_READ`; no display state is changed.

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
