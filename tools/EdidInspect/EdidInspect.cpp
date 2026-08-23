#include "EdidDocument.h"
#include "DisplayCapabilitiesSnapshot.h"
#include "DisplayTimingReport.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace
{
std::string manufacturer_name(std::uint16_t id)
{
	std::string name(3, '?');
	name[0] = static_cast<char>('A' + ((id >> 10U) & 31U) - 1U);
	name[1] = static_cast<char>('A' + ((id >> 5U) & 31U) - 1U);
	name[2] = static_cast<char>('A' + (id & 31U) - 1U);
	return name;
}

const char *scan_mode_name(cru::core::ScanMode mode)
{
	return mode == cru::core::ScanMode::Interlaced ? "interlaced" : "progressive";
}

const char *timing_issue_name(cru::core::TimingIssue issue)
{
	switch (issue)
	{
	case cru::core::TimingIssue::ZeroActiveArea: return "zero_active_area";
	case cru::core::TimingIssue::ZeroPixelClock: return "zero_pixel_clock";
	case cru::core::TimingIssue::HorizontalBlankingMismatch: return "horizontal_blanking_mismatch";
	case cru::core::TimingIssue::HorizontalTotalMismatch: return "horizontal_total_mismatch";
	case cru::core::TimingIssue::VerticalBlankingMismatch: return "vertical_blanking_mismatch";
	case cru::core::TimingIssue::VerticalTotalMismatch: return "vertical_total_mismatch";
	case cru::core::TimingIssue::HorizontalRateMismatch: return "horizontal_rate_mismatch";
	case cru::core::TimingIssue::RefreshRateMismatch: return "refresh_rate_mismatch";
	}
	return "unknown";
}

void write_json(
	const cru::core::DisplayCapabilitiesSnapshot &capabilities,
	const cru::core::DisplayTimingReport &report)
{
	std::cout << "{\n"
		<< "  \"schema_version\": 1,\n"
		<< "  \"display\": {\n"
		<< "    \"manufacturer\": " << std::quoted(manufacturer_name(capabilities.manufacturer_id())) << ",\n"
		<< "    \"manufacturer_id\": " << capabilities.manufacturer_id() << ",\n"
		<< "    \"product_code\": " << capabilities.product_code() << ",\n"
		<< "    \"serial_number\": " << capabilities.serial_number() << ",\n"
		<< "    \"edid_version\": " << static_cast<unsigned>(capabilities.edid_version()) << ",\n"
		<< "    \"edid_revision\": " << static_cast<unsigned>(capabilities.edid_revision()) << ",\n"
		<< "    \"cta_extension_count\": " << capabilities.cta_extension_count() << ",\n"
		<< "    \"unparsed_extension_count\": " << capabilities.unparsed_extension_count() << ",\n"
		<< "    \"advertised_range_limits\": ";
	if (capabilities.range_limits())
	{
		const auto &range = *capabilities.range_limits();
		std::cout << "{\"minimum_vertical_rate_hz\": " << range.minimum_vertical_rate_hz
			<< ", \"maximum_vertical_rate_hz\": " << range.maximum_vertical_rate_hz
			<< ", \"minimum_horizontal_rate_khz\": " << range.minimum_horizontal_rate_khz
			<< ", \"maximum_horizontal_rate_khz\": " << range.maximum_horizontal_rate_khz
			<< ", \"maximum_pixel_clock_hz\": " << range.maximum_pixel_clock_hz << "}\n";
	}
	else std::cout << "null\n";
	std::cout << "  },\n"
		<< "  \"timing_summary\": {\n"
		<< "    \"total\": " << report.timings().size() << ",\n"
		<< "    \"consistent\": " << report.consistent_timing_count() << ",\n"
		<< "    \"inconsistent\": " << report.inconsistent_timing_count() << "\n"
		<< "  },\n"
		<< "  \"detailed_timings\": [\n";

	for (std::size_t index = 0; index < report.timings().size(); ++index)
	{
		const auto &entry = report.timings()[index];
		const auto &timing = entry.timing;
		std::cout << "    {\"index\": " << index + 1U
			<< ", \"horizontal_active\": " << timing.horizontal().active
			<< ", \"vertical_active\": " << timing.vertical().active
			<< ", \"horizontal_total\": " << timing.horizontal().total
			<< ", \"vertical_total\": " << timing.vertical().total
			<< ", \"pixel_clock_hz\": " << timing.pixel_clock_hz()
			<< ", \"refresh_rate_millihertz\": " << timing.refresh_rate_millihertz()
			<< ", \"scan_mode\": \"" << scan_mode_name(timing.scan_mode()) << "\""
			<< ", \"active_pixel_ratio_ppm\": " << entry.analysis.active_pixel_ratio_ppm
			<< ", \"internally_consistent\": " << (entry.analysis.internally_consistent() ? "true" : "false")
			<< ", \"issues\": [";
		for (std::size_t issue_index = 0; issue_index < entry.analysis.issues.size(); ++issue_index)
		{
			if (issue_index != 0U) std::cout << ", ";
			std::cout << '\"' << timing_issue_name(entry.analysis.issues[issue_index]) << '\"';
		}
		std::cout << "]}" << (index + 1U == report.timings().size() ? "\n" : ",\n");
	}

	std::cout << "  ],\n  \"advertised_video_modes\": [\n";
	const auto &modes = capabilities.advertised_video_modes();
	for (std::size_t index = 0; index < modes.size(); ++index)
	{
		const auto &mode = modes[index];
		std::cout << "    {\"vic\": " << static_cast<unsigned>(mode.descriptor.video_identification_code)
			<< ", \"native\": " << (mode.descriptor.native ? "true" : "false")
			<< ", \"catalog_known\": " << (mode.catalog_info ? "true" : "false");
		if (mode.catalog_info)
			std::cout << ", \"horizontal_active\": " << mode.catalog_info->horizontal_active
				<< ", \"vertical_active\": " << mode.catalog_info->vertical_active
				<< ", \"scan_mode\": \"" << scan_mode_name(mode.catalog_info->scan_mode) << "\""
				<< ", \"nominal_refresh_rate_millihertz\": " << mode.catalog_info->nominal_refresh_rate_millihertz;
		std::cout << "}" << (index + 1U == modes.size() ? "\n" : ",\n");
	}
	std::cout << "  ]\n}\n";
}
}

int main(int argc, char *argv[])
{
	const bool json_output = argc == 3 && std::string(argv[1]) == "--json";
	if ((!json_output && argc != 2) || (json_output && argc != 3))
	{
		std::cerr << "Usage: EdidInspect [--json] <edid.bin>\n";
		return 2;
	}
	const char *path = argv[json_output ? 2 : 1];

	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file)
	{
		std::cerr << "Error: could not open file: " << path << '\n';
		return 2;
	}

	const auto size = file.tellg();
	if (size < 128 || size % 128 != 0)
	{
		std::cerr << "Error: EDID size must be a positive multiple of 128 bytes.\n";
		return 2;
	}

	std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
	file.seekg(0);
	file.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
	const auto edid = cru::core::EdidDocumentParser::parse(bytes);
	if (!edid)
	{
		std::cerr << "Error: invalid EDID structure, block count, or checksum.\n";
		return 1;
	}
	const cru::core::DisplayCapabilitiesSnapshot capabilities(*edid);
	const cru::core::DisplayTimingReport timing_report(capabilities);
	if (json_output)
	{
		write_json(capabilities, timing_report);
		return 0;
	}

	std::cout << "Manufacturer: " << manufacturer_name(capabilities.manufacturer_id()) << '\n'
		<< "Product code: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << capabilities.product_code() << std::dec << '\n'
		<< "Serial number: " << capabilities.serial_number() << '\n'
		<< "EDID version: " << static_cast<unsigned>(capabilities.edid_version()) << '.' << static_cast<unsigned>(capabilities.edid_revision()) << '\n'
		<< "CTA extensions: " << capabilities.cta_extension_count() << '\n'
		<< "Unparsed extensions: " << capabilities.unparsed_extension_count() << '\n'
		<< "Detailed timings: " << capabilities.detailed_timings().size() << '\n'
		<< "Consistent timings: " << timing_report.consistent_timing_count() << '\n'
		<< "Inconsistent timings: " << timing_report.inconsistent_timing_count() << '\n'
		<< "CTA advertised video modes: " << capabilities.advertised_video_modes().size() << "\n\n";
	if (capabilities.range_limits())
	{
		const auto &range = *capabilities.range_limits();
		std::cout << "EDID advertised limits (not measured hard limits):\n"
			<< "Vertical: " << range.minimum_vertical_rate_hz << " - " << range.maximum_vertical_rate_hz << " Hz\n"
			<< "Horizontal: " << range.minimum_horizontal_rate_khz << " - " << range.maximum_horizontal_rate_khz << " kHz\n"
			<< "Maximum pixel clock: " << range.maximum_pixel_clock_hz << " Hz\n\n";
	}

	for (std::size_t index = 0; index < timing_report.timings().size(); ++index)
	{
		const auto &entry = timing_report.timings()[index];
		const auto &timing = entry.timing;
		const auto &analysis = entry.analysis;
		std::cout << index + 1 << ": " << timing.horizontal().active << 'x' << timing.vertical().active
			<< (timing.interlaced() ? "i" : "p") << " @ "
			<< timing.refresh_rate_millihertz() / 1000 << '.' << std::setw(3) << std::setfill('0') << timing.refresh_rate_millihertz() % 1000 << " Hz\n"
			<< "   Pixel clock: " << timing.pixel_clock_hz() << " Hz\n"
			<< "   Horizontal: " << timing.horizontal().total << " total, " << timing.horizontal().blanking << " blanking\n"
			<< "   Vertical: " << timing.vertical().total << " total, " << timing.vertical().blanking << " blanking\n"
			<< "   Internal consistency: " << (analysis.internally_consistent() ? "OK" : "FAILED") << '\n'
			<< "   Active pixel ratio: " << analysis.active_pixel_ratio_ppm / 10000U << '.'
			<< std::setw(2) << std::setfill('0') << (analysis.active_pixel_ratio_ppm / 100U) % 100U << "%\n";
	}

	for (const auto &mode : capabilities.advertised_video_modes())
	{
		std::cout << "VIC " << static_cast<unsigned>(mode.descriptor.video_identification_code);
		if (mode.descriptor.native)
			std::cout << " (native)";
		if (mode.catalog_info)
			std::cout << ": " << mode.catalog_info->horizontal_active << 'x' << mode.catalog_info->vertical_active
				<< (mode.catalog_info->scan_mode == cru::core::ScanMode::Interlaced ? 'i' : 'p')
				<< " @ " << mode.catalog_info->nominal_refresh_rate_millihertz / 1000U << " Hz";
		std::cout << '\n';
	}

	return 0;
}
