#pragma once

#include "TimingSnapshot.h"

#include <cstdint>
#include <optional>

namespace cru { namespace core {

enum class CtaAspectRatio : std::uint8_t
{
	Unknown = 0,
	FourByThree = 4,
	TwelveFifty = 12,
	SixteenByNine = 16,
	TwoHundredFiftySixByOneThirtyFive = 17,
	SixtyFourByTwentySeven = 21
};

struct CtaVicInfo
{
	std::uint8_t video_identification_code;
	std::uint32_t horizontal_active;
	std::uint32_t vertical_active;
	ScanMode scan_mode;
	CtaAspectRatio aspect_ratio;
	std::uint32_t nominal_refresh_rate_millihertz;
	bool supported_by_legacy_cru;
};

class CtaVicCatalog
{
public:
	static std::optional<CtaVicInfo> lookup(std::uint8_t video_identification_code);
};

} }
