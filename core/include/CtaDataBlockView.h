#pragma once

#include "Cta861Extension.h"

#include <cstdint>
#include <optional>

namespace cru { namespace core {

enum class CtaDataBlockType : std::uint8_t
{
	Reserved = 0,
	Audio = 1,
	Video = 2,
	VendorSpecific = 3,
	SpeakerAllocation = 4,
	VesaDisplayTransfer = 5,
	Extended = 7
};

class CtaDataBlockView
{
public:
	explicit CtaDataBlockView(const CtaDataBlock &block);

	CtaDataBlockType type() const;
	std::optional<std::uint8_t> extended_tag() const;
	const std::vector<std::uint8_t> &payload() const;

private:
	const CtaDataBlock &block_;
};

} }
