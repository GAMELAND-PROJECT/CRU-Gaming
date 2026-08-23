# Current CRU Architecture

## Scope and audit basis

This document describes the code in this repository as it exists. It does not describe the proposed CRU Gaming target architecture. The main application is legacy C++Builder/VCL code, with parsing, serialization, Windows integration, and UI concerns coupled in the same executable.

The repository now also contains an independent Portable Core. This does not replace or alter the legacy application:

```text
CRU Legacy (behavioral reference)
        ↓ fixed characterization fixtures
Portable Core (standard C++17)
        ↓
MSVC 2022 / platform-independent library
```

`PortableCore.sln` builds `core/PortableCore.vcxproj` and `tests/PortableCoreTests/PortableCoreTests.vcxproj`. The core has no VCL, C++Builder, Borland RTL, Win32, Registry, GPU vendor, GUI, serializer, display-driver, or hardware dependency. Its current scope is a read-only parser for the standard 18-byte EDID detailed timing descriptor and an explicitly-unitized timing value object.

## Build status

| Field | Finding |
|---|---|
| Build | `CRU/CRU.bdsproj` (main application); separate Visual C++ projects for `restart` and `reset-all` |
| Compiler | Borland C++Builder classic Win32 compiler (`bcc32`) and VCL; helper projects use MSVC/Windows SDK |
| Version | Borland Developer Studio 2006 / BDS 4.0, inferred from `vcl100.csm` and explicit `Borland\\BDS\\4.0` package paths |
| Platform | Main application: Win32 only. Helpers define Win32 and x64 configurations; `restart.vcxproj` selects `Windows7.1SDK` for release builds. |
| Result | Not built in the audit environment: `bcc32`, BDS, MSBuild, Visual Studio, and .NET SDK are not installed. This is a missing-toolchain failure, not a source-code failure. |

The project statically links the C++ runtime and VCL (`dynamicrtl=0`, `use_packages=0`) through `vcl.lib`, `rtl.lib`, `import32.lib`, `cp32mti.lib`, `memmgr.lib`, and `sysinit.obj`. It uses Windows headers/libraries for registry, SetupAPI, user interface, and driver control. AMD ADL and NVIDIA NVAPI are loaded dynamically, so their DLLs are optional at application startup. The manifest requests elevation because monitor overrides live under `HKLM`.

Recommended reproduction environment: a Windows VM with an archived, licensed BDS 2006/C++Builder installation and its Win32 Platform SDK integration. First open and build `CRU.bdsproj` unchanged in Debug and Release. Build helpers separately with a Visual Studio/SDK setup capable of the Windows 7.1 SDK toolset, or retarget a copy of those projects only after recording the original build result.

## Application and data flow

```text
CRU.cpp / WinMain
        ↓
TDisplayForm (DisplayFormClass.cpp/.dfm)
        ↓
DisplayListClass enumerates HKLM Enum\\DISPLAY
        ↓
DisplayClass loads monitor identity, active EDID, and EDID_OVERRIDE
        ↓
AMD ADL / NVIDIA NVAPI optionally supply active full EDIDs
        ↓
DisplayClass parses base EDID and ExtensionBlockClass parses extensions
        ↓
VCL forms edit list/model objects backed by raw byte slots
        ↓
DisplayClass serializes base/extension data and recalculates checksums
        ↓
EDID_OVERRIDE values are created, changed, or deleted in HKLM
        ↓
Windows/graphics driver consumes the override after driver restart/reboot
```

`CRU.cpp` is the entry point. `WinMain` initializes VCL, creates the global `TDisplayForm`, and runs the message loop. `TDisplayForm` owns a `DisplayListClass`, populates controls from the current `DisplayClass`, launches specialized editor forms, and calls `DisplayList.Save()` when OK is pressed.

`DisplayListClass::LoadDisplays()` enumerates `SYSTEM\\CurrentControlSet\\Enum\\DISPLAY`. Each monitor instance becomes a `DisplayClass`. `DisplayClass::Load()` reads the device name, present/active status, the installed `Device Parameters\\EDID`, and numbered blocks below `Device Parameters\\EDID_OVERRIDE`. `AMDDisplayClass` loads `atiadlxx.dll`/`atiadlxy.dll` and calls ADL; `NVIDIADisplayClass` loads `nvapi.dll` and calls NVAPI. Their full active EDIDs are collected in `EDIDListClass` and matched back to registry displays. These vendor paths augment acquisition; they are not general GPU capability backends.

`DisplayClass` keeps three raw buffers (`ActiveData`, `OverrideData`, and `ResetData`) and parallel `PropertiesClass` states. It parses the base block into detailed, standard, and established resolution lists and parses extension blocks through `ExtensionBlockListClass`/`ExtensionBlockClass`. Editing is mostly copy-edit-commit: forms receive a model or list entry, validate it, then copy changed bytes/state back.

The first read-only core seam is `TimingSnapshotClass`:

```text
DetailedResolutionClass
        ↓ copies through existing getters
TimingSnapshotClass
        ↓ future read-only consumer
Timing Analyzer
```

`TimingSnapshotClass.h` is a VCL-free, header-only value abstraction. Construction copies the timing mode, horizontal and vertical components/totals, polarity, CRU rate/clock values, scan mode, and native flag. It exposes only const getters and owns no pointer or reference to the source, so later changes to `DetailedResolutionClass` cannot change an existing snapshot. It deliberately reuses CRU's `GetHBlank`, `GetHTotal`, `GetVBlank`, `GetVTotal`, `GetVRate`, `GetHRate`, and `GetPClock` results rather than duplicating timing calculations. Rates retain CRU's units: vertical millihertz, horizontal hertz, and pixel clock units of 10 kHz. For interlaced timings the vertical values retain CRU's per-field representation.

On save, `DisplayClass::DisplayWrite()` writes detailed, standard, established, and extension data into `OverrideData`; properties are written; checksums are recalculated; and `SaveOverrideData()` writes base block value `0` plus numbered 128-byte extension values under `EDID_OVERRIDE`. Deletion removes that key and related EDID value. Windows does not consume the in-memory model directly—it consumes these registry bytes after display-driver re-enumeration.

The main CRU executable does not restart the driver itself. `RESTART-DISPLAY-DRIVER/restart.c` is a separate recovery-aware utility. It stops/starts display devices, resets graphics configuration/connectivity caches, and temporarily renames `EDID_OVERRIDE` to `EDID_RECOVERY` during recovery. `RESET-ALL/reset-all/reset-all.c` deletes all overrides and graphics configuration caches. Both are privileged and hardware-impacting.

## Major subsystems

| Subsystem | Source files | Responsibility and public interface | Dependencies / structures | Risk and refactoring note |
|---|---|---|---|---|
| Startup | `CRU.cpp`, `Manifest.rc`, `Manifest.xml` | VCL initialization and main-form creation | VCL, elevated manifest | Low logic risk; leave unchanged initially. |
| Main UI | `DisplayFormClass.cpp/.h/.dfm` | Display selection, edit commands, import/export, final save | `DisplayListClass`, all editor forms | High coupling; add new UI beside existing handlers, not inside save logic. |
| Display inventory | `DisplayListClass.cpp/.h` | `Load`, registry enumeration, sort/current display, `Save` | Win32 registry; vector of `DisplayClass*` | Hardware/registry sensitive. A future read-only inventory facade can wrap it. |
| Display/EDID aggregate | `DisplayClass.cpp/.h` | Acquisition, base EDID parse/write, import/export, checksums, registry save/delete | Raw block buffers; four resolution/extension lists; `PropertiesClass`; SetupAPI/registry | Highest-risk file. It combines model, persistence, and OS access; do not alter serialization/save initially. |
| Vendor EDID acquisition | `AMDDisplayClass.cpp/.h`, `NVIDIADisplayClass.cpp/.h`, `EDIDListClass.cpp/.h` | `LoadEDIDList`; obtain active EDID and match it to instances | Runtime-loaded ADL/NVAPI DLLs and locally declared ABI structures | Driver-version/ABI risk. Read-only today; not a capability or settings backend. |
| Base detailed timings | `DetailedResolutionClass.cpp/.h`, `DetailedResolutionListClass.cpp/.h` | `Read`, `Write`, timing calculations, rate/clock getters, validation | `ItemClass`/`ListClass`; integer/fixed-point timing fields | Best reusable timing seed, but mixed representation/calculation and legacy assumptions require characterization tests first. |
| Timing snapshot | `TimingSnapshotClass.h` | Copy an existing detailed timing into a read-only, VCL-free value object | Getter-compatible timing source; normally `DetailedResolutionClass` | Low risk: no serialization, registry, UI, or hardware access. Future analyzers should consume this layer. |
| Portable timing model | `core/include/TimingSnapshot.h`, `core/src/TimingSnapshot.cpp` | Standard C++ timing value with explicit Hz/mHz units and scan/timing/blanking enums | C++ standard library only | Independent of Legacy; intended input to future analyzers. |
| Portable EDID DTD reader | `core/include/DetailedTimingDescriptor.h`, `core/src/DetailedTimingDescriptor.cpp` | Parse one fixed 18-byte descriptor into a portable snapshot | `std::array`, `std::optional`, portable timing model | Read-only; deliberately has no serialization or EDID document ownership. |
| Portable base EDID reader | `core/include/EdidBaseBlock.h`, `core/src/EdidBaseBlock.cpp` | Validate one 128-byte base block, expose basic identity/version fields, and parse DTD slots at bytes 54–125 | Standard library and portable DTD reader only | Read-only; extension payloads and non-DTD descriptors are not interpreted yet. |
| Monitor range-limits reader | `core/include/MonitorRangeLimits.h`, `core/src/MonitorRangeLimits.cpp` | Parse EDID `0xFD` descriptors into advertised vertical/horizontal frequency ranges, maximum pixel clock, and secondary timing formula | Standard library only; integrated with base EDID and capabilities snapshot | Advertised metadata, not a measured hard limit. Especially for CRTs, estimates must combine scan rate, pixel clock, resolution, and totals rather than treating maximum vertical Hz as absolute. |
| Range capability estimator | `core/include/RangeCapabilityEstimator.h`, `core/src/RangeCapabilityEstimator.cpp` | For one existing detailed timing, calculate separate horizontal-scan and pixel-clock refresh ceilings, their mathematical minimum, and an EDID-vertical-bounded ceiling | Timing snapshot and advertised monitor range limits | Fixed-total arithmetic estimate only. Values above advertised vertical limits are labeled experimental territory and are neither measured nor safe/apply recommendations. |
| Portable CTA-861 reader | `core/include/Cta861Extension.h`, `core/src/Cta861Extension.cpp` | Validate a CTA block, expose header flags, preserve generic data-block tag/payload pairs, and parse trailing DTDs | Standard library, portable DTD reader | Structural only: HDMI, audio, HDR, colorimetry, and VRR semantics are intentionally not decoded yet. |
| Portable CTA data-block view | `core/include/CtaDataBlockView.h`, `core/src/CtaDataBlockView.cpp` | Classify generic CTA data blocks and expose an Extended Data Block tag without copying payload data | Portable CTA-861 reader | Classification only; it does not interpret audio, HDR, colorimetry, or vendor payloads. |
| Portable CTA Video Data Block decoder | `core/include/CtaVideoDataBlock.h`, `core/src/CtaVideoDataBlock.cpp` | Decode read-only Short Video Descriptors into VIC/native pairs using CRU's existing native-bit interpretation | Portable CTA-861 reader | Does not map VICs to timings or infer bandwidth/display capabilities. |
| Portable CTA VIC catalog | `core/include/CtaVicCatalog.h`, `core/src/CtaVicCatalog.cpp` | Map every non-empty VIC entry represented by Legacy `TVResolutionClass::Codes` to resolution, scan mode, aspect code, nominal refresh rate, and Legacy support status | Standard library only; Legacy table is the behavioral reference | Nominal refresh values intentionally preserve Legacy CRU's integer-Hz representation; this is not a detailed timing or bandwidth model. |
| Portable advertised CTA video modes | `core/include/CtaAdvertisedVideoModes.h`, `core/src/CtaAdvertisedVideoModes.cpp` | Join parsed CTA Video Data Blocks with the VIC catalog while preserving native flags, ordering, duplicates, and unknown VICs | CTA parser, SVD decoder, VIC catalog | Read-only advertised-mode view; it does not infer detailed timings, bandwidth, or display capability. |
| Portable EDID document reader | `core/include/EdidDocument.h`, `core/src/EdidDocument.cpp` | Validate a complete block-aligned EDID, enforce its declared extension count, parse CTA extensions, preserve unknown extensions, and aggregate DTDs and advertised CTA modes | Base-block, CTA, advertised-mode readers | Read-only; unknown extension payloads are preserved but not interpreted. No serialization or hardware access. |
| Display capabilities snapshot | `core/include/DisplayCapabilitiesSnapshot.h`, `core/src/DisplayCapabilitiesSnapshot.cpp` | Copy parsed display identity, extension summary, detailed timings, and advertised CTA modes into a UI-free immutable-through-API view | Portable EDID document reader | Reports advertised EDID data only; it does not infer link bandwidth, GPU support, HDR, VRR, or gaming suitability. |
| Portable timing analyzer | `core/include/TimingAnalyzer.h`, `core/src/TimingAnalyzer.cpp` | Check snapshot component/total/rate consistency and report active pixels, total pixels, active-pixel ratio, and independently calculated rates | Portable timing snapshot only | Structural arithmetic only; no HDMI/DP bandwidth, GPU, monitor, HDR, VRR, or gaming verdicts. |
| Display timing report | `core/include/DisplayTimingReport.h`, `core/src/DisplayTimingReport.cpp` | Analyze every detailed timing in a capabilities snapshot and expose copied per-timing results plus consistent/inconsistent totals | Display capabilities snapshot and timing analyzer | Summary only; it does not rank modes or make hardware compatibility decisions. |
| Display mode inventory | `core/include/DisplayModeInventory.h`, `core/src/DisplayModeInventory.cpp` | Merge and deduplicate resolved DTD and CTA VIC modes, preserve source/native metadata, and summarize advertised minimum/maximum refresh per resolution and scan mode | Display capabilities snapshot | Advertised-mode inventory only; ranges are not overclocking limits or proof of link/GPU support. |
| EDID inspection CLI | `tools/EdidInspect/EdidInspect.cpp` | Read an EDID binary file and emit human-readable or schema-versioned JSON identity, timing analysis, and advertised-mode data | Portable Core plus standard C++ file/stream APIs | Diagnostic only; no hardware discovery or write/apply path. |
| Portable Core command wrappers | `tools/BuildPortableCore.ps1`, `tools/InspectEdid.ps1` | Discover an existing VS 2022 build, run validation, and invoke the read-only inspector with optional JSON file output | PowerShell, existing MSBuild, Portable Core binaries | No installation, hardware discovery, Registry access, or display mutation. |
| Read-only EDID Viewer GUI | `tools/EdidViewer/EdidViewer.cpp`, `tools/EdidViewer/MonitorDiscovery.cpp`, `tools/EdidViewer/EdidViewer.vcxproj` | Discover present Windows monitor devices, read their installed EDID with read-only access, or select a binary file, then display identity, timing health, detailed timings, and CTA modes | Win32 SetupAPI/read-only device registry presentation layer over Portable Core | Separate from the platform-independent core; no Registry writes, serialization, override, mode change, or apply path. |
| Standard/established timings | `StandardResolutionClass*`, `StandardResolutionListClass*`, `EstablishedResolutionListClass*` | EDID standard and established timing byte handling | Raw list slots | Serialization-sensitive; do not modify initially. |
| Extension blocks | `ExtensionBlockClass.cpp/.h`, `ExtensionBlockListClass.cpp/.h` | CEA-861, VTB-EXT, DisplayID, default/raw extension dispatch and layout | Detailed/standard/CEA/DisplayID lists | Central 128-byte layout and checksum boundary; high risk. |
| CEA/CTA data blocks | `CEADataListClass.cpp/.h` | Generic data-block collection, tag/OUI/extended-tag classification, read/write, editability | `ListClass`, raw slots up to 32 bytes | Recognizes more blocks than it can semantically edit; preserve unknown bytes. |
| DisplayID | `DIDDataListClass.cpp/.h`, `DIDDetailedResolutionListClass.cpp/.h`, `TiledDisplayTopologyClass.cpp/.h` | DisplayID block classification; detailed timing and tiled-topology editing | Raw variable-size blocks | Partial semantic coverage; other blocks pass through as raw slots. |
| HDMI | `HDMISupportClass*`, `HDMI2SupportClass*`, `HDMIResolutionClass*` | HDMI VSDB and HDMI Forum VSDB fields, TMDS limits, deep color, latency, HDMI VIC/3D data | CEA vendor blocks; editor forms | Covers HDMI 1.x/2.0-era TMDS metadata, not an end-to-end link analyzer. No FRL or HDMI 2.1 model was found. |
| FreeSync | `FreeSyncRangeClass.cpp/.h`, `FreeSyncRangeFormClass*` | Parse/write AMD vendor-specific CEA block and validate min/max vertical Hz | CEA OUI classification and raw bytes | Narrow AMD FreeSync range only; not generic Adaptive-Sync/HDMI VRR state. |
| HDR/color | `CEADataListClass*`, `ColorimetryClass*`, `ColorimetryFormClass*` | Recognize HDR static/dynamic extended tags; edit colorimetry/metadata-profile bits | CEA extended data blocks | HDR blocks are named/classified but have no typed HDR editor/validator; they remain opaque. |
| Common containers | `ItemClass*`, `ListClass*`, `BitListClass*`, `Common.cpp/.h`, `CommonFormClass*` | Raw-slot list operations, copy/undo, UI sizing/drawing and conversions | VCL in common/form utilities | `ListClass` is reusable but owns raw bytes; UI helpers are VCL-only. |
| Registry override | `DisplayClass.cpp` | `Save`, `SaveOverrideData`, `SaveActiveData`, `DeleteData` | Elevated HKLM access | Can create invalid EDIDs or remove overrides. Isolate behind a future transactional service. |
| Driver restart/recovery | `RESTART-DISPLAY-DRIVER/restart.c` | Restart display devices and offer F8 recovery | SetupAPI/Configuration Manager, registry, service/process handling | Black-screen/system-state risk; preserve as a separate recovery tool. |
| Global reset | `RESET-ALL/reset-all/reset-all.c` | Remove all monitor overrides and graphics caches | Registry/elevation | Destructive, intentionally broad recovery operation. |

## Important files

| File | Responsibility | Importance | Safe to modify? |
|---|---|---:|---|
| `CRU/CRU.cpp` | Application entry point and VCL form registration | High | Avoid initially; low-value change surface |
| `CRU/DisplayFormClass.cpp` / `.dfm` | Main UI and final OK/save command | High | UI-only additions can be isolated; save path is unsafe |
| `CRU/DisplayListClass.cpp` | Monitor-instance enumeration, vendor EDID matching, aggregate save | Critical | No, until discovery/save tests exist |
| `CRU/DisplayClass.cpp` / `.h` | EDID aggregate, base parser/serializer, checksums, import/export, registry override | Critical | No during first phase |
| `CRU/DetailedResolutionClass.cpp` / `.h` | Detailed timing representation, calculations, validation, parse/write | Critical | Read-only adapter is safe; modifying formulas/write is not |
| `CRU/TimingSnapshotClass.h` | Read-only copied view of a detailed timing | Medium | Yes, while it remains value-only and side-effect free |
| `CRU/StandardResolutionClass.cpp` / `.h` | Standard timing representation and encoding | High | No, pending round-trip tests |
| `CRU/ExtensionBlockClass.cpp` / `.h` | CEA/VTB/DisplayID extension dispatch and serialization | Critical | No during first phase |
| `CRU/CEADataListClass.cpp` / `.h` | CTA data-block parsing, classification, raw preservation, serialization | Critical | No during first phase |
| `CRU/DIDDataListClass.cpp` / `.h` | DisplayID block parsing/classification/serialization | Critical | No during first phase |
| `CRU/FreeSyncRangeClass.cpp` / `.h` | AMD FreeSync CEA range block | High | Read-only use is safe; write changes are not |
| `CRU/HDMISupportClass.cpp` / `.h` | HDMI VSDB | High | Read-only use is safe; serializer changes are not |
| `CRU/HDMI2SupportClass.cpp` / `.h` | HDMI Forum VSDB / HDMI 2.0-era fields | High | Read-only use is safe; serializer changes are not |
| `CRU/ColorimetryClass.cpp` / `.h` | CTA colorimetry flags | Medium | Only after fixtures; not a complete HDR model |
| `CRU/AMDDisplayClass.cpp` | Dynamic ADL-based active EDID acquisition | High | No, hardware/vendor-specific |
| `CRU/NVIDIADisplayClass.cpp` | Dynamic NVAPI-based active EDID acquisition | High | No, hardware/vendor-specific |
| `CRU/ListClass.cpp` / `ItemClass.cpp` | Raw-slot storage, copy, undo, sizing | High | Carefully, because every format list depends on it |
| `CRU/Common.cpp` | Shared conversion and VCL drawing helpers | Medium | Small isolated changes only |
| `CRU/CRU.bdsproj` | BDS 2006 Win32 build definition | Critical | Do not convert before baseline build |
| `RESTART-DISPLAY-DRIVER/restart.c` | Driver restart, cache reset, and recovery override swap | Critical | No; hardware/recovery-specific |
| `RESET-ALL/reset-all/reset-all.c` | Global override/cache removal | Critical | No; destructive recovery utility |

## UI-only versus core-bearing files

Files ending in `FormClass.cpp/.h/.dfm` are primarily VCL presentation and event handling. `CRU.cpp` is VCL startup. Model/core-bearing files are the non-Form `*Class.cpp/.h` pairs, but they are not a portable core: many include `vcl.h`, use Borland types/conventions, or expose raw buffer-oriented interfaces. `DisplayClass` and `DisplayListClass` additionally contain Windows persistence and discovery.

## Actual format coverage and limitations

- EDID base block: detailed, standard, and established timings plus monitor properties are parsed and serialized.
- CEA-861/CTA: many tag types are classified. Audio, video, speaker allocation, HDMI, HDMI 2.0, FreeSync, colorimetry, video capability, and YCbCr 4:2:0 video have typed editing. HDR static/dynamic metadata are recognized only by tag name and preserved as opaque data.
- DisplayID: the extension container and block list are supported, with typed editors for detailed timings and tiled topology. This is not complete DisplayID semantic coverage.
- HDMI: TMDS clock/rate, deep-color and related VSDB/HF-VSDB flags are modeled. No FRL, HDMI 2.1 link-rate, or DSC implementation was found.
- DisplayPort: no protocol/link-rate/lane-count capability model was found.
- DSC: no reference or implementation was found.
- Bandwidth: no general bandwidth/link-budget analyzer was found. Detailed timing exposes pixel clock and totals; HDMI models expose advertised TMDS limits.

## Files to avoid in the first coding phase

Do not initially modify `DisplayClass.cpp`, `ExtensionBlockClass.cpp`, `CEADataListClass.cpp`, `DIDDataListClass.cpp`, any `Write` method in format classes, `RESTART-DISPLAY-DRIVER/restart.c`, or `RESET-ALL/reset-all/reset-all.c`. They define byte layout, checksums, privileged writes, or recovery behavior. Also avoid project-format conversion until the unchanged BDS build has been reproduced and archived.

## Principal risks

- A serializer size/offset error can corrupt an EDID block while still producing a valid checksum.
- Partial registry writes can leave a mixed base/extension override; `SaveOverrideData` writes values sequentially and has no transaction.
- A syntactically valid but unsupported timing can cause loss of picture until driver restart, safe mode, recovery, or override removal.
- ADL/NVAPI use private local declarations and dynamically resolved entry points; vendor updates can change behavior.
- Monitor, GPU, cable/link, adapter, and driver limits are distinct. Current code mostly represents EDID claims, not negotiated link capability.
- HDMI TMDS metadata must not be generalized to DisplayPort or modern HDMI FRL. DSC is currently unknown, not unsupported by the hardware.
