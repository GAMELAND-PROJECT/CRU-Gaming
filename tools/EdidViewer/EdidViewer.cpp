#include "DisplayCapabilitiesSnapshot.h"
#include "DisplayTimingReport.h"
#include "DisplayModeInventory.h"
#include "EdidDocument.h"
#include "MonitorDiscovery.h"

#include <windows.h>
#include <commdlg.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace
{
constexpr int open_button_id = 1001;
constexpr int monitor_combo_id = 1002;
constexpr int refresh_button_id = 1003;
HWND output_control = nullptr;
HWND open_button = nullptr;
HWND monitor_combo = nullptr;
HWND refresh_button = nullptr;
std::vector<cru::viewer::ConnectedMonitor> connected_monitors;

std::wstring manufacturer_name(std::uint16_t id)
{
	std::wstring name(3, L'?');
	name[0] = static_cast<wchar_t>(L'A' + ((id >> 10U) & 31U) - 1U);
	name[1] = static_cast<wchar_t>(L'A' + ((id >> 5U) & 31U) - 1U);
	name[2] = static_cast<wchar_t>(L'A' + (id & 31U) - 1U);
	return name;
}

void append_refresh_rate(std::wostream &text, std::uint64_t millihertz)
{
	text << millihertz / 1000U << L'.' << std::setw(3) << std::setfill(L'0') << millihertz % 1000U;
}

std::optional<std::vector<std::uint8_t>> read_file(const std::wstring &path)
{
	std::ifstream file(std::filesystem::path(path), std::ios::binary | std::ios::ate);
	if (!file) return std::nullopt;
	const auto size = file.tellg();
	if (size < 0) return std::nullopt;
	std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
	file.seekg(0);
	file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	if (!file) return std::nullopt;
	return bytes;
}

std::wstring format_report(const cru::core::DisplayCapabilitiesSnapshot &capabilities)
{
	const cru::core::DisplayTimingReport report(capabilities);
	const cru::core::DisplayModeInventory inventory(capabilities);
	std::wostringstream text;
	text << L"Manufacturer: " << manufacturer_name(capabilities.manufacturer_id()) << L"\r\n"
		<< L"Product code: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill(L'0')
		<< capabilities.product_code() << std::dec << L"\r\n"
		<< L"EDID: " << static_cast<unsigned>(capabilities.edid_version()) << L'.'
		<< static_cast<unsigned>(capabilities.edid_revision()) << L"\r\n"
		<< L"CTA extensions: " << capabilities.cta_extension_count() << L"\r\n"
		<< L"Detailed timings: " << report.timings().size() << L" ("
		<< report.consistent_timing_count() << L" consistent, "
		<< report.inconsistent_timing_count() << L" inconsistent)\r\n\r\n";

	text << L"Advertised resolution ranges:\r\n";
	for (const auto &resolution : inventory.resolutions())
	{
		text << resolution.horizontal_active << L'x' << resolution.vertical_active
			<< (resolution.scan_mode == cru::core::ScanMode::Interlaced ? L'i' : L'p') << L": ";
		append_refresh_rate(text, resolution.minimum_refresh_rate_millihertz);
		text << L" - ";
		append_refresh_rate(text, resolution.maximum_refresh_rate_millihertz);
		text << L" Hz (" << resolution.advertised_mode_count << L" mode"
			<< (resolution.advertised_mode_count == 1U ? L")" : L"s)") << L"\r\n";
	}
	if (inventory.unresolved_cta_mode_count() != 0U)
		text << L"Unresolved CTA modes: " << inventory.unresolved_cta_mode_count() << L"\r\n";
	text << L"\r\nDetailed timings:\r\n";

	for (std::size_t index = 0; index < report.timings().size(); ++index)
	{
		const auto &entry = report.timings()[index];
		const auto &timing = entry.timing;
		text << index + 1U << L". " << timing.horizontal().active << L'x' << timing.vertical().active
			<< (timing.interlaced() ? L'i' : L'p') << L" @ "
			<< timing.refresh_rate_millihertz() / 1000U << L'.' << std::setw(3) << std::setfill(L'0')
			<< timing.refresh_rate_millihertz() % 1000U << L" Hz, "
			<< timing.pixel_clock_hz() << L" Hz, "
			<< (entry.analysis.internally_consistent() ? L"OK" : L"FAILED") << L"\r\n";
	}

	text << L"\r\nCTA advertised video modes: " << capabilities.advertised_video_modes().size() << L"\r\n";
	for (const auto &mode : capabilities.advertised_video_modes())
	{
		text << L"VIC " << static_cast<unsigned>(mode.descriptor.video_identification_code);
		if (mode.descriptor.native) text << L" (native)";
		if (mode.catalog_info)
			text << L": " << mode.catalog_info->horizontal_active << L'x' << mode.catalog_info->vertical_active
				<< (mode.catalog_info->scan_mode == cru::core::ScanMode::Interlaced ? L'i' : L'p')
				<< L" @ " << mode.catalog_info->nominal_refresh_rate_millihertz / 1000U << L" Hz";
		text << L"\r\n";
	}
	return text.str();
}

void show_edid(HWND owner, const std::vector<std::uint8_t> &bytes)
{
	const auto document = cru::core::EdidDocumentParser::parse(bytes);
	if (!document)
	{
		MessageBoxW(owner, L"The selected source does not contain a valid complete EDID.", L"CRU Gaming EDID Viewer", MB_OK | MB_ICONERROR);
		return;
	}
	const cru::core::DisplayCapabilitiesSnapshot capabilities(*document);
	SetWindowTextW(output_control, format_report(capabilities).c_str());
}

void open_edid(HWND owner)
{
	wchar_t path[MAX_PATH] = {};
	OPENFILENAMEW dialog = {};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner = owner;
	dialog.lpstrFilter = L"EDID binary files\0*.bin;*.dat\0All files\0*.*\0";
	dialog.lpstrFile = path;
	dialog.nMaxFile = MAX_PATH;
	dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if (!GetOpenFileNameW(&dialog)) return;

	const auto bytes = read_file(path);
	if (!bytes)
	{
		MessageBoxW(owner, L"The selected file could not be read.", L"CRU Gaming EDID Viewer", MB_OK | MB_ICONERROR);
		return;
	}
	show_edid(owner, *bytes);
}

void load_selected_monitor(HWND owner)
{
	const LRESULT selection = SendMessageW(monitor_combo, CB_GETCURSEL, 0U, 0U);
	if (selection == CB_ERR || static_cast<std::size_t>(selection) >= connected_monitors.size()) return;
	show_edid(owner, connected_monitors[static_cast<std::size_t>(selection)].edid);
}

void refresh_monitors(HWND owner)
{
	connected_monitors = cru::viewer::MonitorDiscovery::discover();
	SendMessageW(monitor_combo, CB_RESETCONTENT, 0U, 0U);
	for (const auto &monitor : connected_monitors)
	{
		const auto &label = monitor.name.empty() ? monitor.instance_id : monitor.name;
		SendMessageW(monitor_combo, CB_ADDSTRING, 0U, reinterpret_cast<LPARAM>(label.c_str()));
	}
	if (connected_monitors.empty())
	{
		SetWindowTextW(output_control, L"No connected monitor with a readable EDID was found.\r\nYou can still use Open EDID... to inspect a file.");
		return;
	}
	SendMessageW(monitor_combo, CB_SETCURSEL, 0U, 0U);
	load_selected_monitor(owner);
}

void resize_controls(HWND window)
{
	RECT area = {};
	GetClientRect(window, &area);
	MoveWindow(open_button, 12, 12, 120, 30, TRUE);
	MoveWindow(monitor_combo, 144, 12, area.right - 280, 240, TRUE);
	MoveWindow(refresh_button, area.right - 124, 12, 112, 30, TRUE);
	MoveWindow(output_control, 12, 54, area.right - 24, area.bottom - 66, TRUE);
}

LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	switch (message)
	{
	case WM_COMMAND:
		if (LOWORD(w_param) == open_button_id) open_edid(window);
		else if (LOWORD(w_param) == refresh_button_id) refresh_monitors(window);
		else if (LOWORD(w_param) == monitor_combo_id && HIWORD(w_param) == CBN_SELCHANGE) load_selected_monitor(window);
		return 0;
	case WM_SIZE:
		resize_controls(window);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProcW(window, message, w_param, l_param);
	}
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command)
{
	const wchar_t class_name[] = L"CruGamingEdidViewer";
	WNDCLASSW window_class = {};
	window_class.lpfnWndProc = window_procedure;
	window_class.hInstance = instance;
	window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	window_class.lpszClassName = class_name;
	if (!RegisterClassW(&window_class)) return 1;

	HWND window = CreateWindowExW(0, class_name, L"CRU Gaming - EDID Viewer",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 820, 620,
		nullptr, nullptr, instance, nullptr);
	if (!window) return 1;

	open_button = CreateWindowExW(0, L"BUTTON", L"Open EDID...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(open_button_id)), instance, nullptr);
	monitor_combo = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
		0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(monitor_combo_id)), instance, nullptr);
	refresh_button = CreateWindowExW(0, L"BUTTON", L"Refresh monitors", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(refresh_button_id)), instance, nullptr);
	output_control = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Select a binary EDID file to inspect.",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
		0, 0, 0, 0, window, nullptr, instance, nullptr);
	if (!open_button || !monitor_combo || !refresh_button || !output_control) return 1;

	const auto font = reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT));
	SendMessageW(open_button, WM_SETFONT, font, TRUE);
	SendMessageW(monitor_combo, WM_SETFONT, font, TRUE);
	SendMessageW(refresh_button, WM_SETFONT, font, TRUE);
	SendMessageW(output_control, WM_SETFONT, font, TRUE);
	resize_controls(window);
	refresh_monitors(window);
	ShowWindow(window, show_command);
	UpdateWindow(window);

	MSG message = {};
	while (GetMessageW(&message, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	return static_cast<int>(message.wParam);
}
