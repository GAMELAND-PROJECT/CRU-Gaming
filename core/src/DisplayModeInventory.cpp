#include "DisplayModeInventory.h"

#include <algorithm>

namespace cru { namespace core {
namespace {

bool same_mode(const AdvertisedDisplayMode &left, const AdvertisedDisplayMode &right)
{
	return left.horizontal_active == right.horizontal_active
		&& left.vertical_active == right.vertical_active
		&& left.refresh_rate_millihertz == right.refresh_rate_millihertz
		&& left.scan_mode == right.scan_mode;
}

bool mode_less(const AdvertisedDisplayMode &left, const AdvertisedDisplayMode &right)
{
	if (left.horizontal_active != right.horizontal_active) return left.horizontal_active < right.horizontal_active;
	if (left.vertical_active != right.vertical_active) return left.vertical_active < right.vertical_active;
	if (left.scan_mode != right.scan_mode) return left.scan_mode < right.scan_mode;
	return left.refresh_rate_millihertz < right.refresh_rate_millihertz;
}

}

DisplayModeInventory::DisplayModeInventory(const DisplayCapabilitiesSnapshot &capabilities)
	: unresolved_cta_mode_count_(0U)
{
	for (const auto &timing : capabilities.detailed_timings())
	{
		modes_.push_back({
			timing.horizontal().active, timing.vertical().active,
			timing.refresh_rate_millihertz(), timing.scan_mode(),
			timing.pixel_clock_hz(), true, false, false});
	}

	for (const auto &mode : capabilities.advertised_video_modes())
	{
		if (!mode.catalog_info)
		{
			++unresolved_cta_mode_count_;
			continue;
		}
		const auto &info = *mode.catalog_info;
		AdvertisedDisplayMode candidate = {
			info.horizontal_active, info.vertical_active, info.nominal_refresh_rate_millihertz,
			info.scan_mode, std::nullopt, false, true, mode.descriptor.native};
		const auto existing = std::find_if(modes_.begin(), modes_.end(),
			[&candidate](const AdvertisedDisplayMode &value) { return same_mode(value, candidate); });
		if (existing == modes_.end()) modes_.push_back(candidate);
		else
		{
			existing->from_cta_vic = true;
			existing->native = existing->native || candidate.native;
		}
	}

	std::sort(modes_.begin(), modes_.end(), mode_less);
	std::vector<AdvertisedDisplayMode> unique_modes;
	unique_modes.reserve(modes_.size());
	for (const auto &mode : modes_)
	{
		if (unique_modes.empty() || !same_mode(unique_modes.back(), mode)) unique_modes.push_back(mode);
		else
		{
			auto &existing = unique_modes.back();
			if (!existing.pixel_clock_hz && mode.pixel_clock_hz) existing.pixel_clock_hz = mode.pixel_clock_hz;
			existing.from_detailed_timing = existing.from_detailed_timing || mode.from_detailed_timing;
			existing.from_cta_vic = existing.from_cta_vic || mode.from_cta_vic;
			existing.native = existing.native || mode.native;
		}
	}
	modes_.swap(unique_modes);

	for (const auto &mode : modes_)
	{
		if (resolutions_.empty()
			|| resolutions_.back().horizontal_active != mode.horizontal_active
			|| resolutions_.back().vertical_active != mode.vertical_active
			|| resolutions_.back().scan_mode != mode.scan_mode)
		{
			resolutions_.push_back({
				mode.horizontal_active, mode.vertical_active, mode.scan_mode,
				mode.refresh_rate_millihertz, mode.refresh_rate_millihertz, 1U,
				mode.from_detailed_timing, mode.from_cta_vic, mode.native});
		}
		else
		{
			auto &summary = resolutions_.back();
			summary.minimum_refresh_rate_millihertz = std::min(summary.minimum_refresh_rate_millihertz, mode.refresh_rate_millihertz);
			summary.maximum_refresh_rate_millihertz = std::max(summary.maximum_refresh_rate_millihertz, mode.refresh_rate_millihertz);
			++summary.advertised_mode_count;
			summary.has_detailed_timing = summary.has_detailed_timing || mode.from_detailed_timing;
			summary.has_cta_vic = summary.has_cta_vic || mode.from_cta_vic;
			summary.has_native_cta_mode = summary.has_native_cta_mode || mode.native;
		}
	}
}

const std::vector<AdvertisedDisplayMode> &DisplayModeInventory::modes() const noexcept { return modes_; }
const std::vector<DisplayResolutionSummary> &DisplayModeInventory::resolutions() const noexcept { return resolutions_; }
std::size_t DisplayModeInventory::unresolved_cta_mode_count() const noexcept { return unresolved_cta_mode_count_; }

} }
