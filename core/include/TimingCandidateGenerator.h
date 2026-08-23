#pragma once

#include "RangeCapabilityEstimator.h"
#include "TimingSnapshot.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cru { namespace core {

enum class TimingCandidateClassification
{
	Advertised,
	Experimental
};

struct TimingCandidate
{
	std::uint64_t refresh_rate_millihertz;
	std::uint64_t required_pixel_clock_hz;
	std::uint64_t required_horizontal_rate_hz;
	TimingCandidateClassification classification;
	bool edid_detailed_timing_representable;
};

struct TimingCandidateSet
{
	std::vector<TimingCandidate> candidates;
	bool truncated;
};

class TimingCandidateGenerator
{
public:
	static TimingCandidateSet generate(
		const TimingSnapshot &timing,
		const RangeCapabilityEstimate &estimate,
		std::uint64_t minimum_refresh_rate_millihertz = 60000U,
		std::uint64_t step_millihertz = 5000U,
		std::size_t maximum_candidates = 512U);
};

} }
