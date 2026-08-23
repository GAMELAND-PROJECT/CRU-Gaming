#pragma once
#include "TimingSnapshot.h"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace cru { namespace core {
struct CtaDataBlock { std::uint8_t tag; std::vector<std::uint8_t> payload; };
struct Cta861Extension {
	std::uint8_t revision;
	bool underscan, basic_audio, ycbcr444, ycbcr422;
	std::vector<CtaDataBlock> data_blocks;
	std::vector<TimingSnapshot> detailed_timings;
};
class Cta861ExtensionParser {
public:
	using Bytes = std::array<std::uint8_t, 128>;
	static std::optional<Cta861Extension> parse(const Bytes &bytes);
};
} }

