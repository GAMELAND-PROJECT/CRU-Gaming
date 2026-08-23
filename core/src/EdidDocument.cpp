#include "EdidDocument.h"

#include <algorithm>

namespace cru { namespace core {
namespace {

bool checksum_valid(const std::array<std::uint8_t, 128> &block)
{
	std::uint32_t sum = 0;
	for (const auto byte : block)
		sum += byte;
	return (sum & 0xFFU) == 0U;
}

}

std::optional<EdidDocument> EdidDocumentParser::parse(const std::vector<std::uint8_t> &bytes)
{
	if (bytes.size() < 128U || bytes.size() % 128U != 0U)
		return std::nullopt;

	EdidBaseBlockParser::Bytes base_bytes;
	std::copy_n(bytes.begin(), base_bytes.size(), base_bytes.begin());
	const auto base = EdidBaseBlockParser::parse(base_bytes);
	if (!base || bytes.size() != (static_cast<std::size_t>(base->extension_count) + 1U) * 128U)
		return std::nullopt;

	EdidDocument document = {*base, {}, {}, base->detailed_timings, {}};
	for (std::size_t block_index = 1U; block_index <= base->extension_count; ++block_index)
	{
		std::array<std::uint8_t, 128> extension_bytes;
		std::copy_n(bytes.begin() + block_index * 128U, extension_bytes.size(), extension_bytes.begin());
		if (!checksum_valid(extension_bytes))
			return std::nullopt;

		if (extension_bytes[0] != 0x02U)
		{
			document.unparsed_extensions.push_back(extension_bytes);
			continue;
		}

		const auto cta = Cta861ExtensionParser::parse(extension_bytes);
		if (!cta)
			return std::nullopt;

		document.detailed_timings.insert(
			document.detailed_timings.end(), cta->detailed_timings.begin(), cta->detailed_timings.end());
		const auto modes = CtaAdvertisedVideoModes::collect(*cta);
		document.advertised_video_modes.insert(
			document.advertised_video_modes.end(), modes.begin(), modes.end());
		document.cta_extensions.push_back(*cta);
	}

	return document;
}

} }
