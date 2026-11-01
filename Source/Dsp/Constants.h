#pragma once
#include <array>
namespace reelglue::constants {
inline constexpr double meterReferenceDb = -12.0;
inline constexpr double referenceAmplitude = 0.251188643150958;
inline constexpr double smoothingSeconds = 0.025;
inline constexpr double meterRiseSeconds = 0.065;
inline constexpr double meterFallSeconds = 0.18;
inline constexpr double wowBaseSeconds = 0.0005;
inline constexpr double wowDepthSeconds = 0.000038;
inline constexpr double wowHz = 0.5;
inline constexpr double flutterHz = 13.0;
inline constexpr double flutterDepthSeconds = 0.0000015;
inline constexpr double crosstalkGain = -0.00251188643151;
inline constexpr double crosstalkCutoff = 5000.0;
inline constexpr double railLimit = 10.0;
inline constexpr double headBumpQ = 0.85;
inline constexpr double headBumpDb = 1.6;
inline constexpr double headWavelengthMetres = 0.0069;
inline constexpr double saturationDrive = 0.62;
inline constexpr double noiseDbfs = -80.0;
inline constexpr double humDbfs = -90.0;
inline constexpr bool allowCcirAt75Ips = true;
inline constexpr bool meterRefIsRms = true;
inline constexpr std::array<double,4> speeds {3.75, 7.5, 15.0, 30.0};
inline constexpr std::array<double,4> flux {77.0, 251.0, 355.0, 502.0};
inline constexpr std::array<double,3> speedWowScale {3.0, 1.8, 1.0};
inline constexpr std::array<const char*,7> formulaNames {"111", "35-90", "456", "GP9", "250", "900", "499"};
inline constexpr std::array<int,7> recommendedCal {0,1,2,3,1,3,3};
inline constexpr std::array<double,7> formulaBias {-0.08,-0.05,0.0,0.07,-0.025,0.055,0.04};
inline constexpr std::array<double,3> emphasisLfSeconds {0.00318,0.0,0.0};
inline constexpr std::array<double,3> emphasisHfSeconds {0.00005,0.000035,0.0000175};
inline constexpr double calibrationHfDb = 12.0;
inline constexpr double calibrationLfDb = 6.0;
inline constexpr double calibrationShelfHz = 120.0;
inline constexpr double calibrationHfHz = 8000.0;
inline constexpr double biasHfDb = 4.0;
inline constexpr double pi = 3.14159265358979323846;
}
