#pragma once
#include "Constants.h"
#include <algorithm>
#include <array>
#include <cmath>
namespace reelglue {
struct TapeConfig {
    int speed = 2, head = 1, slot = 0, cal = 2, emphasis = 0;
    int formula = 2;
    double ips = 15, referenceFlux = 355;
    int humHz = 60;
};
inline TapeConfig resolve(int speed, int head, int slot, int cal, int emphasis) {
    TapeConfig c;
    c.speed=std::clamp(speed,0,3); c.head=c.speed<=1 ? 0:std::clamp(head,0,2);
    c.slot=std::clamp(slot,0,3); c.cal=std::clamp(cal,0,3);
    c.emphasis=c.speed==3 ? 2 : (c.speed==0 ? 0 : std::clamp(emphasis,0,1));
    constexpr int table[4][3][4] = {
        {{0,1,2,3},{0,1,2,3},{0,1,2,3}},
        {{0,1,2,3},{0,1,2,3},{0,1,2,3}},
        {{2,3,5,6},{2,3,5,6},{2,3,5,6}},
        {{4,2,5,3},{4,2,5,3},{4,2,5,3}}};
    c.formula=table[c.speed][c.head][c.slot]; c.ips=constants::speeds[c.speed];
    c.referenceFlux=constants::flux[c.cal]; c.humHz=c.emphasis==1 ? 50:60; return c;
}
inline std::array<double,5> idealFor(const TapeConfig& c) {
    std::array<double,5> result {};
    for (int i=0;i<5;++i) result[static_cast<size_t>(i)]=std::clamp(0.5+constants::formulaBias[c.formula]*(0.5+i*0.1)
        +0.025*(c.speed-2)*(i%2==0 ? 1:-1)+0.015*(c.head-1),0.2,0.8);
    return result;
}
}
