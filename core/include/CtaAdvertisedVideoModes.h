#pragma once

#include "Cta861Extension.h"
#include "CtaVicCatalog.h"
#include "CtaVideoDataBlock.h"

#include <optional>
#include <vector>

namespace cru { namespace core {

struct CtaAdvertisedVideoMode
{
	ShortVideoDescriptor descriptor;
	std::optional<CtaVicInfo> catalog_info;
};

class CtaAdvertisedVideoModes
{
public:
	static std::optional<std::vector<CtaAdvertisedVideoMode>> decode(const CtaDataBlock &block);
	static std::vector<CtaAdvertisedVideoMode> collect(const Cta861Extension &extension);
};

} }
