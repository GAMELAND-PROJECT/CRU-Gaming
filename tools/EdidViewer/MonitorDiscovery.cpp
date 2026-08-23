#include "MonitorDiscovery.h"

#include <windows.h>
#include <devguid.h>
#include <setupapi.h>

#include <vector>

namespace cru { namespace viewer {
namespace {

std::wstring device_property(HDEVINFO devices, SP_DEVINFO_DATA &device, DWORD property)
{
	DWORD required = 0U;
	SetupDiGetDeviceRegistryPropertyW(devices, &device, property, nullptr, nullptr, 0U, &required);
	if (required == 0U) return {};
	std::vector<std::uint8_t> buffer(required);
	if (!SetupDiGetDeviceRegistryPropertyW(
		devices, &device, property, nullptr, buffer.data(), required, nullptr)) return {};
	return reinterpret_cast<const wchar_t *>(buffer.data());
}

std::wstring instance_id(HDEVINFO devices, SP_DEVINFO_DATA &device)
{
	DWORD required = 0U;
	SetupDiGetDeviceInstanceIdW(devices, &device, nullptr, 0U, &required);
	if (required == 0U) return {};
	std::vector<wchar_t> buffer(required);
	if (!SetupDiGetDeviceInstanceIdW(devices, &device, buffer.data(), required, nullptr)) return {};
	return buffer.data();
}

std::vector<std::uint8_t> read_edid(HDEVINFO devices, SP_DEVINFO_DATA &device)
{
	HKEY key = SetupDiOpenDevRegKey(devices, &device, DICS_FLAG_GLOBAL, 0U, DIREG_DEV, KEY_READ);
	if (key == INVALID_HANDLE_VALUE) return {};
	DWORD type = 0U;
	DWORD size = 0U;
	LONG status = RegQueryValueExW(key, L"EDID", nullptr, &type, nullptr, &size);
	std::vector<std::uint8_t> bytes;
	if (status == ERROR_SUCCESS && type == REG_BINARY && size >= 128U)
	{
		bytes.resize(size);
		status = RegQueryValueExW(key, L"EDID", nullptr, &type, bytes.data(), &size);
		if (status != ERROR_SUCCESS) bytes.clear();
		else bytes.resize(size);
	}
	RegCloseKey(key);
	return bytes;
}

}

std::vector<ConnectedMonitor> MonitorDiscovery::discover()
{
	std::vector<ConnectedMonitor> monitors;
	HDEVINFO devices = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, nullptr, nullptr, DIGCF_PRESENT);
	if (devices == INVALID_HANDLE_VALUE) return monitors;

	for (DWORD index = 0U; ; ++index)
	{
		SP_DEVINFO_DATA device = {};
		device.cbSize = sizeof(device);
		if (!SetupDiEnumDeviceInfo(devices, index, &device))
		{
			if (GetLastError() == ERROR_NO_MORE_ITEMS) break;
			continue;
		}

		auto name = device_property(devices, device, SPDRP_FRIENDLYNAME);
		if (name.empty()) name = device_property(devices, device, SPDRP_DEVICEDESC);
		auto id = instance_id(devices, device);
		auto edid = read_edid(devices, device);
		if (!edid.empty()) monitors.push_back({name, id, edid});
	}

	SetupDiDestroyDeviceInfoList(devices);
	return monitors;
}

} }
