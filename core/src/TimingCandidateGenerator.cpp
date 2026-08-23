#include "TimingCandidateGenerator.h"

#include <limits>

namespace cru { namespace core {
namespace {

std::uint64_t multiply_divide_ceiling(
	std::uint64_t left, std::uint64_t middle, std::uint64_t right, std::uint64_t divisor) noexcept
{
	if (divisor == 0U) return 0U;
	const long double value = static_cast<long double>(left) * middle * right / divisor;
	if (value >= static_cast<long double>(std::numeric_limits<std::uint64_t>::max()))
		return std::numeric_limits<std::uint64_t>::max();
	const auto truncated = static_cast<std::uint64_t>(value);
	return static_cast<long double>(truncated) < value ? truncated + 1U : truncated;
}

TimingCandidate make_candidate(
	const TimingSnapshot &timing,
	const RangeCapabilityEstimate &estimate,
	std::uint64_t refresh_rate_millihertz) noexcept
{
	const std::uint64_t vertical_factor = static_cast<std::uint64_t>(timing.vertical().total) * 2U
		+ (timing.interlaced() ? 1U : 0U);
	const auto pixel_clock = multiply_divide_ceiling(
		refresh_rate_millihertz, timing.horizontal().total, vertical_factor, 2000U);
	const auto horizontal_rate = multiply_divide_ceiling(refresh_rate_millihertz, vertical_factor, 1U, 2000U);
	return {
		refresh_rate_millihertz,
		pixel_clock,
		horizontal_rate,
		refresh_rate_millihertz <= estimate.advertised_timing_ceiling_millihertz
			? TimingCandidateClassification::Advertised
			: TimingCandidateClassification::Experimental,
		pixel_clock <= 655350000U};
}

}

TimingCandidateSet TimingCandidateGenerator::generate(
	const TimingSnapshot &timing,
	const RangeCapabilityEstimate &estimate,
	std::uint64_t minimum_refresh_rate_millihertz,
	std::uint64_t step_millihertz,
	std::size_t maximum_candidates)
{
	TimingCandidateSet result = {{}, false};
	const auto ceiling = estimate.estimated_timing_ceiling_millihertz;
	if (minimum_refresh_rate_millihertz == 0U || step_millihertz == 0U
		|| maximum_candidates == 0U || ceiling < minimum_refresh_rate_millihertz)
		return result;

	std::uint64_t refresh = minimum_refresh_rate_millihertz;
	while (refresh <= ceiling && result.candidates.size() + 1U < maximum_candidates)
	{
		result.candidates.push_back(make_candidate(timing, estimate, refresh));
		if (refresh > std::numeric_limits<std::uint64_t>::max() - step_millihertz) break;
		refresh += step_millihertz;
	}

	if (refresh <= ceiling) result.truncated = true;
	if (result.candidates.empty() || result.candidates.back().refresh_rate_millihertz != ceiling)
		result.candidates.push_back(make_candidate(timing, estimate, ceiling));
	return result;
}

} }
