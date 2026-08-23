#include "GtfTimingCalculator.h"

#include <limits>

namespace cru { namespace core {

std::optional<TimingSnapshot> GtfTimingCalculator::calculate(
	std::uint32_t horizontal_active,
	std::uint32_t vertical_active,
	std::uint64_t refresh_rate_millihertz,
	ScanMode scan_mode) noexcept
{
	if (horizontal_active == 0U || horizontal_active > 65536U
		|| vertical_active == 0U || vertical_active > 65536U
		|| refresh_rate_millihertz == 0U || refresh_rate_millihertz > 10000000U)
		return std::nullopt;
	const std::int64_t interlaced = scan_mode == ScanMode::Interlaced ? 1LL : 0LL;
	const std::int64_t vertical_divisor = static_cast<std::int64_t>(vertical_active) * 2LL + 2LL + interlaced;
	const std::int64_t period_numerator = 2000000000000000000LL / static_cast<std::int64_t>(refresh_rate_millihertz)
		- 1100000000000LL;
	if (period_numerator <= 0LL) return std::nullopt;
	const std::int64_t horizontal_period = period_numerator / vertical_divisor;
	if (horizontal_period <= 0LL) return std::nullopt;

	const std::int64_t ideal_duty_cycle = 30000000000000LL - 300LL * horizontal_period;
	const std::int64_t duty_divisor = 100000000000000LL - ideal_duty_cycle;
	if (ideal_duty_cycle <= 0LL || duty_divisor <= 0LL) return std::nullopt;
	const std::int64_t horizontal_blanking =
		((static_cast<std::int64_t>(horizontal_active) * ideal_duty_cycle / duty_divisor + 8LL) / 16LL) * 16LL;
	const std::int64_t horizontal_back = horizontal_blanking / 2LL;
	const std::int64_t horizontal_sync =
		(static_cast<std::int64_t>(horizontal_active) + horizontal_blanking + 50LL) / 100LL * 8LL;
	const std::int64_t horizontal_front = horizontal_back - horizontal_sync;
	const std::int64_t vertical_front = 1LL;
	const std::int64_t vertical_sync = 3LL;
	const std::int64_t vertical_back = (5500000000000LL / horizontal_period + 5LL) / 10LL - vertical_sync;
	if (horizontal_front <= 0LL || horizontal_sync <= 0LL || horizontal_back < 0LL || vertical_back < 0LL)
		return std::nullopt;

	const std::int64_t horizontal_total = static_cast<std::int64_t>(horizontal_active) + horizontal_blanking;
	const std::int64_t vertical_blanking = vertical_front + vertical_sync + vertical_back;
	const std::int64_t vertical_total = static_cast<std::int64_t>(vertical_active) + vertical_blanking;
	const std::int64_t pixel_clock_10khz =
		(static_cast<std::int64_t>(refresh_rate_millihertz) * horizontal_total
			* (vertical_total * 2LL + interlaced) + 10000000LL) / 20000000LL;
	if (pixel_clock_10khz <= 0LL) return std::nullopt;
	const std::int64_t pixel_clock_hz = pixel_clock_10khz * 10000LL;
	const std::int64_t actual_refresh = pixel_clock_10khz * 20000000LL
		/ horizontal_total / (vertical_total * 2LL + interlaced);
	const std::int64_t horizontal_rate = pixel_clock_hz / horizontal_total;
	const auto maximum = static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max());
	if (horizontal_blanking > maximum || horizontal_total > 131072LL || vertical_blanking > maximum
		|| vertical_total > 131072LL || pixel_clock_hz <= 0LL || actual_refresh <= 0LL || horizontal_rate <= 0LL)
		return std::nullopt;

	const AxisTiming horizontal = {
		horizontal_active, static_cast<std::uint32_t>(horizontal_front),
		static_cast<std::uint32_t>(horizontal_sync), static_cast<std::uint32_t>(horizontal_back),
		static_cast<std::uint32_t>(horizontal_blanking), static_cast<std::uint32_t>(horizontal_total),
		SyncPolarity::Negative};
	const AxisTiming vertical = {
		vertical_active, static_cast<std::uint32_t>(vertical_front),
		static_cast<std::uint32_t>(vertical_sync), static_cast<std::uint32_t>(vertical_back),
		static_cast<std::uint32_t>(vertical_blanking), static_cast<std::uint32_t>(vertical_total),
		SyncPolarity::Positive};
	return TimingSnapshot(horizontal, vertical, static_cast<std::uint64_t>(pixel_clock_hz),
		static_cast<std::uint64_t>(actual_refresh), static_cast<std::uint64_t>(horizontal_rate),
		scan_mode, TimingType::Gtf, ReducedBlanking::NotReduced);
}

} }
