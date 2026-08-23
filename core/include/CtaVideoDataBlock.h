#pragma once

#include "Cta861Extension.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace cru { namespace core {

struct ShortVideoDescriptor
{
	std::uint8_t video_identification_code;
	bool native;
};

class CtaVideoDataBlock
{
public:
	static std::optional<std::vector<ShortVideoDescriptor>> decode(const CtaDataBlock &block);
};

} }
