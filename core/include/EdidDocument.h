#pragma once

#include "Cta861Extension.h"
#include "CtaAdvertisedVideoModes.h"
#include "EdidBaseBlock.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace cru { namespace core {

struct EdidDocument
{
	EdidBaseBlock base_block;
	std::vector<Cta861Extension> cta_extensions;
	std::vector<std::array<std::uint8_t, 128>> unparsed_extensions;
	std::vector<TimingSnapshot> detailed_timings;
	std::vector<CtaAdvertisedVideoMode> advertised_video_modes;
};

class EdidDocumentParser
{
public:
	static std::optional<EdidDocument> parse(const std::vector<std::uint8_t> &bytes);
};

} }
