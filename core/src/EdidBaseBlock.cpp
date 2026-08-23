#include "EdidBaseBlock.h"
#include "DetailedTimingDescriptor.h"
#include "MonitorRangeLimits.h"
#include "BaseAdvertisedTimings.h"

#include <algorithm>
#include <iterator>

namespace cru { namespace core {

std::optional<EdidBaseBlock> EdidBaseBlockParser::parse(const Bytes &bytes)
{
	static const std::array<std::uint8_t, 8> header = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
	if (!std::equal(header.begin(), header.end(), bytes.begin()))
		return std::nullopt;

	std::uint32_t checksum = 0;
	for (const auto byte : bytes)
		checksum += byte;
	if ((checksum & 0xFFU) != 0U)
		return std::nullopt;

	EdidBaseBlock result = {
		static_cast<std::uint16_t>((bytes[8] << 8U) | bytes[9]),
		static_cast<std::uint16_t>(bytes[10] | (bytes[11] << 8U)),
		static_cast<std::uint32_t>(bytes[12] | (bytes[13] << 8U) | (bytes[14] << 16U) | (bytes[15] << 24U)),
		bytes[18], bytes[19], bytes[126], {}, std::nullopt, {}};
	BaseAdvertisedTimings::EstablishedBytes established = {bytes[35], bytes[36], bytes[37]};
	BaseAdvertisedTimings::StandardBytes standard;
	std::copy_n(bytes.begin() + 38, standard.size(), standard.begin());
	result.base_advertised_timings = BaseAdvertisedTimings::parse_established(established);
	const auto standard_timings = BaseAdvertisedTimings::parse_standard(standard, result.version, result.revision);
	result.base_advertised_timings.insert(
		result.base_advertised_timings.end(), standard_timings.begin(), standard_timings.end());

	for (std::size_t offset = 54; offset < 126; offset += 18)
	{
		DetailedTimingDescriptor::Bytes descriptor;
		std::copy_n(bytes.begin() + offset, descriptor.size(), descriptor.begin());
		const auto timing = DetailedTimingDescriptor::parse(descriptor);
		if (timing)
			result.detailed_timings.push_back(*timing);
		else if (!result.range_limits)
			result.range_limits = MonitorRangeLimitsDescriptor::parse(
				descriptor, result.version > 1U || (result.version == 1U && result.revision >= 4U));
	}

	return result;
}

} }
