#include "DetailedTimingDescriptor.h"
#include "DisplayCapabilitiesSnapshot.h"
#include "DisplayTimingReport.h"
#include "DisplayModeInventory.h"
#include "EdidBaseBlock.h"
#include "EdidDocument.h"
#include "Cta861Extension.h"
#include "CtaAdvertisedVideoModes.h"
#include "CtaDataBlockView.h"
#include "CtaVideoDataBlock.h"
#include "CtaVicCatalog.h"
#include "TimingAnalyzer.h"
#include "MonitorRangeLimits.h"
#include "RangeCapabilityEstimator.h"
#include "TimingCandidateGenerator.h"
#include "BaseAdvertisedTimings.h"
#include "GtfTimingCalculator.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iterator>

namespace
{

using cru::core::DetailedTimingDescriptor;
using cru::core::AxisTiming;
using cru::core::ReducedBlanking;
using cru::core::ScanMode;
using cru::core::SyncPolarity;
using cru::core::TimingSnapshot;
using cru::core::TimingType;

struct ExpectedTiming
{
	const char *name;
	DetailedTimingDescriptor::Bytes bytes;
	std::uint32_t horizontal_active;
	std::uint32_t horizontal_front;
	std::uint32_t horizontal_sync;
	std::uint32_t horizontal_back;
	std::uint32_t horizontal_blanking;
	std::uint32_t horizontal_total;
	SyncPolarity horizontal_polarity;
	std::uint32_t vertical_active;
	std::uint32_t vertical_front;
	std::uint32_t vertical_sync;
	std::uint32_t vertical_back;
	std::uint32_t vertical_blanking;
	std::uint32_t vertical_total;
	SyncPolarity vertical_polarity;
	std::uint64_t pixel_clock_hz;
	std::uint64_t refresh_rate_millihertz;
	std::uint64_t horizontal_rate_hz;
	ScanMode scan_mode;
};

int failures = 0;

void check_equal(
	const char *test,
	const char *field,
	std::uint64_t expected,
	std::uint64_t actual)
{
	if (expected == actual)
		return;

	std::printf("%s: %s: expected %llu, got %llu\n", test, field, expected, actual);
	++failures;
}

template <typename T>
void check_equal(const char *test, const char *field, T expected, T actual)
{
	check_equal(
		test,
		field,
		static_cast<std::uint64_t>(expected),
		static_cast<std::uint64_t>(actual));
}

void check_timing(const ExpectedTiming &expected)
{
	const auto parsed = DetailedTimingDescriptor::parse(expected.bytes);

	if (!parsed)
	{
		std::printf("%s: parser rejected a valid descriptor\n", expected.name);
		++failures;
		return;
	}

	const TimingSnapshot &snapshot = *parsed;
	const auto &horizontal = snapshot.horizontal();
	const auto &vertical = snapshot.vertical();

	check_equal(expected.name, "horizontal.active", expected.horizontal_active, horizontal.active);
	check_equal(expected.name, "horizontal.front_porch", expected.horizontal_front, horizontal.front_porch);
	check_equal(expected.name, "horizontal.sync_width", expected.horizontal_sync, horizontal.sync_width);
	check_equal(expected.name, "horizontal.back_porch", expected.horizontal_back, horizontal.back_porch);
	check_equal(expected.name, "horizontal.blanking", expected.horizontal_blanking, horizontal.blanking);
	check_equal(expected.name, "horizontal.total", expected.horizontal_total, horizontal.total);
	check_equal(expected.name, "horizontal.sync_polarity", expected.horizontal_polarity, horizontal.sync_polarity);
	check_equal(expected.name, "vertical.active", expected.vertical_active, vertical.active);
	check_equal(expected.name, "vertical.front_porch", expected.vertical_front, vertical.front_porch);
	check_equal(expected.name, "vertical.sync_width", expected.vertical_sync, vertical.sync_width);
	check_equal(expected.name, "vertical.back_porch", expected.vertical_back, vertical.back_porch);
	check_equal(expected.name, "vertical.blanking", expected.vertical_blanking, vertical.blanking);
	check_equal(expected.name, "vertical.total", expected.vertical_total, vertical.total);
	check_equal(expected.name, "vertical.sync_polarity", expected.vertical_polarity, vertical.sync_polarity);
	check_equal(expected.name, "pixel_clock_hz", expected.pixel_clock_hz, snapshot.pixel_clock_hz());
	check_equal(expected.name, "refresh_rate_millihertz", expected.refresh_rate_millihertz, snapshot.refresh_rate_millihertz());
	check_equal(expected.name, "horizontal_rate_hz", expected.horizontal_rate_hz, snapshot.horizontal_rate_hz());
	check_equal(expected.name, "scan_mode", expected.scan_mode, snapshot.scan_mode());
	check_equal(expected.name, "timing_type", TimingType::Manual, snapshot.timing_type());
	check_equal(expected.name, "reduced_blanking", ReducedBlanking::Unknown, snapshot.reduced_blanking());
	check_equal(expected.name, "interlaced", expected.scan_mode == ScanMode::Interlaced, snapshot.interlaced());
	check_equal(expected.name, "progressive", expected.scan_mode == ScanMode::Progressive, snapshot.progressive());
	const auto analysis = cru::core::TimingAnalyzer::analyze(snapshot);
	check_equal(expected.name, "internally consistent", true, analysis.internally_consistent());
}

void check_timing_analyzer()
{
	const AxisTiming horizontal = {1920U, 88U, 44U, 148U, 280U, 2200U, SyncPolarity::Positive};
	const AxisTiming vertical = {1080U, 4U, 5U, 36U, 45U, 1125U, SyncPolarity::Positive};
	const TimingSnapshot timing(horizontal, vertical, 148500000U, 60000U, 67500U,
		ScanMode::Progressive, TimingType::Manual, ReducedBlanking::Unknown);
	const auto analysis = cru::core::TimingAnalyzer::analyze(timing);
	check_equal("timing analyzer", "consistent", true, analysis.internally_consistent());
	check_equal("timing analyzer", "active pixels", 2073600U, analysis.active_pixels);
	check_equal("timing analyzer", "total pixels", 2475000U, analysis.total_pixels);
	check_equal("timing analyzer", "active ratio ppm", 837818U, analysis.active_pixel_ratio_ppm);
	check_equal("timing analyzer", "calculated horizontal rate", 67500U, analysis.calculated_horizontal_rate_hz);
	check_equal("timing analyzer", "calculated refresh", 60000U, analysis.calculated_refresh_rate_millihertz);
	const cru::core::MonitorRangeLimits crt_limits = {
		48U, 144U, 30U, 240U, 600000000U, cru::core::SecondaryTimingFormula::NoTimingFormula};
	const auto estimate = cru::core::RangeCapabilityEstimator::estimate(timing, crt_limits);
	check_equal("range estimator", "advertised maximum", 144000U, estimate.advertised_maximum_refresh_rate_millihertz);
	check_equal("range estimator", "horizontal ceiling", 213333U, estimate.horizontal_scan_ceiling_millihertz);
	check_equal("range estimator", "pixel clock ceiling", 242424U, estimate.pixel_clock_ceiling_millihertz);
	check_equal("range estimator", "estimated ceiling", 213333U, estimate.estimated_timing_ceiling_millihertz);
	check_equal("range estimator", "EDID-bounded ceiling", 144000U, estimate.advertised_timing_ceiling_millihertz);
	check_equal("range estimator", "limiting source", cru::core::EstimatedLimitSource::HorizontalScan,
		estimate.estimated_limit_source);
	check_equal("range estimator", "exceeds advertised", true, estimate.estimate_exceeds_advertised_vertical_limit);
	const auto candidates = cru::core::TimingCandidateGenerator::generate(timing, estimate);
	check_equal("timing candidates", "count", 32U, candidates.candidates.size());
	check_equal("timing candidates", "truncated", false, candidates.truncated);
	if (candidates.candidates.size() == 32U) {
		check_equal("timing candidates", "first refresh", 60000U, candidates.candidates.front().refresh_rate_millihertz);
		check_equal("timing candidates", "first pixel clock", 148500000U, candidates.candidates.front().required_pixel_clock_hz);
		check_equal("timing candidates", "first horizontal rate", 67500U, candidates.candidates.front().required_horizontal_rate_hz);
		check_equal("timing candidates", "first classification", cru::core::TimingCandidateClassification::Advertised,
			candidates.candidates.front().classification);
		check_equal("timing candidates", "145 Hz classification", cru::core::TimingCandidateClassification::Experimental,
			candidates.candidates[17].classification);
		check_equal("timing candidates", "ceiling refresh", 213333U, candidates.candidates.back().refresh_rate_millihertz);
		check_equal("timing candidates", "ceiling representable", true,
			candidates.candidates.back().edid_detailed_timing_representable);
	}
	const auto truncated_candidates = cru::core::TimingCandidateGenerator::generate(timing, estimate, 60000U, 5000U, 3U);
	check_equal("timing candidates truncated", "count", 3U, truncated_candidates.candidates.size());
	check_equal("timing candidates truncated", "truncated", true, truncated_candidates.truncated);
	check_equal("timing candidates invalid step", "count", 0U,
		cru::core::TimingCandidateGenerator::generate(timing, estimate, 60000U, 0U).candidates.size());

	const cru::core::MonitorRangeLimits pixel_limited = {
		48U, 85U, 30U, 255U, 150000000U, cru::core::SecondaryTimingFormula::DefaultGtf};
	const auto pixel_estimate = cru::core::RangeCapabilityEstimator::estimate(timing, pixel_limited);
	check_equal("range estimator pixel", "estimated ceiling", 60606U, pixel_estimate.estimated_timing_ceiling_millihertz);
	check_equal("range estimator pixel", "limiting source", cru::core::EstimatedLimitSource::PixelClock,
		pixel_estimate.estimated_limit_source);
	check_equal("range estimator pixel", "exceeds advertised", false,
		pixel_estimate.estimate_exceeds_advertised_vertical_limit);

	const AxisTiming bad_horizontal = {1920U, 88U, 44U, 147U, 280U, 2200U, SyncPolarity::Positive};
	const AxisTiming bad_vertical = {1080U, 4U, 5U, 36U, 45U, 1124U, SyncPolarity::Positive};
	const TimingSnapshot malformed(bad_horizontal, bad_vertical, 0U, 0U, 0U,
		ScanMode::Progressive, TimingType::Manual, ReducedBlanking::Unknown);
	const auto malformed_analysis = cru::core::TimingAnalyzer::analyze(malformed);
	check_equal("timing analyzer malformed", "consistent", false, malformed_analysis.internally_consistent());
	check_equal("timing analyzer malformed", "issue count", 3U, malformed_analysis.issues.size());
}

void check_invalid_descriptors()
{
	DetailedTimingDescriptor::Bytes zero_clock = {};
	check_equal("zero pixel clock", "accepted", false, DetailedTimingDescriptor::parse(zero_clock).has_value());

	DetailedTimingDescriptor::Bytes invalid_blanking = {
		0x01, 0x00, 0x80, 0x01, 0x70, 0x38, 0x01, 0x40, 0x02,
		0x01, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E};
	check_equal("porches exceed blanking", "accepted", false, DetailedTimingDescriptor::parse(invalid_blanking).has_value());
}

void check_base_edid()
{
	cru::core::EdidBaseBlockParser::Bytes bytes = {};
	const std::uint8_t header[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
	std::copy(std::begin(header), std::end(header), bytes.begin());
	bytes[8] = 0x10; bytes[9] = 0xAC; bytes[10] = 0x34; bytes[11] = 0x12;
	bytes[18] = 1; bytes[19] = 4;
	const DetailedTimingDescriptor::Bytes dtd = {0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C, 0x45, 0, 0, 0, 0, 0, 0, 0x1E};
	std::copy(dtd.begin(), dtd.end(), bytes.begin() + 54);
	std::uint32_t sum = 0;
	for (std::size_t index = 0; index < 127; ++index) sum += bytes[index];
	bytes[127] = static_cast<std::uint8_t>(0U - sum);
	const auto parsed = cru::core::EdidBaseBlockParser::parse(bytes);
	check_equal("base EDID", "accepted", true, parsed.has_value());
	if (parsed)
	{
		check_equal("base EDID", "manufacturer ID", 0x10ACU, parsed->manufacturer_id);
		check_equal("base EDID", "product code", 0x1234U, parsed->product_code);
		check_equal("base EDID", "version", 1U, parsed->version);
		check_equal("base EDID", "revision", 4U, parsed->revision);
		check_equal("base EDID", "extension count", 0U, parsed->extension_count);
		check_equal("base EDID", "DTD count", 1U, parsed->detailed_timings.size());
		check_equal("base EDID", "DTD width", 1920U, parsed->detailed_timings[0].horizontal().active);
		check_equal("base EDID", "range limits", false, parsed->range_limits.has_value());
	}

	auto with_range = bytes;
	with_range[75] = 0xFDU;
	with_range[77] = 48U;
	with_range[78] = 144U;
	with_range[79] = 30U;
	with_range[80] = 240U;
	with_range[81] = 60U;
	with_range[82] = 1U;
	with_range[127] = 0U;
	sum = 0U; for (std::size_t index = 0U; index < 127U; ++index) sum += with_range[index];
	with_range[127] = static_cast<std::uint8_t>(0U - sum);
	const auto ranged = cru::core::EdidBaseBlockParser::parse(with_range);
	check_equal("base EDID range", "accepted", true, ranged.has_value());
	if (ranged && ranged->range_limits) {
		check_equal("base EDID range", "minimum vertical", 48U, ranged->range_limits->minimum_vertical_rate_hz);
		check_equal("base EDID range", "maximum vertical", 144U, ranged->range_limits->maximum_vertical_rate_hz);
		check_equal("base EDID range", "minimum horizontal", 30U, ranged->range_limits->minimum_horizontal_rate_khz);
		check_equal("base EDID range", "maximum horizontal", 240U, ranged->range_limits->maximum_horizontal_rate_khz);
		check_equal("base EDID range", "maximum pixel clock", 600000000U, ranged->range_limits->maximum_pixel_clock_hz);
		check_equal("base EDID range", "formula", cru::core::SecondaryTimingFormula::NoTimingFormula,
			ranged->range_limits->secondary_timing_formula);
	}
	const std::vector<std::uint8_t> ranged_document_bytes(with_range.begin(), with_range.end());
	const auto ranged_document = cru::core::EdidDocumentParser::parse(ranged_document_bytes);
	check_equal("range snapshot path", "document accepted", true, ranged_document.has_value());
	if (ranged_document) {
		const cru::core::DisplayCapabilitiesSnapshot ranged_snapshot(*ranged_document);
		check_equal("range snapshot path", "range present", true, ranged_snapshot.range_limits().has_value());
		if (ranged_snapshot.range_limits())
			check_equal("range snapshot path", "maximum vertical", 144U,
				ranged_snapshot.range_limits()->maximum_vertical_rate_hz);
	}

	const cru::core::MonitorRangeLimitsDescriptor::Bytes extended_range = {
		0U, 0U, 0U, 0xFDU, 0x0FU, 5U, 10U, 15U, 20U, 30U, 4U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
	const auto extended = cru::core::MonitorRangeLimitsDescriptor::parse(extended_range, true);
	check_equal("extended range", "accepted", true, extended.has_value());
	if (extended) {
		check_equal("extended range", "minimum vertical", 260U, extended->minimum_vertical_rate_hz);
		check_equal("extended range", "maximum vertical", 265U, extended->maximum_vertical_rate_hz);
		check_equal("extended range", "minimum horizontal", 270U, extended->minimum_horizontal_rate_khz);
		check_equal("extended range", "maximum horizontal", 275U, extended->maximum_horizontal_rate_khz);
		check_equal("extended range", "formula", cru::core::SecondaryTimingFormula::Cvt, extended->secondary_timing_formula);
	}

	auto bad_header = bytes;
	bad_header[0] = 1U;
	bad_header[127] = static_cast<std::uint8_t>(bad_header[127] - 1U);
	check_equal("bad header", "accepted", false, cru::core::EdidBaseBlockParser::parse(bad_header).has_value());

	bytes[127] ^= 1U;
	check_equal("bad checksum", "accepted", false, cru::core::EdidBaseBlockParser::parse(bytes).has_value());
}

void check_base_advertised_timings()
{
	const auto established = cru::core::BaseAdvertisedTimings::parse_established({0x24U, 0x10U, 0x80U});
	check_equal("established timings", "count", 4U, established.size());
	if (established.size() == 4U) {
		check_equal("established timings", "first width", 640U, established[0].horizontal_active);
		check_equal("established timings", "first refresh", 60000U, established[0].refresh_rate_millihertz);
		check_equal("established timings", "interlaced", ScanMode::Interlaced, established[2].scan_mode);
		check_equal("established timings", "last width", 1152U, established[3].horizontal_active);
	}

	cru::core::EdidBaseBlockParser::Bytes bytes = {};
	const std::uint8_t header[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
	std::copy(std::begin(header), std::end(header), bytes.begin());
	bytes[8] = 0x41U; bytes[9] = 0xD0U; bytes[10] = 0xFFU; bytes[11] = 0x09U;
	bytes[18] = 1U; bytes[19] = 3U;
	const std::array<std::uint8_t, 16> standard = {
		0x61U, 0x59U, 0x45U, 0x72U, 0x31U, 0x7EU,
		0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U};
	std::copy(standard.begin(), standard.end(), bytes.begin() + 38);
	std::uint32_t sum = 0U;
	for (std::size_t index = 0U; index < 127U; ++index) sum += bytes[index];
	bytes[127] = static_cast<std::uint8_t>(0U - sum);

	const std::vector<std::uint8_t> document_bytes(bytes.begin(), bytes.end());
	const auto document = cru::core::EdidDocumentParser::parse(document_bytes);
	check_equal("PNP standard timing path", "accepted", true, document.has_value());
	if (document) {
		const cru::core::DisplayCapabilitiesSnapshot snapshot(*document);
		const cru::core::DisplayModeInventory inventory(snapshot);
		check_equal("PNP standard timing path", "snapshot count", 3U, snapshot.base_advertised_timings().size());
		check_equal("PNP standard timing path", "inventory count", 3U, inventory.modes().size());
		if (inventory.modes().size() == 3U) {
			check_equal("PNP standard timing path", "640 width", 640U, inventory.modes()[0].horizontal_active);
			check_equal("PNP standard timing path", "640 refresh", 122000U, inventory.modes()[0].refresh_rate_millihertz);
			check_equal("PNP standard timing path", "800 width", 800U, inventory.modes()[1].horizontal_active);
			check_equal("PNP standard timing path", "800 refresh", 110000U, inventory.modes()[1].refresh_rate_millihertz);
			check_equal("PNP standard timing path", "1024 width", 1024U, inventory.modes()[2].horizontal_active);
			check_equal("PNP standard timing path", "1024 refresh", 85000U, inventory.modes()[2].refresh_rate_millihertz);
			check_equal("PNP standard timing path", "source", true, inventory.modes()[2].from_standard_timing);
			check_equal("PNP standard timing path", "pixel clock absent", false,
				inventory.modes()[2].pixel_clock_hz.has_value());
		}
	}
}

void check_gtf_timing_calculator()
{
	struct GtfExpected {
		std::uint32_t horizontal_active;
		std::uint32_t vertical_active;
		std::uint64_t requested_refresh;
		std::uint32_t horizontal_front;
		std::uint32_t horizontal_sync;
		std::uint32_t horizontal_back;
		std::uint32_t horizontal_total;
		std::uint32_t vertical_back;
		std::uint32_t vertical_total;
		std::uint64_t pixel_clock_hz;
		std::uint64_t actual_refresh;
		std::uint64_t horizontal_rate;
	};
	const std::array<GtfExpected, 3> fixtures = {{
		{640U, 480U, 122000U, 40U, 64U, 104U, 848U, 32U, 516U, 53380000U, 121992U, 62948U},
		{800U, 600U, 110000U, 48U, 88U, 136U, 1072U, 36U, 640U, 75470000U, 110001U, 70401U},
		{1024U, 768U, 85000U, 64U, 112U, 176U, 1376U, 35U, 807U, 94390000U, 85002U, 68597U}
	}};
	for (const auto &expected : fixtures) {
		const auto timing = cru::core::GtfTimingCalculator::calculate(
			expected.horizontal_active, expected.vertical_active, expected.requested_refresh);
		check_equal("GTF legacy parity", "calculated", true, timing.has_value());
		if (!timing) continue;
		check_equal("GTF legacy parity", "horizontal front", expected.horizontal_front, timing->horizontal().front_porch);
		check_equal("GTF legacy parity", "horizontal sync", expected.horizontal_sync, timing->horizontal().sync_width);
		check_equal("GTF legacy parity", "horizontal back", expected.horizontal_back, timing->horizontal().back_porch);
		check_equal("GTF legacy parity", "horizontal total", expected.horizontal_total, timing->horizontal().total);
		check_equal("GTF legacy parity", "horizontal polarity", SyncPolarity::Negative, timing->horizontal().sync_polarity);
		check_equal("GTF legacy parity", "vertical front", 1U, timing->vertical().front_porch);
		check_equal("GTF legacy parity", "vertical sync", 3U, timing->vertical().sync_width);
		check_equal("GTF legacy parity", "vertical back", expected.vertical_back, timing->vertical().back_porch);
		check_equal("GTF legacy parity", "vertical total", expected.vertical_total, timing->vertical().total);
		check_equal("GTF legacy parity", "vertical polarity", SyncPolarity::Positive, timing->vertical().sync_polarity);
		check_equal("GTF legacy parity", "pixel clock", expected.pixel_clock_hz, timing->pixel_clock_hz());
		check_equal("GTF legacy parity", "actual refresh", expected.actual_refresh, timing->refresh_rate_millihertz());
		check_equal("GTF legacy parity", "horizontal rate", expected.horizontal_rate, timing->horizontal_rate_hz());
		check_equal("GTF legacy parity", "timing type", TimingType::Gtf, timing->timing_type());
		check_equal("GTF legacy parity", "reduced blanking", ReducedBlanking::NotReduced, timing->reduced_blanking());
		check_equal("GTF legacy parity", "consistent", true,
			cru::core::TimingAnalyzer::analyze(*timing).internally_consistent());
	}
	check_equal("GTF invalid", "zero active rejected", false,
		cru::core::GtfTimingCalculator::calculate(0U, 480U, 60000U).has_value());
	check_equal("GTF invalid", "zero refresh rejected", false,
		cru::core::GtfTimingCalculator::calculate(640U, 480U, 0U).has_value());
}

void check_cta_extension()
{
	cru::core::Cta861ExtensionParser::Bytes bytes = {};
	bytes[0] = 0x02; bytes[1] = 0x03; bytes[2] = 7; bytes[3] = 0x70;
	bytes[4] = 0x42; bytes[5] = 16; bytes[6] = 31;
	const DetailedTimingDescriptor::Bytes dtd = {0x02,0x3A,0x80,0x18,0x71,0x38,0x2D,0x40,0x58,0x2C,0x45,0,0,0,0,0,0,0x1E};
	std::copy(dtd.begin(), dtd.end(), bytes.begin() + 7);
	std::uint32_t sum = 0; for (std::size_t i = 0; i < 127; ++i) sum += bytes[i];
	bytes[127] = static_cast<std::uint8_t>(0U - sum);
	const auto parsed = cru::core::Cta861ExtensionParser::parse(bytes);
	check_equal("CTA", "accepted", true, parsed.has_value());
	if (parsed) {
		check_equal("CTA", "revision", 3U, parsed->revision);
		check_equal("CTA", "basic audio", true, parsed->basic_audio);
		check_equal("CTA", "YCbCr 4:4:4", true, parsed->ycbcr444);
		check_equal("CTA", "blocks", 1U, parsed->data_blocks.size());
		check_equal("CTA", "block tag", 2U, parsed->data_blocks[0].tag);
		check_equal("CTA", "block payload", 2U, parsed->data_blocks[0].payload.size());
		const auto video = cru::core::CtaVideoDataBlock::decode(parsed->data_blocks[0]);
		check_equal("CTA video", "decoded", true, video.has_value());
		if (video) {
			check_equal("CTA video", "SVD count", 2U, video->size());
			check_equal("CTA video", "first VIC", 16U, (*video)[0].video_identification_code);
			check_equal("CTA video", "first native", false, (*video)[0].native);
			check_equal("CTA video", "second VIC", 31U, (*video)[1].video_identification_code);
		}
		check_equal("CTA", "DTDs", 1U, parsed->detailed_timings.size());
		const auto advertised = cru::core::CtaAdvertisedVideoModes::collect(*parsed);
		check_equal("CTA advertised", "mode count", 2U, advertised.size());
		if (advertised.size() == 2U) {
			check_equal("CTA advertised", "first VIC", 16U, advertised[0].descriptor.video_identification_code);
			check_equal("CTA advertised", "first known", true, advertised[0].catalog_info.has_value());
			if (advertised[0].catalog_info) {
				check_equal("CTA advertised", "first width", 1920U, advertised[0].catalog_info->horizontal_active);
				check_equal("CTA advertised", "first height", 1080U, advertised[0].catalog_info->vertical_active);
			}
		}
	}
	const cru::core::CtaDataBlock native_video = {2U, {0x90U, 0xC1U}};
	const auto native_descriptors = cru::core::CtaVideoDataBlock::decode(native_video);
	check_equal("CTA native video", "decoded", true, native_descriptors.has_value());
	if (native_descriptors) {
		check_equal("CTA native video", "native VIC", 16U, (*native_descriptors)[0].video_identification_code);
		check_equal("CTA native video", "native flag", true, (*native_descriptors)[0].native);
		check_equal("CTA native video", "extended VIC", 193U, (*native_descriptors)[1].video_identification_code);
		check_equal("CTA native video", "extended native flag", false, (*native_descriptors)[1].native);
	}
	const cru::core::CtaDataBlock audio = {1U, {0x90U}};
	check_equal("CTA non-video", "decoded", false, cru::core::CtaVideoDataBlock::decode(audio).has_value());
	auto bad_tag = bytes; bad_tag[0] = 0x03; bad_tag[127] = static_cast<std::uint8_t>(bad_tag[127] - 1U);
	check_equal("CTA tag", "accepted", false, cru::core::Cta861ExtensionParser::parse(bad_tag).has_value());
	auto bad_checksum = bytes; bad_checksum[127] ^= 1U;
	check_equal("CTA checksum", "accepted", false, cru::core::Cta861ExtensionParser::parse(bad_checksum).has_value());
	bytes[4] = 0x5F; sum = 0; for (std::size_t i = 0; i < 127; ++i) sum += bytes[i]; bytes[127] = static_cast<std::uint8_t>(0U - sum);
	check_equal("CTA overrun", "accepted", false, cru::core::Cta861ExtensionParser::parse(bytes).has_value());
}

void check_cta_advertised_video_modes()
{
	const cru::core::CtaDataBlock video = {2U, {0x90U, 0xC1U, 0x80U}};
	const auto modes = cru::core::CtaAdvertisedVideoModes::decode(video);
	check_equal("CTA advertised direct", "decoded", true, modes.has_value());
	if (modes && modes->size() == 3U) {
		check_equal("CTA advertised direct", "mode count", 3U, modes->size());
		check_equal("CTA advertised direct", "native flag", true, (*modes)[0].descriptor.native);
		check_equal("CTA advertised direct", "native VIC", 16U, (*modes)[0].descriptor.video_identification_code);
		check_equal("CTA advertised direct", "extended VIC", 193U, (*modes)[1].descriptor.video_identification_code);
		check_equal("CTA advertised direct", "extended known", true, (*modes)[1].catalog_info.has_value());
		check_equal("CTA advertised direct", "unknown VIC", 128U, (*modes)[2].descriptor.video_identification_code);
		check_equal("CTA advertised direct", "unknown retained", false, (*modes)[2].catalog_info.has_value());
	}

	const cru::core::CtaDataBlock audio = {1U, {0x90U}};
	check_equal("CTA advertised audio", "decoded", false,
		cru::core::CtaAdvertisedVideoModes::decode(audio).has_value());
}

void check_edid_document()
{
	cru::core::EdidBaseBlockParser::Bytes base = {};
	const std::uint8_t header[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
	std::copy(std::begin(header), std::end(header), base.begin());
	base[8] = 0x10U; base[9] = 0xACU; base[18] = 1U; base[19] = 4U; base[126] = 1U;
	const DetailedTimingDescriptor::Bytes dtd = {0x02,0x3A,0x80,0x18,0x71,0x38,0x2D,0x40,0x58,0x2C,0x45,0,0,0,0,0,0,0x1E};
	std::copy(dtd.begin(), dtd.end(), base.begin() + 54);
	std::uint32_t sum = 0U; for (std::size_t i = 0U; i < 127U; ++i) sum += base[i];
	base[127] = static_cast<std::uint8_t>(0U - sum);

	cru::core::Cta861ExtensionParser::Bytes cta = {};
	cta[0] = 0x02U; cta[1] = 0x03U; cta[2] = 7U; cta[4] = 0x42U; cta[5] = 16U; cta[6] = 31U;
	std::copy(dtd.begin(), dtd.end(), cta.begin() + 7);
	sum = 0U; for (std::size_t i = 0U; i < 127U; ++i) sum += cta[i];
	cta[127] = static_cast<std::uint8_t>(0U - sum);

	std::vector<std::uint8_t> bytes;
	bytes.insert(bytes.end(), base.begin(), base.end());
	bytes.insert(bytes.end(), cta.begin(), cta.end());
	const auto document = cru::core::EdidDocumentParser::parse(bytes);
	check_equal("EDID document", "accepted", true, document.has_value());
	if (document) {
		check_equal("EDID document", "CTA count", 1U, document->cta_extensions.size());
		check_equal("EDID document", "unknown extension count", 0U, document->unparsed_extensions.size());
		check_equal("EDID document", "combined DTD count", 2U, document->detailed_timings.size());
		check_equal("EDID document", "advertised mode count", 2U, document->advertised_video_modes.size());
		auto mutable_document = *document;
		const cru::core::DisplayCapabilitiesSnapshot snapshot(mutable_document);
		const cru::core::DisplayTimingReport report(snapshot);
		const cru::core::DisplayModeInventory inventory(snapshot);
		check_equal("capabilities snapshot", "manufacturer ID", 0x10ACU, snapshot.manufacturer_id());
		check_equal("capabilities snapshot", "CTA count", 1U, snapshot.cta_extension_count());
		check_equal("capabilities snapshot", "unknown extension count", 0U, snapshot.unparsed_extension_count());
		check_equal("capabilities snapshot", "DTD count", 2U, snapshot.detailed_timings().size());
		check_equal("capabilities snapshot", "advertised mode count", 2U, snapshot.advertised_video_modes().size());
		check_equal("display timing report", "timing count", 2U, report.timings().size());
		check_equal("display timing report", "consistent count", 2U, report.consistent_timing_count());
		check_equal("display timing report", "inconsistent count", 0U, report.inconsistent_timing_count());
		check_equal("display timing report", "all consistent", true, report.all_timings_consistent());
		check_equal("display mode inventory", "deduplicated modes", 2U, inventory.modes().size());
		check_equal("display mode inventory", "resolution count", 1U, inventory.resolutions().size());
		if (inventory.resolutions().size() == 1U) {
			const auto &resolution = inventory.resolutions()[0];
			check_equal("display mode inventory", "width", 1920U, resolution.horizontal_active);
			check_equal("display mode inventory", "height", 1080U, resolution.vertical_active);
			check_equal("display mode inventory", "minimum refresh", 50000U, resolution.minimum_refresh_rate_millihertz);
			check_equal("display mode inventory", "maximum refresh", 60000U, resolution.maximum_refresh_rate_millihertz);
			check_equal("display mode inventory", "mode count", 2U, resolution.advertised_mode_count);
			check_equal("display mode inventory", "has DTD", true, resolution.has_detailed_timing);
			check_equal("display mode inventory", "has CTA", true, resolution.has_cta_vic);
		}
		mutable_document.detailed_timings.clear();
		mutable_document.advertised_video_modes.clear();
		mutable_document.cta_extensions.clear();
		check_equal("capabilities snapshot independence", "CTA count", 1U, snapshot.cta_extension_count());
		check_equal("capabilities snapshot independence", "DTD count", 2U, snapshot.detailed_timings().size());
		check_equal("capabilities snapshot independence", "advertised mode count", 2U, snapshot.advertised_video_modes().size());

		const AxisTiming bad_horizontal = {1920U, 88U, 44U, 147U, 280U, 2200U, SyncPolarity::Positive};
		const AxisTiming bad_vertical = {1080U, 4U, 5U, 36U, 45U, 1124U, SyncPolarity::Positive};
		mutable_document = *document;
		mutable_document.advertised_video_modes.push_back({{128U, false}, std::nullopt});
		const cru::core::DisplayCapabilitiesSnapshot unresolved_snapshot(mutable_document);
		check_equal("display mode inventory", "unresolved CTA count", 1U,
			cru::core::DisplayModeInventory(unresolved_snapshot).unresolved_cta_mode_count());
		mutable_document.detailed_timings.push_back(TimingSnapshot(
			bad_horizontal, bad_vertical, 0U, 0U, 0U, ScanMode::Progressive,
			TimingType::Manual, ReducedBlanking::Unknown));
		const cru::core::DisplayCapabilitiesSnapshot mixed_snapshot(mutable_document);
		const cru::core::DisplayTimingReport mixed_report(mixed_snapshot);
		check_equal("mixed timing report", "consistent count", 2U, mixed_report.consistent_timing_count());
		check_equal("mixed timing report", "inconsistent count", 1U, mixed_report.inconsistent_timing_count());
		check_equal("mixed timing report", "all consistent", false, mixed_report.all_timings_consistent());
	}

	auto truncated = bytes; truncated.resize(128U);
	check_equal("EDID document truncated", "accepted", false,
		cru::core::EdidDocumentParser::parse(truncated).has_value());
	auto bad_checksum = bytes; bad_checksum[255] ^= 1U;
	check_equal("EDID document checksum", "accepted", false,
		cru::core::EdidDocumentParser::parse(bad_checksum).has_value());

	auto unknown = bytes;
	unknown[128] = 0x70U;
	unknown[255] = 0U; sum = 0U; for (std::size_t i = 128U; i < 255U; ++i) sum += unknown[i];
	unknown[255] = static_cast<std::uint8_t>(0U - sum);
	const auto unknown_document = cru::core::EdidDocumentParser::parse(unknown);
	check_equal("EDID unknown extension", "accepted", true, unknown_document.has_value());
	if (unknown_document) {
		check_equal("EDID unknown extension", "CTA count", 0U, unknown_document->cta_extensions.size());
		check_equal("EDID unknown extension", "preserved", 1U, unknown_document->unparsed_extensions.size());
	}
}

void check_cta_data_block_view()
{
	const cru::core::CtaDataBlock video = {2U, {16U, 31U}};
	const cru::core::CtaDataBlockView video_view(video);
	check_equal("CTA block view", "video type", cru::core::CtaDataBlockType::Video, video_view.type());
	check_equal("CTA block view", "payload size", 2U, video_view.payload().size());
	check_equal("CTA block view", "video extended tag", false, video_view.extended_tag().has_value());

	const cru::core::CtaDataBlock extended = {7U, {0x06U, 0x01U}};
	const cru::core::CtaDataBlockView extended_view(extended);
	check_equal("CTA block view", "extended type", cru::core::CtaDataBlockType::Extended, extended_view.type());
	check_equal("CTA block view", "extended tag", 0x06U, extended_view.extended_tag().value_or(0xFFU));

	const cru::core::CtaDataBlock empty_extended = {7U, {}};
	check_equal("CTA block view", "empty extended tag", false,
		cru::core::CtaDataBlockView(empty_extended).extended_tag().has_value());
	const cru::core::CtaDataBlock reserved = {6U, {}};
	check_equal("CTA block view", "reserved type", cru::core::CtaDataBlockType::Reserved,
		cru::core::CtaDataBlockView(reserved).type());
}

void check_cta_vic_catalog()
{
	const auto hd = cru::core::CtaVicCatalog::lookup(16U);
	check_equal("CTA VIC 16", "known", true, hd.has_value());
	if (hd) {
		check_equal("CTA VIC 16", "horizontal active", 1920U, hd->horizontal_active);
		check_equal("CTA VIC 16", "vertical active", 1080U, hd->vertical_active);
		check_equal("CTA VIC 16", "scan mode", ScanMode::Progressive, hd->scan_mode);
		check_equal("CTA VIC 16", "aspect", cru::core::CtaAspectRatio::SixteenByNine, hd->aspect_ratio);
		check_equal("CTA VIC 16", "nominal refresh", 60000U, hd->nominal_refresh_rate_millihertz);
		check_equal("CTA VIC 16", "legacy support", true, hd->supported_by_legacy_cru);
	}

	const auto interlaced = cru::core::CtaVicCatalog::lookup(5U);
	check_equal("CTA VIC 5", "known", true, interlaced.has_value());
	if (interlaced)
		check_equal("CTA VIC 5", "scan mode", ScanMode::Interlaced, interlaced->scan_mode);

	const auto uhd = cru::core::CtaVicCatalog::lookup(97U);
	check_equal("CTA VIC 97", "known", true, uhd.has_value());
	if (uhd) {
		check_equal("CTA VIC 97", "horizontal active", 3840U, uhd->horizontal_active);
		check_equal("CTA VIC 97", "vertical active", 2160U, uhd->vertical_active);
		check_equal("CTA VIC 97", "nominal refresh", 60000U, uhd->nominal_refresh_rate_millihertz);
	}

	const auto extended = cru::core::CtaVicCatalog::lookup(193U);
	check_equal("CTA VIC 193", "known", true, extended.has_value());
	if (extended) {
		check_equal("CTA VIC 193", "horizontal active", 5120U, extended->horizontal_active);
		check_equal("CTA VIC 193", "nominal refresh", 120000U, extended->nominal_refresh_rate_millihertz);
	}

	const auto legacy_unsupported = cru::core::CtaVicCatalog::lookup(2U);
	check_equal("CTA VIC 2", "known", true, legacy_unsupported.has_value());
	if (legacy_unsupported)
		check_equal("CTA VIC 2", "legacy support", false, legacy_unsupported->supported_by_legacy_cru);

	check_equal("CTA VIC 0", "known", false, cru::core::CtaVicCatalog::lookup(0U).has_value());
	check_equal("CTA VIC 128", "known", false, cru::core::CtaVicCatalog::lookup(128U).has_value());
	check_equal("CTA VIC 220", "known", false, cru::core::CtaVicCatalog::lookup(220U).has_value());
}

}

int main()
{
	const std::array<ExpectedTiming, 5> timings = {{
		{
			"1920x1080p60 CTA-861",
			{0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E},
			1920, 88, 44, 148, 280, 2200, SyncPolarity::Positive,
			1080, 4, 5, 36, 45, 1125, SyncPolarity::Positive,
			148500000, 60000, 67500, ScanMode::Progressive
		},
		{
			"1920x1080p144 CRU CVT-RB",
			{0x5F, 0x87, 0x80, 0xA0, 0x70, 0x38, 0x4D, 0x40, 0x30, 0x20, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A},
			1920, 48, 32, 80, 160, 2080, SyncPolarity::Positive,
			1080, 3, 5, 69, 77, 1157, SyncPolarity::Negative,
			346550000, 144000, 166610, ScanMode::Progressive
		},
		{
			"2560x1440p144 CRU CVT-RB",
			{0x15, 0xEC, 0x00, 0xA0, 0xA0, 0xA0, 0x67, 0x50, 0x30, 0x20, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A},
			2560, 48, 32, 80, 160, 2720, SyncPolarity::Positive,
			1440, 3, 5, 95, 103, 1543, SyncPolarity::Negative,
			604370000, 144000, 222194, ScanMode::Progressive
		},
		{
			"1920x1080p60 CRU CVT-RB",
			{0x2A, 0x36, 0x80, 0xA0, 0x70, 0x38, 0x1F, 0x40, 0x30, 0x20, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A},
			1920, 48, 32, 80, 160, 2080, SyncPolarity::Positive,
			1080, 3, 5, 23, 31, 1111, SyncPolarity::Negative,
			138660000, 60000, 66663, ScanMode::Progressive
		},
		{
			"1920x1080i60 CTA-861",
			{0x01, 0x1D, 0x80, 0x18, 0x71, 0x1C, 0x16, 0x20, 0x58, 0x2C, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9E},
			1920, 88, 44, 148, 280, 2200, SyncPolarity::Positive,
			540, 2, 5, 15, 22, 562, SyncPolarity::Positive,
			74250000, 60000, 33750, ScanMode::Interlaced
		}
	}};

	for (const auto &timing : timings)
		check_timing(timing);

	check_invalid_descriptors();
	check_timing_analyzer();
	check_base_edid();
	check_base_advertised_timings();
	check_gtf_timing_calculator();
	check_cta_extension();
	check_cta_data_block_view();
	check_cta_vic_catalog();
	check_cta_advertised_video_modes();
	check_edid_document();

	if (failures != 0)
		return 1;

	std::printf("Portable Core: %zu DTD fixtures, base EDID/CTA parsing, and invalid-input checks passed.\n", timings.size());
	return 0;
}
