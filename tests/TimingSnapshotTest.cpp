#include "../CRU/TimingSnapshotClass.h"
#include <cstdio>

class TimingFixture
{
public:
	int Timing;
	int HActive, HFront, HSync, HBack, HBlank, HTotal;
	bool HPolarity;
	int VActive, VFront, VSync, VBack, VBlank, VTotal;
	bool VPolarity;
	long long VRate, HRate, PClock;
	bool Interlaced, Native;

	int GetTiming() { return Timing; }
	int GetHActive() { return HActive; }
	int GetHFront() { return HFront; }
	int GetHSync() { return HSync; }
	int GetHBack() { return HBack; }
	int GetHBlank() { return HBlank; }
	int GetHTotal() { return HTotal; }
	bool GetHPolarity() { return HPolarity; }
	int GetVActive() { return VActive; }
	int GetVFront() { return VFront; }
	int GetVSync() { return VSync; }
	int GetVBack() { return VBack; }
	int GetVBlank() { return VBlank; }
	int GetVTotal() { return VTotal; }
	bool GetVPolarity() { return VPolarity; }
	long long GetVRate() { return VRate; }
	long long GetHRate() { return HRate; }
	long long GetPClock() { return PClock; }
	bool GetInterlaced() { return Interlaced; }
	bool GetNative() { return Native; }
};

static int Failures;

#define CHECK_EQUAL(Expected, Actual) CheckEqual(__LINE__, #Actual, (long long)(Expected), (long long)(Actual))

static void CheckEqual(int Line, const char *Name, long long Expected, long long Actual)
{
	if (Expected == Actual)
		return;

	std::printf("line %d: %s: expected %lld, got %lld\n", Line, Name, Expected, Actual);
	Failures++;
}

static void CheckSnapshot(TimingFixture &Fixture)
{
	TimingSnapshotClass Snapshot(Fixture);

	CHECK_EQUAL(Fixture.Timing, Snapshot.GetTiming());
	CHECK_EQUAL(Fixture.HActive, Snapshot.GetHActive());
	CHECK_EQUAL(Fixture.HFront, Snapshot.GetHFront());
	CHECK_EQUAL(Fixture.HSync, Snapshot.GetHSync());
	CHECK_EQUAL(Fixture.HBack, Snapshot.GetHBack());
	CHECK_EQUAL(Fixture.HBlank, Snapshot.GetHBlank());
	CHECK_EQUAL(Fixture.HTotal, Snapshot.GetHTotal());
	CHECK_EQUAL(Fixture.HPolarity, Snapshot.GetHPolarity());
	CHECK_EQUAL(Fixture.VActive, Snapshot.GetVActive());
	CHECK_EQUAL(Fixture.VFront, Snapshot.GetVFront());
	CHECK_EQUAL(Fixture.VSync, Snapshot.GetVSync());
	CHECK_EQUAL(Fixture.VBack, Snapshot.GetVBack());
	CHECK_EQUAL(Fixture.VBlank, Snapshot.GetVBlank());
	CHECK_EQUAL(Fixture.VTotal, Snapshot.GetVTotal());
	CHECK_EQUAL(Fixture.VPolarity, Snapshot.GetVPolarity());
	CHECK_EQUAL(Fixture.VRate, Snapshot.GetVRate());
	CHECK_EQUAL(Fixture.HRate, Snapshot.GetHRate());
	CHECK_EQUAL(Fixture.PClock, Snapshot.GetPClock());
	CHECK_EQUAL(Fixture.Interlaced, Snapshot.GetInterlaced());
	CHECK_EQUAL(!Fixture.Interlaced, Snapshot.GetProgressive());
	CHECK_EQUAL(Fixture.Native, Snapshot.GetNative());
	CHECK_EQUAL(Fixture.Timing == 3, Snapshot.IsLCDReducedTiming());

	Fixture.HActive = 1;
	Fixture.PClock = 1;
	CHECK_EQUAL(Snapshot.GetHActive() != Fixture.HActive, true);
	CHECK_EQUAL(Snapshot.GetPClock() != Fixture.PClock, true);
}

int main()
{
	// CTA-861 1920x1080p60 detailed timing.
	TimingFixture Timings[] =
	{
		{1, 1920, 88, 44, 148, 280, 2200, true, 1080, 4, 5, 36, 45, 1125, true, 60000, 67500, 14850, false, false},
		// Values produced by CRU's LCD-standard CVT-RB fallback.
		{1, 1920, 48, 32, 80, 160, 2080, true, 1080, 3, 5, 69, 77, 1157, false, 144000, 166610, 34655, false, false},
		{1, 2560, 48, 32, 80, 160, 2720, true, 1440, 3, 5, 95, 103, 1543, false, 144000, 222194, 60437, false, false},
		{1, 2560, 48, 32, 80, 160, 2720, true, 1440, 3, 5, 171, 179, 1619, false, 240000, 388562, 105689, false, false},
		// CRU timing type 3: Automatic - LCD reduced.
		{3, 1920, 48, 32, 80, 160, 2080, true, 1080, 3, 5, 23, 31, 1111, false, 60000, 66663, 13866, false, false},
		// CTA-861 1920x1080i60. CRU stores vertical values per field.
		{1, 1920, 88, 44, 148, 280, 2200, true, 540, 2, 5, 15, 22, 562, true, 60000, 33750, 7425, true, false}
	};

	const int Count = sizeof Timings / sizeof Timings[0];

	for (int Index = 0; Index < Count; Index++)
		CheckSnapshot(Timings[Index]);

	if (Failures)
		return 1;

	std::printf("TimingSnapshot: %d timing fixtures passed.\n", Count);
	return 0;
}

