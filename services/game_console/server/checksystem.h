#pragma once
#include <vector>
#include "misc.h"

void InitChecksystem(const std::vector<IPAddr>& consolesIp);
IPAddr GetHwConsoleIp(NetworkAddr teamNet);
bool Check(NetworkAddr teamNet);