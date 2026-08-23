#pragma once

#include "DisplayCapabilitiesSnapshot.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace cru { namespace core {

struct AdvertisedDisplayMode
{
	std::uint32_t horizontal_active;
	std::uint32_t vertical_active;
	std::uint64_t refresh_rate_millihertz;
	ScanMode scan_mode;
	std::optional<std::uint64_t> pixel_clock_hz;
	bool from_detailed_timing;
	bool from_cta_vic;
	bool from_established_timing;
	bool from_standard_timing;
	bool native;
};

struct DisplayResolutionSummary
{
	std::uint32_t horizontal_active;
	std::uint32_t vertical_active;
	ScanMode scan_mode;
	std::uint64_t minimum_refresh_rate_millihertz;
	std::uint64_t maximum_refresh_rate_millihertz;
	std::size_t advertised_mode_count;
	bool has_detailed_timing;
	bool has_cta_vic;
	bool has_established_timing;
	bool has_standard_timing;
	bool has_native_cta_mode;
};

class DisplayModeInventory
{
public:
	explicit DisplayModeInventory(const DisplayCapabilitiesSnapshot &capabilities);

	const std::vector<AdvertisedDisplayMode> &modes() const noexcept;
	const std::vector<DisplayResolutionSummary> &resolutions() const noexcept;
	std::size_t unresolved_cta_mode_count() const noexcept;

private:
	std::vector<AdvertisedDisplayMode> modes_;
	std::vector<DisplayResolutionSummary> resolutions_;
	std::size_t unresolved_cta_mode_count_;
};

} }
