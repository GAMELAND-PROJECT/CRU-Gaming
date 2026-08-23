#include "DetailedTimingDescriptor.h"

namespace cru
{
namespace core
{
namespace
{

std::uint64_t calculate_pixel_clock_units(
	std::uint64_t refresh_rate_millihertz,
	std::uint64_t horizontal_total,
	std::uint64_t vertical_total,
	bool interlaced) noexcept
{
	const std::uint64_t vertical_factor = vertical_total * 2 + (interlaced ? 1U : 0U);
	return (refresh_rate_millihertz * horizontal_total * vertical_factor + 19999999U) / 20000000U;
}

std::uint64_t normalize_refresh_rate(
	std::uint64_t pixel_clock_units,
	std::uint64_t horizontal_total,
	std::uint64_t vertical_total,
	bool interlaced) noexcept
{
	const std::uint64_t vertical_factor = vertical_total * 2 + (interlaced ? 1U : 0U);
	const std::uint64_t actual = pixel_clock_units * 20000000U / horizontal_total / vertical_factor;
	std::uint64_t candidate = (actual + 500U) / 1000U * 1000U;

	if (calculate_pixel_clock_units(candidate, horizontal_total, vertical_total, interlaced) == pixel_clock_units)
		return candidate;

	if (candidate % 24000U == 0U || candidate % 30000U == 0U)
	{
		const std::uint64_t fractional_candidate = candidate * 1000U / 1001U;

		if (calculate_pixel_clock_units(fractional_candidate, horizontal_total, vertical_total, interlaced) == pixel_clock_units)
			return fractional_candidate;
	}

	candidate = (actual + 50U) / 100U * 100U;

	if (calculate_pixel_clock_units(candidate, horizontal_total, vertical_total, interlaced) == pixel_clock_units)
		return candidate;

	return actual;
}

}

std::optional<TimingSnapshot> DetailedTimingDescriptor::parse(const Bytes &bytes) noexcept
{
	const std::uint32_t pixel_clock_units =
		static_cast<std::uint32_t>(bytes[0]) |
		(static_cast<std::uint32_t>(bytes[1]) << 8U);

	if (pixel_clock_units == 0U)
		return std::nullopt;

	const std::uint32_t horizontal_active =
		static_cast<std::uint32_t>(bytes[2]) |
		((static_cast<std::uint32_t>(bytes[4]) & 0xF0U) << 4U);
	const std::uint32_t horizontal_blanking =
		static_cast<std::uint32_t>(bytes[3]) |
		((static_cast<std::uint32_t>(bytes[4]) & 0x0FU) << 8U);
	const std::uint32_t horizontal_front =
		static_cast<std::uint32_t>(bytes[8]) |
		((static_cast<std::uint32_t>(bytes[11]) & 0xC0U) << 2U);
	const std::uint32_t horizontal_sync =
		static_cast<std::uint32_t>(bytes[9]) |
		((static_cast<std::uint32_t>(bytes[11]) & 0x30U) << 4U);

	const std::uint32_t vertical_active =
		static_cast<std::uint32_t>(bytes[5]) |
		((static_cast<std::uint32_t>(bytes[7]) & 0xF0U) << 4U);
	const std::uint32_t vertical_blanking =
		static_cast<std::uint32_t>(bytes[6]) |
		((static_cast<std::uint32_t>(bytes[7]) & 0x0FU) << 8U);
	const std::uint32_t vertical_front =
		((static_cast<std::uint32_t>(bytes[10]) & 0xF0U) >> 4U) |
		((static_cast<std::uint32_t>(bytes[11]) & 0x0CU) << 2U);
	const std::uint32_t vertical_sync =
		(static_cast<std::uint32_t>(bytes[10]) & 0x0FU) |
		((static_cast<std::uint32_t>(bytes[11]) & 0x03U) << 4U);

	if (horizontal_active == 0U || vertical_active == 0U ||
		horizontal_front + horizontal_sync > horizontal_blanking ||
		vertical_front + vertical_sync > vertical_blanking)
	{
		return std::nullopt;
	}

	const std::uint32_t horizontal_back = horizontal_blanking - horizontal_front - horizontal_sync;
	const std::uint32_t vertical_back = vertical_blanking - vertical_front - vertical_sync;
	const std::uint32_t horizontal_total = horizontal_active + horizontal_blanking;
	const std::uint32_t vertical_total = vertical_active + vertical_blanking;
	const bool interlaced = (bytes[17] & 0x80U) != 0U;
	const std::uint64_t pixel_clock_hz = static_cast<std::uint64_t>(pixel_clock_units) * 10000U;
	const std::uint64_t horizontal_rate_hz = pixel_clock_hz / horizontal_total;
	const std::uint64_t refresh_rate_millihertz = normalize_refresh_rate(
		pixel_clock_units,
		horizontal_total,
		vertical_total,
		interlaced);

	AxisTiming horizontal = {
		horizontal_active,
		horizontal_front,
		horizontal_sync,
		horizontal_back,
		horizontal_blanking,
		horizontal_total,
		(bytes[17] & 0x02U) != 0U ? SyncPolarity::Positive : SyncPolarity::Negative};
	AxisTiming vertical = {
		vertical_active,
		vertical_front,
		vertical_sync,
		vertical_back,
		vertical_blanking,
		vertical_total,
		(bytes[17] & 0x04U) != 0U ? SyncPolarity::Positive : SyncPolarity::Negative};

	return TimingSnapshot(
		horizontal,
		vertical,
		pixel_clock_hz,
		refresh_rate_millihertz,
		horizontal_rate_hz,
		interlaced ? ScanMode::Interlaced : ScanMode::Progressive,
		TimingType::Manual,
		ReducedBlanking::Unknown);
}

}
}

