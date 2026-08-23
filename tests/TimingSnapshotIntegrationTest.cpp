#include "../CRU/DetailedResolutionClass.h"
#include "../CRU/TimingSnapshotClass.h"
#include <cstdio>

struct ExpectedTiming
{
	const char *Name;
	unsigned char Data[18];
	int HActive, HFront, HSync, HBack, HBlank, HTotal;
	bool HPolarity;
	int VActive, VFront, VSync, VBack, VBlank, VTotal;
	bool VPolarity;
	long long VRate, HRate, PClock;
	bool Interlaced;
	int Timing;
};

static int Failures;

static void CheckEqual(const char *Test, const char *Field, long long Expected, long long Actual)
{
	if (Expected == Actual)
		return;

	std::printf("%s: %s: expected %lld, got %lld\n", Test, Field, Expected, Actual);
	Failures++;
}

#define CHECK(Test, Field, Expected, Actual) CheckEqual(Test, Field, (long long)(Expected), (long long)(Actual))

static void CheckTiming(ExpectedTiming &Expected)
{
	DetailedResolutionClass DetailedResolution(0);

	if (!DetailedResolution.Read(Expected.Data, sizeof Expected.Data))
	{
		std::printf("%s: DetailedResolutionClass::Read failed\n", Expected.Name);
		Failures++;
		return;
	}

	TimingSnapshotClass Snapshot(DetailedResolution);

	CHECK(Expected.Name, "HActive", Expected.HActive, Snapshot.GetHActive());
	CHECK(Expected.Name, "HFront", Expected.HFront, Snapshot.GetHFront());
	CHECK(Expected.Name, "HSync", Expected.HSync, Snapshot.GetHSync());
	CHECK(Expected.Name, "HBack", Expected.HBack, Snapshot.GetHBack());
	CHECK(Expected.Name, "HBlank", Expected.HBlank, Snapshot.GetHBlank());
	CHECK(Expected.Name, "HTotal", Expected.HTotal, Snapshot.GetHTotal());
	CHECK(Expected.Name, "HPolarity", Expected.HPolarity, Snapshot.GetHPolarity());
	CHECK(Expected.Name, "VActive", Expected.VActive, Snapshot.GetVActive());
	CHECK(Expected.Name, "VFront", Expected.VFront, Snapshot.GetVFront());
	CHECK(Expected.Name, "VSync", Expected.VSync, Snapshot.GetVSync());
	CHECK(Expected.Name, "VBack", Expected.VBack, Snapshot.GetVBack());
	CHECK(Expected.Name, "VBlank", Expected.VBlank, Snapshot.GetVBlank());
	CHECK(Expected.Name, "VTotal", Expected.VTotal, Snapshot.GetVTotal());
	CHECK(Expected.Name, "VPolarity", Expected.VPolarity, Snapshot.GetVPolarity());
	CHECK(Expected.Name, "VRate", Expected.VRate, Snapshot.GetVRate());
	CHECK(Expected.Name, "HRate", Expected.HRate, Snapshot.GetHRate());
	CHECK(Expected.Name, "PClock", Expected.PClock, Snapshot.GetPClock());
	CHECK(Expected.Name, "Interlaced", Expected.Interlaced, Snapshot.GetInterlaced());
	CHECK(Expected.Name, "Progressive", !Expected.Interlaced, Snapshot.GetProgressive());
	CHECK(Expected.Name, "Timing", Expected.Timing, Snapshot.GetTiming());
}

int main()
{
	ExpectedTiming Timings[] =
	{
		{
			"1920x1080p60 CTA-861",
			{0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E},
			1920, 88, 44, 148, 280, 2200, true,
			1080, 4, 5, 36, 45, 1125, true,
			60000, 67500, 14850, false, 0
		},
		{
			"1920x1080p144 CRU CVT-RB",
			{0x5F, 0x87, 0x80, 0xA0, 0x70, 0x38, 0x4D, 0x40, 0x30, 0x20, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A},
			1920, 48, 32, 80, 160, 2080, true,
			1080, 3, 5, 69, 77, 1157, false,
			144000, 166610, 34655, false, 0
		},
		{
			"2560x1440p144 CRU CVT-RB",
			{0x15, 0xEC, 0x00, 0xA0, 0xA0, 0xA0, 0x67, 0x50, 0x30, 0x20, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A},
			2560, 48, 32, 80, 160, 2720, true,
			1440, 3, 5, 95, 103, 1543, false,
			144000, 222194, 60437, false, 0
		},
		{
			"1920x1080p60 CRU CVT-RB",
			{0x2A, 0x36, 0x80, 0xA0, 0x70, 0x38, 0x1F, 0x40, 0x30, 0x20, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A},
			1920, 48, 32, 80, 160, 2080, true,
			1080, 3, 5, 23, 31, 1111, false,
			60000, 66663, 13866, false, 0
		},
		{
			"1920x1080i60 CTA-861",
			{0x01, 0x1D, 0x80, 0x18, 0x71, 0x1C, 0x16, 0x20, 0x58, 0x2C, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9E},
			1920, 88, 44, 148, 280, 2200, true,
			540, 2, 5, 15, 22, 562, true,
			60000, 33750, 7425, true, 0
		}
	};

	const int Count = sizeof Timings / sizeof Timings[0];

	for (int Index = 0; Index < Count; Index++)
		CheckTiming(Timings[Index]);

	if (Failures)
		return 1;

	std::printf("TimingSnapshot integration: %d EDID descriptors passed.\n", Count);
	return 0;
}

