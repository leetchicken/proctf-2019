#pragma once
#include <vector>
#include "misc.h"

void InitChecksystem(const std::vector<IPAddr>& consolesIp);
bool Check(IPAddr ip);