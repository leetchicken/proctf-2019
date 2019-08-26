#include "log.h"
#include <mutex>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "misc.h"

static const uint32_t kPageSize = 256;

struct Line
{
    std::string text;
    uint32_t logSize;
};

static std::mutex GMutex;
static std::vector<Line> GLog;


static Line& NewLine()
{
    GLog.resize(GLog.size() + 1);
    return GLog.back();
}


void Log(const char* formatStr, ...)
{
    std::lock_guard<std::mutex> guard(GMutex);

    char buf[1024];

    auto& newLine = NewLine();
    sprintf(buf, "[%.2f] ", GetTime());
    newLine.text.append(buf);

    va_list args;
    va_start(args, formatStr);
    vsnprintf(buf, sizeof(buf), formatStr, args);
    va_end(args);

    newLine.text.append(buf);

    uint32_t prevLogSize = 0;
    if(GLog.size() > 1)
        prevLogSize = GLog[GLog.size() - 2].logSize;
    newLine.logSize = newLine.text.length() + prevLogSize;

    printf("%s", newLine.text.c_str());
}


char* GetPage(uint32_t pageIdx, uint32_t& pageSize)
{
    std::mutex GMutex;
    uint32_t pagesNum = (GLog.size() + kPageSize - 1) / kPageSize;
    if(pageIdx == ~0u)
        pageIdx = pagesNum - 1;
    if(pageIdx >= pagesNum)
    {
        pageSize = 0;
        return nullptr;
    }

    uint32_t firstLineIdx = pageIdx * kPageSize;
    uint32_t lastLineIdx =  firstLineIdx + kPageSize - 1;
    if(lastLineIdx >= GLog.size())
        lastLineIdx = GLog.size() - 1;

    char buf[128];
    sprintf(buf, "Page %u of %u\n", pageIdx + 1, pagesNum);
    pageSize = strlen(buf);

    pageSize += GLog[lastLineIdx].logSize - (firstLineIdx == 0 ? 0 : GLog[firstLineIdx - 1].logSize);

    char* page = (char*)malloc(pageSize);
    char* pagePtr = page;
    strcpy(pagePtr, buf);
    pagePtr += strlen(buf);
    for(uint32_t i = firstLineIdx; i <= lastLineIdx; i++)
    {
        strncpy(pagePtr, GLog[i].text.c_str(), GLog[i].text.length());
        pagePtr += GLog[i].text.length();
    }

    return page;
}


char* GetFullLog(uint32_t& size)
{
    std::mutex GMutex;
    if(GLog.size() == 0)
    {
        size = 0;
        return nullptr;
    }

    size = GLog.back().logSize;

    char* log = (char*)malloc(size);
    char* logPtr = log;
    for(uint32_t i = 0; i < GLog.size(); i++)
    {
        strncpy(logPtr, GLog[i].text.c_str(), GLog[i].text.length());
        logPtr += GLog[i].text.length();
    }

    return log;
}