#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace cru { namespace core {

enum class SecondaryTimingFormula : std::uint8_t
{
	DefaultGtf = 0,
	NoTimingFormula = 1,
	SecondaryGtf = 2,
	Cvt = 4,
	Unknown = 255
};

struct MonitorRangeLimits
{
	std::uint16_t minimum_vertical_rate_hz;
	std::uint16_t maximum_vertical_rate_hz;
	std::uint16_t minimum_horizontal_rate_khz;
	std::uint16_t maximum_horizontal_rate_khz;
	std::uint64_t maximum_pixel_clock_hz;
	SecondaryTimingFormula secondary_timing_formula;
};

class MonitorRangeLimitsDescriptor
{
public:
	using Bytes = std::array<std::uint8_t, 18>;
	static std::optional<MonitorRangeLimits> parse(const Bytes &bytes, bool extended_offsets_allowed) noexcept;
};

} }
