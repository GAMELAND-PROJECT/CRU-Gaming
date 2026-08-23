#pragma once

#include "EdidDocument.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace cru { namespace core {

class DisplayCapabilitiesSnapshot
{
public:
	explicit DisplayCapabilitiesSnapshot(const EdidDocument &document);

	std::uint16_t manufacturer_id() const noexcept;
	std::uint16_t product_code() const noexcept;
	std::uint32_t serial_number() const noexcept;
	std::uint8_t edid_version() const noexcept;
	std::uint8_t edid_revision() const noexcept;
	std::size_t cta_extension_count() const noexcept;
	std::size_t unparsed_extension_count() const noexcept;
	const std::vector<TimingSnapshot> &detailed_timings() const noexcept;
	const std::vector<BaseAdvertisedTiming> &base_advertised_timings() const noexcept;
	const std::vector<CtaAdvertisedVideoMode> &advertised_video_modes() const noexcept;
	const std::optional<MonitorRangeLimits> &range_limits() const noexcept;

private:
	std::uint16_t manufacturer_id_;
	std::uint16_t product_code_;
	std::uint32_t serial_number_;
	std::uint8_t edid_version_;
	std::uint8_t edid_revision_;
	std::size_t cta_extension_count_;
	std::size_t unparsed_extension_count_;
	std::vector<TimingSnapshot> detailed_timings_;
	std::vector<BaseAdvertisedTiming> base_advertised_timings_;
	std::vector<CtaAdvertisedVideoMode> advertised_video_modes_;
	std::optional<MonitorRangeLimits> range_limits_;
};

} }
