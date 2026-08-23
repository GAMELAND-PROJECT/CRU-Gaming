#include "CtaDataBlockView.h"

namespace cru { namespace core {

CtaDataBlockView::CtaDataBlockView(const CtaDataBlock &block) : block_(block)
{
}

CtaDataBlockType CtaDataBlockView::type() const
{
	if (block_.tag == 6U)
		return CtaDataBlockType::Reserved;

	return static_cast<CtaDataBlockType>(block_.tag);
}

std::optional<std::uint8_t> CtaDataBlockView::extended_tag() const
{
	if (type() != CtaDataBlockType::Extended || block_.payload.empty())
		return std::nullopt;

	return block_.payload.front();
}

const std::vector<std::uint8_t> &CtaDataBlockView::payload() const
{
	return block_.payload;
}

} }
