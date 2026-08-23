#include "BaseAdvertisedTimings.h"

#include <cstddef>

namespace cru { namespace core {
namespace {

void add_if_set(
	std::vector<BaseAdvertisedTiming> &timings,
	std::uint8_t byte, std::uint8_t mask,
	std::uint32_t horizontal, std::uint32_t vertical, std::uint32_t refresh,
	ScanMode scan_mode = ScanMode::Progressive)
{
	if ((byte & mask) != 0U)
		timings.push_back({horizontal, vertical, refresh * 1000U, scan_mode, BaseTimingSource::Established});
}

std::uint32_t standard_vertical(std::uint32_t horizontal, std::uint8_t aspect, bool edid_1_3_or_later)
{
	if (!edid_1_3_or_later && aspect == 0U) return horizontal;
	switch (aspect)
	{
	case 0U: return horizontal * 10U / 16U;
	case 1U: return horizontal * 3U / 4U;
	case 2U: return horizontal * 4U / 5U;
	default: return horizontal * 9U / 16U;
	}
}

}

std::vector<BaseAdvertisedTiming> BaseAdvertisedTimings::parse_established(const EstablishedBytes &bytes)
{
	std::vector<BaseAdvertisedTiming> timings;
	add_if_set(timings, bytes[0], 0x80U, 720U, 400U, 70U);
	add_if_set(timings, bytes[0], 0x40U, 720U, 400U, 88U);
	add_if_set(timings, bytes[0], 0x20U, 640U, 480U, 60U);
	add_if_set(timings, bytes[0], 0x10U, 640U, 480U, 67U);
	add_if_set(timings, bytes[0], 0x08U, 640U, 480U, 72U);
	add_if_set(timings, bytes[0], 0x04U, 640U, 480U, 75U);
	add_if_set(timings, bytes[0], 0x02U, 800U, 600U, 56U);
	add_if_set(timings, bytes[0], 0x01U, 800U, 600U, 60U);
	add_if_set(timings, bytes[1], 0x80U, 800U, 600U, 72U);
	add_if_set(timings, bytes[1], 0x40U, 800U, 600U, 75U);
	add_if_set(timings, bytes[1], 0x20U, 832U, 624U, 75U);
	add_if_set(timings, bytes[1], 0x10U, 1024U, 768U, 87U, ScanMode::Interlaced);
	add_if_set(timings, bytes[1], 0x08U, 1024U, 768U, 60U);
	add_if_set(timings, bytes[1], 0x04U, 1024U, 768U, 70U);
	add_if_set(timings, bytes[1], 0x02U, 1024U, 768U, 75U);
	add_if_set(timings, bytes[1], 0x01U, 1280U, 1024U, 75U);
	add_if_set(timings, bytes[2], 0x80U, 1152U, 870U, 75U);
	return timings;
}

std::vector<BaseAdvertisedTiming> BaseAdvertisedTimings::parse_standard(
	const StandardBytes &bytes, std::uint8_t edid_version, std::uint8_t edid_revision)
{
	std::vector<BaseAdvertisedTiming> timings;
	const bool edid_1_3_or_later = edid_version > 1U || (edid_version == 1U && edid_revision >= 3U);
	for (std::size_t offset = 0U; offset < bytes.size(); offset += 2U)
	{
		if (bytes[offset] == 0x01U && bytes[offset + 1U] == 0x01U) continue;
		if (bytes[offset] == 0x00U && bytes[offset + 1U] == 0x00U) continue;
		const std::uint32_t horizontal = (static_cast<std::uint32_t>(bytes[offset]) + 31U) * 8U;
		const std::uint8_t aspect = static_cast<std::uint8_t>(bytes[offset + 1U] >> 6U);
		const std::uint32_t refresh = static_cast<std::uint32_t>(bytes[offset + 1U] & 0x3FU) + 60U;
		timings.push_back({
			horizontal, standard_vertical(horizontal, aspect, edid_1_3_or_later),
			refresh * 1000U, ScanMode::Progressive, BaseTimingSource::Standard});
	}
	return timings;
}

} }
