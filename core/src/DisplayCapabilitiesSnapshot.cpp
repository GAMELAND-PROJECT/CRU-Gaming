#include "DisplayCapabilitiesSnapshot.h"

namespace cru { namespace core {

DisplayCapabilitiesSnapshot::DisplayCapabilitiesSnapshot(const EdidDocument &document)
	: manufacturer_id_(document.base_block.manufacturer_id),
	  product_code_(document.base_block.product_code),
	  serial_number_(document.base_block.serial_number),
	  edid_version_(document.base_block.version),
	  edid_revision_(document.base_block.revision),
	  cta_extension_count_(document.cta_extensions.size()),
	  unparsed_extension_count_(document.unparsed_extensions.size()),
	  detailed_timings_(document.detailed_timings),
	  advertised_video_modes_(document.advertised_video_modes)
{
}

std::uint16_t DisplayCapabilitiesSnapshot::manufacturer_id() const noexcept { return manufacturer_id_; }
std::uint16_t DisplayCapabilitiesSnapshot::product_code() const noexcept { return product_code_; }
std::uint32_t DisplayCapabilitiesSnapshot::serial_number() const noexcept { return serial_number_; }
std::uint8_t DisplayCapabilitiesSnapshot::edid_version() const noexcept { return edid_version_; }
std::uint8_t DisplayCapabilitiesSnapshot::edid_revision() const noexcept { return edid_revision_; }
std::size_t DisplayCapabilitiesSnapshot::cta_extension_count() const noexcept { return cta_extension_count_; }
std::size_t DisplayCapabilitiesSnapshot::unparsed_extension_count() const noexcept { return unparsed_extension_count_; }
const std::vector<TimingSnapshot> &DisplayCapabilitiesSnapshot::detailed_timings() const noexcept { return detailed_timings_; }
const std::vector<CtaAdvertisedVideoMode> &DisplayCapabilitiesSnapshot::advertised_video_modes() const noexcept
{
	return advertised_video_modes_;
}

} }
