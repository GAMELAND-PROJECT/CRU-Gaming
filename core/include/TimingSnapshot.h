#pragma once

#include <cstdint>

namespace cru
{
namespace core
{

enum class SyncPolarity
{
	Negative,
	Positive
};

enum class ScanMode
{
	Progressive,
	Interlaced
};

enum class TimingType
{
	Manual
};

enum class ReducedBlanking
{
	Unknown,
	NotReduced,
	Reduced
};

struct AxisTiming
{
	std::uint32_t active;
	std::uint32_t front_porch;
	std::uint32_t sync_width;
	std::uint32_t back_porch;
	std::uint32_t blanking;
	std::uint32_t total;
	SyncPolarity sync_polarity;
};

class TimingSnapshot
{
public:
	TimingSnapshot(
		AxisTiming horizontal,
		AxisTiming vertical,
		std::uint64_t pixel_clock_hz,
		std::uint64_t refresh_rate_millihertz,
		std::uint64_t horizontal_rate_hz,
		ScanMode scan_mode,
		TimingType timing_type,
		ReducedBlanking reduced_blanking) noexcept;

	const AxisTiming &horizontal() const noexcept;
	const AxisTiming &vertical() const noexcept;
	std::uint64_t pixel_clock_hz() const noexcept;
	std::uint64_t refresh_rate_millihertz() const noexcept;
	std::uint64_t horizontal_rate_hz() const noexcept;
	ScanMode scan_mode() const noexcept;
	TimingType timing_type() const noexcept;
	ReducedBlanking reduced_blanking() const noexcept;
	bool interlaced() const noexcept;
	bool progressive() const noexcept;

private:
	AxisTiming horizontal_;
	AxisTiming vertical_;
	std::uint64_t pixel_clock_hz_;
	std::uint64_t refresh_rate_millihertz_;
	std::uint64_t horizontal_rate_hz_;
	ScanMode scan_mode_;
	TimingType timing_type_;
	ReducedBlanking reduced_blanking_;
};

}
}

