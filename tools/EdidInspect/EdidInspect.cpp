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
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Usage: EdidInspect <edid.bin>\n";
		return 2;
	}

	std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
	if (!file)
	{
		std::cerr << "Error: could not open file: " << argv[1] << '\n';
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
