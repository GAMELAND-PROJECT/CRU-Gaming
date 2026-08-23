#include "CtaVideoDataBlock.h"

namespace cru { namespace core {

std::optional<std::vector<ShortVideoDescriptor>> CtaVideoDataBlock::decode(const CtaDataBlock &block)
{
	if (block.tag != 2U)
		return std::nullopt;

	std::vector<ShortVideoDescriptor> descriptors;
	descriptors.reserve(block.payload.size());

	for (const auto byte : block.payload)
	{
		const auto code_without_native = static_cast<std::uint8_t>(byte & 0x7FU);
		const bool native_possible = code_without_native >= 1U && code_without_native <= 64U;
		descriptors.push_back({
			native_possible ? code_without_native : byte,
			native_possible && (byte & 0x80U) != 0U
		});
	}

	return descriptors;
}

} }
