#pragma once
#include <stdint.h>
#include <algorithm>


struct Point2D
{
    int32_t x;
    int32_t y;
};


static const uint32_t kScreenSize = 32;
static const uint32_t kBitsForCoord = 5;


inline int32_t EdgeFunction(const Point2D& a, const Point2D& b, const Point2D& c) 
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}


inline uint32_t Rasterize(Point2D* v)
{
    int32_t minX = kScreenSize - 1;
    int32_t minY = kScreenSize - 1;
    int32_t maxX = 0;
    int32_t maxY = 0;

    for(uint32_t vi = 0; vi < 3; vi++)
    {
        if(v[vi].x > maxX) maxX = v[vi].x;
        if(v[vi].y > maxY) maxY = v[vi].y;
        if(v[vi].x < minX) minX = v[vi].x;
        if(v[vi].y < minY) minY = v[vi].y;
    }

    uint32_t result = 0;
    int doubleTriArea = EdgeFunction(v[0], v[1], v[2]);
    if(doubleTriArea > 0)
    {
        const int32_t kMaxShift = 31;
        Point2D p;
        for(p.y = minY; p.y <= maxY; p.y++)
        {
            for(p.x = minX; p.x <= maxX; p.x++)
            {
                int32_t w0 = EdgeFunction(v[1], v[2], p);
                int32_t w1 = EdgeFunction(v[2], v[0], p);
                int32_t w2 = EdgeFunction(v[0], v[1], p);

                if((w0 | w1 | w2) >= 0) 
                {
                    result += w0;
                    result += w1 << std::min(p.x, kMaxShift);
                    result += w2 << std::min(p.x + p.y, kMaxShift);
                }
            }
        }
    }

    return result;
}