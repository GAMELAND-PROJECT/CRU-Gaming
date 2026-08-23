#include "TimingAnalyzer.h"

namespace cru { namespace core {
namespace {

std::uint64_t difference(std::uint64_t left, std::uint64_t right) noexcept
{
	return left > right ? left - right : right - left;
}

}

bool TimingAnalysis::internally_consistent() const noexcept
{
	return issues.empty();
}

TimingAnalysis TimingAnalyzer::analyze(const TimingSnapshot &timing)
{
	const auto &horizontal = timing.horizontal();
	const auto &vertical = timing.vertical();
	TimingAnalysis result = {
		static_cast<std::uint64_t>(horizontal.active) * vertical.active,
		static_cast<std::uint64_t>(horizontal.total) * vertical.total,
		0U, 0U, 0U, {}};

	if (horizontal.active == 0U || vertical.active == 0U)
		result.issues.push_back(TimingIssue::ZeroActiveArea);
	if (timing.pixel_clock_hz() == 0U)
		result.issues.push_back(TimingIssue::ZeroPixelClock);
	if (horizontal.blanking != static_cast<std::uint64_t>(horizontal.front_porch)
		+ horizontal.sync_width + horizontal.back_porch)
		result.issues.push_back(TimingIssue::HorizontalBlankingMismatch);
	if (horizontal.total != static_cast<std::uint64_t>(horizontal.active) + horizontal.blanking)
		result.issues.push_back(TimingIssue::HorizontalTotalMismatch);
	if (vertical.blanking != static_cast<std::uint64_t>(vertical.front_porch)
		+ vertical.sync_width + vertical.back_porch)
		result.issues.push_back(TimingIssue::VerticalBlankingMismatch);
	if (vertical.total != static_cast<std::uint64_t>(vertical.active) + vertical.blanking)
		result.issues.push_back(TimingIssue::VerticalTotalMismatch);

	if (result.total_pixels != 0U && result.active_pixels <= result.total_pixels)
		result.active_pixel_ratio_ppm = static_cast<std::uint32_t>(
			static_cast<long double>(result.active_pixels) * 1000000.0L / result.total_pixels);

	if (horizontal.total != 0U)
	{
		result.calculated_horizontal_rate_hz = timing.pixel_clock_hz() / horizontal.total;
		if (result.calculated_horizontal_rate_hz != timing.horizontal_rate_hz())
			result.issues.push_back(TimingIssue::HorizontalRateMismatch);
	}

	if (horizontal.total != 0U && vertical.total != 0U)
	{
		const std::uint64_t vertical_factor = static_cast<std::uint64_t>(vertical.total) * 2U
			+ (timing.interlaced() ? 1U : 0U);
		result.calculated_refresh_rate_millihertz =
			(result.calculated_horizontal_rate_hz * 2000U) / vertical_factor;
		if (difference(result.calculated_refresh_rate_millihertz, timing.refresh_rate_millihertz()) > 100U)
			result.issues.push_back(TimingIssue::RefreshRateMismatch);
	}

	return result;
}

} }
