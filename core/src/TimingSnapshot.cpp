#include "TimingSnapshot.h"

#include <utility>

namespace cru
{
namespace core
{

TimingSnapshot::TimingSnapshot(
	AxisTiming horizontal,
	AxisTiming vertical,
	std::uint64_t pixel_clock_hz,
	std::uint64_t refresh_rate_millihertz,
	std::uint64_t horizontal_rate_hz,
	ScanMode scan_mode,
	TimingType timing_type,
	ReducedBlanking reduced_blanking) noexcept
	: horizontal_(std::move(horizontal)),
	  vertical_(std::move(vertical)),
	  pixel_clock_hz_(pixel_clock_hz),
	  refresh_rate_millihertz_(refresh_rate_millihertz),
	  horizontal_rate_hz_(horizontal_rate_hz),
	  scan_mode_(scan_mode),
	  timing_type_(timing_type),
	  reduced_blanking_(reduced_blanking)
{
}

const AxisTiming &TimingSnapshot::horizontal() const noexcept
{
	return horizontal_;
}

const AxisTiming &TimingSnapshot::vertical() const noexcept
{
	return vertical_;
}

std::uint64_t TimingSnapshot::pixel_clock_hz() const noexcept
{
	return pixel_clock_hz_;
}

std::uint64_t TimingSnapshot::refresh_rate_millihertz() const noexcept
{
	return refresh_rate_millihertz_;
}

std::uint64_t TimingSnapshot::horizontal_rate_hz() const noexcept
{
	return horizontal_rate_hz_;
}

ScanMode TimingSnapshot::scan_mode() const noexcept
{
	return scan_mode_;
}

TimingType TimingSnapshot::timing_type() const noexcept
{
	return timing_type_;
}

ReducedBlanking TimingSnapshot::reduced_blanking() const noexcept
{
	return reduced_blanking_;
}

bool TimingSnapshot::interlaced() const noexcept
{
	return scan_mode_ == ScanMode::Interlaced;
}

bool TimingSnapshot::progressive() const noexcept
{
	return scan_mode_ == ScanMode::Progressive;
}

}
}

