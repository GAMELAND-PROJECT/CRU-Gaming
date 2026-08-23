#pragma once

#include "DisplayCapabilitiesSnapshot.h"
#include "TimingAnalyzer.h"

#include <cstddef>
#include <vector>

namespace cru { namespace core {

struct AnalyzedDisplayTiming
{
	TimingSnapshot timing;
	TimingAnalysis analysis;
};

class DisplayTimingReport
{
public:
	explicit DisplayTimingReport(const DisplayCapabilitiesSnapshot &capabilities);

	const std::vector<AnalyzedDisplayTiming> &timings() const noexcept;
	std::size_t consistent_timing_count() const noexcept;
	std::size_t inconsistent_timing_count() const noexcept;
	bool all_timings_consistent() const noexcept;

private:
	std::vector<AnalyzedDisplayTiming> timings_;
	std::size_t consistent_timing_count_;
	std::size_t inconsistent_timing_count_;
};

} }
