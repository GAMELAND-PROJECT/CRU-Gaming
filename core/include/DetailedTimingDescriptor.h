#pragma once

#include "TimingSnapshot.h"

#include <array>
#include <cstdint>
#include <optional>

namespace cru
{
namespace core
{

class DetailedTimingDescriptor
{
public:
	using Bytes = std::array<std::uint8_t, 18>;

	static std::optional<TimingSnapshot> parse(const Bytes &bytes) noexcept;
};

}
}

