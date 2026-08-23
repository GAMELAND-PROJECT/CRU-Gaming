# Development Rules

1. Preserve existing CRU behavior and byte-for-byte output unless a reviewed change explicitly requires otherwise.
2. Make small, isolated changes with a clear rollback.
3. Do not rewrite large subsystems without evidence from tests and the reproduced legacy build.
4. Avoid changing EDID serialization, extension layout, checksums, or unknown-block preservation unless necessary.
5. Every change affecting EDID parsing or serialization requires fixture and round-trip tests.
6. Hardware- or vendor-specific changes require feature detection, an `Unknown`/safe fallback, and recovery instructions.
7. Never assume an EDID override is harmless or that a valid checksum means a usable mode.
8. Backup, confirmation, recovery, and rollback are first-class requirements for every apply path.
9. Prefer reusable, side-effect-free core logic over logic embedded in VCL event handlers.
10. Do not introduce dependencies unless they have a specific benefit, compatible license, support the required Windows targets, and a documented deployment plan.
11. Keep monitor EDID claims, GPU capabilities, physical-link capabilities, driver behavior, and requested settings as separate evidence sources.
12. Preserve unrecognized EDID/CTA/DisplayID bytes exactly. Do not normalize unknown blocks as a side effect of viewing them.
13. Use fixed-width integer types and explicit units in new code (`pixel_clock_hz`, `refresh_millihz`, `bandwidth_bits_per_second`).
14. New analysis code must be callable without VCL, registry access, a connected monitor, or administrator rights.
15. Do not write to `HKLM`, restart a driver, or reset graphics caches in automated tests.
16. Treat `DisplayClass.cpp`, extension/data-list `Write` methods, `restart.c`, and `reset-all.c` as protected high-risk areas requiring focused review.
17. Reproduce and archive an unchanged BDS 2006 build before project migration or compiler-driven source edits.
18. A new compiler port must be a separate milestone; do not mix it with feature work or semantic refactoring.

## Required review for EDID-affecting work

An EDID-affecting change must include the original binary fixture, expected parsed values, expected serialized bytes, checksum assertions for every 128-byte block, preservation assertions for unknown blocks, malformed/truncated input cases, and a documented recovery exercise. Review must explicitly consider block-size limits and partial registry-write behavior.

## Apply-path safety gate

No gaming profile may directly call `DisplayClass::Save()`. A future apply service must first export the original/active data, validate every generated block, show the exact target monitor instance, ensure a recovery utility is available, perform the smallest registry change, request a driver restart separately, require confirmation, and restore the backup automatically when confirmation fails.

