#include "Cta861Extension.h"
#include "DetailedTimingDescriptor.h"
#include <algorithm>

namespace cru { namespace core {
std::optional<Cta861Extension> Cta861ExtensionParser::parse(const Bytes &bytes)
{
	if (bytes[0] != 0x02U) return std::nullopt;
	std::uint32_t sum = 0; for (auto byte : bytes) sum += byte;
	if ((sum & 0xFFU) != 0U) return std::nullopt;
	const std::size_t dtd_offset = bytes[2] == 0U ? 127U : bytes[2];
	if (dtd_offset < 4U || dtd_offset > 127U) return std::nullopt;
	Cta861Extension result = {bytes[1], (bytes[3]&0x80U)!=0, (bytes[3]&0x40U)!=0,
		(bytes[3]&0x20U)!=0, (bytes[3]&0x10U)!=0, {}, {}};
	for (std::size_t offset = 4; offset < dtd_offset; ) {
		const auto header = bytes[offset++]; const std::size_t size = header & 31U;
		if (offset + size > dtd_offset) return std::nullopt;
		CtaDataBlock block = {static_cast<std::uint8_t>(header >> 5U), {}};
		block.payload.assign(bytes.begin() + offset, bytes.begin() + offset + size);
		result.data_blocks.push_back(block); offset += size;
	}
	for (std::size_t offset = dtd_offset; offset + 18U <= 127U; offset += 18U) {
		DetailedTimingDescriptor::Bytes descriptor;
		std::copy_n(bytes.begin() + offset, descriptor.size(), descriptor.begin());
		const auto timing = DetailedTimingDescriptor::parse(descriptor);
		if (timing) result.detailed_timings.push_back(*timing);
	}
	return result;
}
} }

