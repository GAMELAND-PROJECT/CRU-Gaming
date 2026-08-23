#pragma once

#include "MonitorRangeLimits.h"
#include "TimingSnapshot.h"

#include <cstdint>

namespace cru { namespace core {

enum class EstimatedLimitSource
{
	HorizontalScan,
	PixelClock,
	Equal
};

struct RangeCapabilityEstimate
{
	std::uint64_t advertised_maximum_refresh_rate_millihertz;
	std::uint64_t horizontal_scan_ceiling_millihertz;
	std::uint64_t pixel_clock_ceiling_millihertz;
	std::uint64_t estimated_timing_ceiling_millihertz;
	std::uint64_t advertised_timing_ceiling_millihertz;
	EstimatedLimitSource estimated_limit_source;
	bool estimate_exceeds_advertised_vertical_limit;
};

class RangeCapabilityEstimator
{
public:
	static RangeCapabilityEstimate estimate(
		const TimingSnapshot &timing,
		const MonitorRangeLimits &advertised_limits) noexcept;
};

} }
