#pragma once

#include "TimingSnapshot.h"

#include <cstdint>
#include <vector>

namespace cru { namespace core {

enum class TimingIssue
{
	ZeroActiveArea,
	ZeroPixelClock,
	HorizontalBlankingMismatch,
	HorizontalTotalMismatch,
	VerticalBlankingMismatch,
	VerticalTotalMismatch,
	HorizontalRateMismatch,
	RefreshRateMismatch
};

struct TimingAnalysis
{
	std::uint64_t active_pixels;
	std::uint64_t total_pixels;
	std::uint32_t active_pixel_ratio_ppm;
	std::uint64_t calculated_horizontal_rate_hz;
	std::uint64_t calculated_refresh_rate_millihertz;
	std::vector<TimingIssue> issues;

	bool internally_consistent() const noexcept;
};

class TimingAnalyzer
{
public:
	static TimingAnalysis analyze(const TimingSnapshot &timing);
};

} }
