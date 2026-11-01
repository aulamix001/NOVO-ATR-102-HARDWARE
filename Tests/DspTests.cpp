#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Dsp/TapeChannel.h"
#include <random>
using namespace reelglue;
TEST_CASE("Every configuration resolves without changing the stored selection") {
 for(int s=0;s<4;++s)for(int h=0;h<3;++h)for(int e=0;e<2;++e)for(int t=0;t<4;++t){
  const auto c=resolve(s,h,t,2,e);
  CHECK(c.head==(s<=1?0:h));CHECK(c.emphasis==(s==3?2:s==0?0:e));
  CHECK(c.formula>=0);CHECK(c.formula<7);CHECK(c.referenceFlux==355);
  for(auto v:idealFor(c)){CHECK(v>=0);CHECK(v<=1);}
 }
}
TEST_CASE("Power off and thru preserve samples with declared latency") {
 for(double fs:{44100.,48000.,96000.,192000.})for(bool thru:{false,true}){
  TapeChannel c;c.prepare(fs,0);Settings s;s.power=thru;s.thru=thru;s.wow=false;s.noise=false;c.configure(s);
  for(int i=0;i<static_cast<int>(fs);++i)c.process(0);
  std::vector<float> input(2048);for(size_t i=0;i<input.size();++i)input[i]=0.3f*std::sin(static_cast<float>(i)*0.13f);
  for(size_t i=0;i<input.size();++i){auto y=c.process(input[i]);auto expected=i>=static_cast<size_t>(c.latency())?input[i-static_cast<size_t>(c.latency())]:0.f;CHECK(y==Catch::Approx(expected).margin(1e-7));}
 }
}
TEST_CASE("Calibration increases distortion without changing zero or low-level polarity") {
 TapeChannel low,high;low.prepare(48000,0);high.prepare(48000,0);Settings a,b;a.wow=b.wow=false;a.transformer=b.transformer=false;
 a.config=resolve(2,1,0,1,0);b.config=resolve(2,1,0,3,0);low.configure(a);high.configure(b);
 double energyLow=0,energyHigh=0;
 for(int i=0;i<48000;++i){float x=0.8f*std::sin(static_cast<float>(2*constants::pi*1000*i/48000));auto l=low.process(x),h=high.process(x);if(i>24000){energyLow+=l*l;energyHigh+=h*h;}}
 CHECK(energyHigh<energyLow);CHECK(energyHigh>0);
}
TEST_CASE("All sample rates remain finite under randomized configuration and hot inputs") {
 std::mt19937 rng(42);std::uniform_real_distribution<float> sample(-2,2);
 for(double fs:{44100.,48000.,96000.,192000.}){TapeChannel c;c.prepare(fs,0);Settings s;
  for(int i=0;i<200000;++i){if(i%512==0){s.config=resolve(static_cast<int>(rng()%4),static_cast<int>(rng()%3),static_cast<int>(rng()%4),static_cast<int>(rng()%4),static_cast<int>(rng()%2));s.record=static_cast<double>(rng()%40)-20;s.reproduce=-s.record;s.noise=rng()%2;s.wow=rng()%2;c.configure(s);}
   auto y=c.process(sample(rng));REQUIRE(std::isfinite(y));REQUIRE(std::abs(y)<100.f);}
 }
}
TEST_CASE("Independent channels are reproducible after prepare") {
 TapeChannel a,b;a.prepare(48000,0);b.prepare(48000,0);Settings s;s.noise=true;a.configure(s);b.configure(s);
 for(int i=0;i<10000;++i){auto x=0.1f*std::sin(i*0.01f);REQUIRE(a.process(x)==b.process(x));}
}
