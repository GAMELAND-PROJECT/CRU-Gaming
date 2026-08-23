#include "DisplayTimingReport.h"

namespace cru { namespace core {

DisplayTimingReport::DisplayTimingReport(const DisplayCapabilitiesSnapshot &capabilities)
	: consistent_timing_count_(0U), inconsistent_timing_count_(0U)
{
	timings_.reserve(capabilities.detailed_timings().size());
	for (const auto &timing : capabilities.detailed_timings())
	{
		auto analysis = TimingAnalyzer::analyze(timing);
		if (analysis.internally_consistent())
			++consistent_timing_count_;
		else
			++inconsistent_timing_count_;
		timings_.push_back({timing, analysis});
	}
}

const std::vector<AnalyzedDisplayTiming> &DisplayTimingReport::timings() const noexcept { return timings_; }
std::size_t DisplayTimingReport::consistent_timing_count() const noexcept { return consistent_timing_count_; }
std::size_t DisplayTimingReport::inconsistent_timing_count() const noexcept { return inconsistent_timing_count_; }
bool DisplayTimingReport::all_timings_consistent() const noexcept { return inconsistent_timing_count_ == 0U; }

} }
