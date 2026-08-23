#include "RangeCapabilityEstimator.h"

#include <algorithm>

namespace cru { namespace core {

RangeCapabilityEstimate RangeCapabilityEstimator::estimate(
	const TimingSnapshot &timing,
	const MonitorRangeLimits &advertised_limits) noexcept
{
	const std::uint64_t horizontal_total = timing.horizontal().total;
	const std::uint64_t vertical_factor = static_cast<std::uint64_t>(timing.vertical().total) * 2U
		+ (timing.interlaced() ? 1U : 0U);
	const std::uint64_t advertised_maximum =
		static_cast<std::uint64_t>(advertised_limits.maximum_vertical_rate_hz) * 1000U;

	std::uint64_t horizontal_ceiling = 0U;
	std::uint64_t pixel_clock_ceiling = 0U;
	if (vertical_factor != 0U)
	{
		horizontal_ceiling = static_cast<std::uint64_t>(advertised_limits.maximum_horizontal_rate_khz)
			* 1000U * 2000U / vertical_factor;
		if (horizontal_total != 0U)
			pixel_clock_ceiling = advertised_limits.maximum_pixel_clock_hz
				* 2000U / horizontal_total / vertical_factor;
	}

	const std::uint64_t estimated = std::min(horizontal_ceiling, pixel_clock_ceiling);
	EstimatedLimitSource source = EstimatedLimitSource::Equal;
	if (horizontal_ceiling < pixel_clock_ceiling) source = EstimatedLimitSource::HorizontalScan;
	else if (pixel_clock_ceiling < horizontal_ceiling) source = EstimatedLimitSource::PixelClock;

	return {
		advertised_maximum,
		horizontal_ceiling,
		pixel_clock_ceiling,
		estimated,
		std::min(estimated, advertised_maximum),
		source,
		estimated > advertised_maximum};
}

} }
