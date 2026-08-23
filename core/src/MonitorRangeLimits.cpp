#include "MonitorRangeLimits.h"

namespace cru { namespace core {
namespace {

SecondaryTimingFormula timing_formula(std::uint8_t value) noexcept
{
	switch (value)
	{
	case 0U: return SecondaryTimingFormula::DefaultGtf;
	case 1U: return SecondaryTimingFormula::NoTimingFormula;
	case 2U: return SecondaryTimingFormula::SecondaryGtf;
	case 4U: return SecondaryTimingFormula::Cvt;
	default: return SecondaryTimingFormula::Unknown;
	}
}

}

std::optional<MonitorRangeLimits> MonitorRangeLimitsDescriptor::parse(
	const Bytes &bytes, bool extended_offsets_allowed) noexcept
{
	if (bytes[0] != 0U || bytes[1] != 0U || bytes[2] != 0U || bytes[3] != 0xFDU)
		return std::nullopt;

	const std::uint16_t minimum_vertical = static_cast<std::uint16_t>(bytes[5])
		+ ((extended_offsets_allowed && (bytes[4] & 0x01U) != 0U) ? 255U : 0U);
	const std::uint16_t maximum_vertical = static_cast<std::uint16_t>(bytes[6])
		+ ((extended_offsets_allowed && (bytes[4] & 0x02U) != 0U) ? 255U : 0U);
	const std::uint16_t minimum_horizontal = static_cast<std::uint16_t>(bytes[7])
		+ ((extended_offsets_allowed && (bytes[4] & 0x04U) != 0U) ? 255U : 0U);
	const std::uint16_t maximum_horizontal = static_cast<std::uint16_t>(bytes[8])
		+ ((extended_offsets_allowed && (bytes[4] & 0x08U) != 0U) ? 255U : 0U);

	if (minimum_vertical == 0U || minimum_vertical > maximum_vertical
		|| minimum_horizontal == 0U || minimum_horizontal > maximum_horizontal
		|| bytes[9] == 0U)
		return std::nullopt;

	return MonitorRangeLimits{
		minimum_vertical, maximum_vertical, minimum_horizontal, maximum_horizontal,
		static_cast<std::uint64_t>(bytes[9]) * 10000000U, timing_formula(bytes[10])};
}

} }
