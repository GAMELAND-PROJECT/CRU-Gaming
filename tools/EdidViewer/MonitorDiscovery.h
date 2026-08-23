#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cru { namespace viewer {

struct ConnectedMonitor
{
	std::wstring name;
	std::wstring instance_id;
	std::vector<std::uint8_t> edid;
};

class MonitorDiscovery
{
public:
	static std::vector<ConnectedMonitor> discover();
};

} }
