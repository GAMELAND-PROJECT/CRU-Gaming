#pragma once

#include "TimingSnapshot.h"
#include "MonitorRangeLimits.h"
#include "BaseAdvertisedTimings.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace cru { namespace core {

struct EdidBaseBlock
{
	std::uint16_t manufacturer_id;
	std::uint16_t product_code;
	std::uint32_t serial_number;
	std::uint8_t version;
	std::uint8_t revision;
	std::uint8_t extension_count;
	std::vector<TimingSnapshot> detailed_timings;
	std::optional<MonitorRangeLimits> range_limits;
	std::vector<BaseAdvertisedTiming> base_advertised_timings;
};

class EdidBaseBlockParser
{
public:
	using Bytes = std::array<std::uint8_t, 128>;
	static std::optional<EdidBaseBlock> parse(const Bytes &bytes);
};

} }
