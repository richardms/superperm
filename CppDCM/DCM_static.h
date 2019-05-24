#ifndef _CPPDCM_DCM_STATIC_H
#define _CPPDCM_DCM_STATIC_H
#include <array>
#include <bitset>
#include <map>
#include <vector>

namespace DCM {

extern std::map<uint32_t, std::vector<std::array<uint32_t, 2>>> known_map;
}

#endif