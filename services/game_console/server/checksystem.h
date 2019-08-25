#pragma once
#include <vector>
#include "misc.h"

void InitChecksystem(const std::vector<IPAddr>& teamsNet);
IPAddr GetHwConsoleIp(NetworkAddr teamNet);
bool Check(NetworkAddr teamNet);