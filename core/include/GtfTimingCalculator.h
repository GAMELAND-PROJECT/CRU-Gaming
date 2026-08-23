#pragma once

#include "TimingSnapshot.h"

#include <cstdint>
#include <optional>

namespace cru { namespace core {

class GtfTimingCalculator
{
public:
	static std::optional<TimingSnapshot> calculate(
		std::uint32_t horizontal_active,
		std::uint32_t vertical_active,
		std::uint64_t refresh_rate_millihertz,
		ScanMode scan_mode = ScanMode::Progressive) noexcept;
};

} }
