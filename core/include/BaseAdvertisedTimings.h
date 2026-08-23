#pragma once

#include "TimingSnapshot.h"

#include <array>
#include <cstdint>
#include <vector>

namespace cru { namespace core {

enum class BaseTimingSource
{
	Established,
	Standard
};

struct BaseAdvertisedTiming
{
	std::uint32_t horizontal_active;
	std::uint32_t vertical_active;
	std::uint32_t refresh_rate_millihertz;
	ScanMode scan_mode;
	BaseTimingSource source;
};

class BaseAdvertisedTimings
{
public:
	using EstablishedBytes = std::array<std::uint8_t, 3>;
	using StandardBytes = std::array<std::uint8_t, 16>;

	static std::vector<BaseAdvertisedTiming> parse_established(const EstablishedBytes &bytes);
	static std::vector<BaseAdvertisedTiming> parse_standard(
		const StandardBytes &bytes, std::uint8_t edid_version, std::uint8_t edid_revision);
};

} }
