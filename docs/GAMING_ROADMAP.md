# Gaming Extension Roadmap

## Guiding boundary

The first gaming code should be read-only and side-effect free. Existing EDID byte models remain the source of truth; new analysis/profile objects should consume snapshots through adapters and must not write `DisplayClass::OverrideData` or the registry.

## Recommended extension seams

| Capability | Existing seam to reuse | First new boundary | Files that should remain unchanged initially |
|---|---|---|---|
| Gaming profiles | `DisplayClass` getters for detailed/standard/extension lists; `DetailedResolutionClass` value getters | New independent `GamingProfile` value type and an adapter that imports a selected timing | `DisplayClass::Save*`, all EDID `Write` methods |
| Timing analyzer | `DetailedResolutionClass` getters (`GetH*`, `GetV*`, `GetVRate`, `GetHRate`, `GetPClock`) and its five legacy timing calculators | New pure `TimingSnapshot` + `TimingAnalysis` layer, initially populated from an existing detailed timing | `DetailedResolutionClass::Calculate*` until golden tests exist |
| Capability analyzer | `CEADataListClass::GetSlotType`, HDMI/HDMI2 typed readers, `ColorFormatListClass`, `DIDDataListClass` | New read-only evidence model: monitor EDID facts, requested mode, link facts (initially unknown), GPU/driver facts (initially unknown) | Vendor loaders and registry code |
| VRR/FreeSync | `CEADataListClass` classification; `FreeSyncRangeClass::Read/Write/IsValid`; `TExtensionBlockForm::CEADataEditFreeSyncRange` | Adapter that extracts an optional advertised AMD FreeSync range into a profile/capability snapshot | FreeSync serializer and CEA block layout |
| HDR | CEA extended-tag recognition (`CEA_HDR_STATIC`, `CEA_HDR_DYNAMIC`), `ColorimetryClass` | First add a read-only typed decoder that preserves original bytes and reports absent/unknown fields | Generic CEA read/write and unknown-block preservation |
| HDMI | `HDMISupportClass`, `HDMI2SupportClass`, `CEADataListClass::HDMISupported/HDMI2Supported` | Read-only advertised-TMDS capability adapter, explicitly labeled EDID evidence | Existing VSDB serializers |
| DisplayID | `ExtensionBlockClass::DIDData`, `DIDDataListClass`, `DIDDetailedResolutionListClass` | Read-only normalization of supported detailed timing records | DisplayID write/layout code |
| DisplayPort / DSC | No implementation seam exists | New capability interfaces whose values can be `Unknown`; later populate from trustworthy OS/vendor APIs | Do not infer DP/DSC from pixel clock or absence in EDID |
| Safe apply/rollback | Existing export, `DisplayClass` reset state, separate restart recovery utility | A future transactional orchestration layer: backup → validate → apply → confirm → rollback | Do not fold restart utility into CRU UI prematurely |

## Current FreeSync representation

The existing VRR support is an AMD vendor-specific CEA data block. `CEADataListClass::GetSlotType()` identifies the vendor OUI and returns `CEA_FREESYNC`; `FreeSyncRangeClass` reads/writes the block and stores two integer fields, `MinVRate` and `MaxVRate`, constrained to 1–255 Hz and ordered min ≤ max. `FreeSyncRangeFormClass` provides the UI, dispatched by `TExtensionBlockForm::CEADataEdit()`.

This is the safest future profile seam: extract the range to a neutral optional `VrrRange` without changing the original block. It must be labeled AMD FreeSync EDID evidence, not generic OS VRR availability. There is no generic Adaptive-Sync, HDMI Forum VRR, LFC, per-game enablement, GPU validation, or link validation here.

## Current HDR representation

`CEADataListClass` recognizes extended tag codes for HDR static metadata and HDR dynamic metadata and displays their names. Neither type is included in `EditPossible()`, and there is no `HDR*Class` or HDR form. Consequently these blocks are preserved as raw list slots but are not semantically decoded or validated. `ColorimetryClass` edits CTA colorimetry flags and metadata profile bits; those flags are related evidence, not a complete HDR capability model.

A future HDR decoder should be read-only first, distinguish EOTF/static-metadata support from luminance values and colorimetry, retain unknown bytes, and never equate an advertised block with a usable Windows HDR pipeline.

## Timing analyzer sequence

1. Define portable value objects for active dimensions, porch/sync/blanking, totals, scan type, rates, and pixel clock.
2. Add an adapter from `DetailedResolutionClass`; verify adapter output against known EDID fixtures.
3. Implement derived calculations (totals, blanking percentages, raw pixel rate) without link assumptions.
4. Add color depth and format to estimate unencoded payload bandwidth.
5. Add separate link models for TMDS, FRL, DisplayPort, and DSC only when their encoding/overhead and capability evidence are modeled explicitly.

The existing `DetailedResolutionClass` is useful because it already has timing fields, clock/rate getters, validation, and algorithms labeled LCD standard/native/reduced, CRT standard, and old standard. Its labels do not map cleanly to a promised modern CVT/CVT-RB/CVT-RB2 API, so characterize the formulas before assigning standards-based names.

## Capability result model

Build results from explicit evidence rather than a single boolean:

```text
RequestedMode
  + MonitorEdidEvidence
  + GpuEvidence (may be Unknown)
  + LinkEvidence (may be Unknown)
  + DriverEvidence (may be Unknown)
  → Supported | SupportedWithDsc | BandwidthLimit | EdidLimit | DriverLimit | Unknown
```

Absence of DP/DSC code means the correct initial answer for those dimensions is `Unknown`. `SupportedWithDsc` must never be produced until both endpoint support and the selected link configuration are known.

## Profile rollout

- Phase A: in-memory `GamingProfile` with name, selected existing detailed timing, and optional read-only VRR/HDR evidence. No persistence or apply.
- Phase B: versioned profile serialization to an application-owned file, with no EDID bytes duplicated unless explicitly exported.
- Phase C: compare a profile to current EDID and report proposed changes.
- Phase D: generate changes in memory and round-trip through fixture tests.
- Phase E: guarded apply with mandatory backup and external recovery path.
- Phase F: per-game activation only after a reliable Windows display-mode/app lifecycle design exists.

Preset names such as Desktop, Competitive, Balanced, and Quality should remain policy/UI labels over the same neutral profile schema rather than separate code paths.

## Recommended first coding task (do not implement yet)

Create a standalone, read-only `TimingSnapshot` adapter for `DetailedResolutionClass`, plus fixture-based tests that assert width, height, totals, refresh rate, scan type, and pixel clock for several known detailed timing descriptors.

This is the best start because it is useful to every future gaming feature, exercises existing timing behavior without changing it, introduces the first test seam, performs no registry or hardware action, and avoids EDID serialization. The adapter should initially live beside new tests rather than refactoring `DetailedResolutionClass` itself.

