# CRU timing tests

## Portable snapshot test

`TimingSnapshotTest.cpp` exercises the VCL-free snapshot with getter-compatible timing fixtures. It can be compiled with any C++ compiler.

## Real CRU integration test

`TimingSnapshotIntegrationTest.cpp` exercises the production parser path:

```text
18-byte EDID detailed timing descriptor
    -> DetailedResolutionClass(0)::Read(Data, 18)
    -> DetailedResolutionClass::Init()
    -> TimingSnapshotClass
```

To run it with Borland Developer Studio 2006, create a Win32 C++ console application in the IDE, add `TimingSnapshotIntegrationTest.cpp` and `../CRU/DetailedResolutionClass.cpp`, and add `../CRU` to the compiler include path. Use the same static RTL/VCL and Win32 library settings as `CRU/CRU.bdsproj`. No form, registry access, display enumeration, or administrator privileges are required.

The production source uses classic C++Builder pointer-to-member syntax, so current MSVC rejects `DetailedResolutionClass.cpp` before linking. Do not edit the production class merely to run this fixture with MSVC; the baseline integration target is BDS 2006.

### EDID DTD limit

An 18-byte EDID detailed timing descriptor stores pixel clock in an unsigned 16-bit field in 10 kHz units, limiting it to 655.35 MHz. A 2560×1440 progressive mode at 240 Hz requires more than 884.736 MHz even before blanking, so that requested case cannot be represented by this format. The fixture does not substitute a truncated or mislabeled descriptor. Coverage for that mode requires a format with a wider clock representation, such as the existing DisplayID detailed timing path, and belongs in a separate fixture.

## EdidInspect smoke test

`fixtures/edid-1080p60.hex` is the reviewable source for a valid 128-byte EDID fixture. The smoke-test script converts it to temporary binary files and deletes them afterward.

```powershell
.\tests\RunEdidInspectTests.ps1 -Executable .\x64\Debug\EdidInspect.exe
```

It verifies successful reporting of 1920×1080p60 and rejection of a corrupted checksum and a 127-byte file.
