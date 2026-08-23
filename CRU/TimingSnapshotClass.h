//---------------------------------------------------------------------------
#ifndef TimingSnapshotClassH
#define TimingSnapshotClassH
//---------------------------------------------------------------------------
class TimingSnapshotClass
{
private:
	int Timing;
	int HActive;
	int HFront;
	int HSync;
	int HBack;
	int HBlank;
	int HTotal;
	bool HPolarity;
	int VActive;
	int VFront;
	int VSync;
	int VBack;
	int VBlank;
	int VTotal;
	bool VPolarity;
	long long VRate;
	long long HRate;
	long long PClock;
	bool Interlaced;
	bool Native;

public:
	template <class DetailedResolutionType>
	explicit TimingSnapshotClass(DetailedResolutionType &DetailedResolution)
	{
		Timing = DetailedResolution.GetTiming();
		HActive = DetailedResolution.GetHActive();
		HFront = DetailedResolution.GetHFront();
		HSync = DetailedResolution.GetHSync();
		HBack = DetailedResolution.GetHBack();
		HBlank = DetailedResolution.GetHBlank();
		HTotal = DetailedResolution.GetHTotal();
		HPolarity = DetailedResolution.GetHPolarity();
		VActive = DetailedResolution.GetVActive();
		VFront = DetailedResolution.GetVFront();
		VSync = DetailedResolution.GetVSync();
		VBack = DetailedResolution.GetVBack();
		VBlank = DetailedResolution.GetVBlank();
		VTotal = DetailedResolution.GetVTotal();
		VPolarity = DetailedResolution.GetVPolarity();
		VRate = DetailedResolution.GetVRate();
		HRate = DetailedResolution.GetHRate();
		PClock = DetailedResolution.GetPClock();
		Interlaced = DetailedResolution.GetInterlaced();
		Native = DetailedResolution.GetNative();
	}

	int GetTiming() const { return Timing; }
	int GetHActive() const { return HActive; }
	int GetHFront() const { return HFront; }
	int GetHSync() const { return HSync; }
	int GetHBack() const { return HBack; }
	int GetHBlank() const { return HBlank; }
	int GetHTotal() const { return HTotal; }
	bool GetHPolarity() const { return HPolarity; }
	int GetVActive() const { return VActive; }
	int GetVFront() const { return VFront; }
	int GetVSync() const { return VSync; }
	int GetVBack() const { return VBack; }
	int GetVBlank() const { return VBlank; }
	int GetVTotal() const { return VTotal; }
	bool GetVPolarity() const { return VPolarity; }
	long long GetVRate() const { return VRate; }
	long long GetHRate() const { return HRate; }
	long long GetPClock() const { return PClock; }
	bool GetInterlaced() const { return Interlaced; }
	bool GetProgressive() const { return !Interlaced; }
	bool GetNative() const { return Native; }
	bool IsLCDReducedTiming() const { return Timing == 3; }
};
//---------------------------------------------------------------------------
#endif

