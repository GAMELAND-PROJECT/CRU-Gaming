#include "CtaAdvertisedVideoModes.h"

namespace cru { namespace core {

std::optional<std::vector<CtaAdvertisedVideoMode>> CtaAdvertisedVideoModes::decode(const CtaDataBlock &block)
{
	const auto descriptors = CtaVideoDataBlock::decode(block);
	if (!descriptors)
		return std::nullopt;

	std::vector<CtaAdvertisedVideoMode> modes;
	modes.reserve(descriptors->size());
	for (const auto &descriptor : *descriptors)
		modes.push_back({descriptor, CtaVicCatalog::lookup(descriptor.video_identification_code)});

	return modes;
}

std::vector<CtaAdvertisedVideoMode> CtaAdvertisedVideoModes::collect(const Cta861Extension &extension)
{
	std::vector<CtaAdvertisedVideoMode> modes;
	for (const auto &block : extension.data_blocks)
	{
		const auto block_modes = decode(block);
		if (block_modes)
			modes.insert(modes.end(), block_modes->begin(), block_modes->end());
	}

	return modes;
}

} }
