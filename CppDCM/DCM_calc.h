#ifndef _CPPDCM_DCM_CALC_H
#define _CPPDCM_DCM_CALC_H
#include <array>
#include <bitset>
#include <map>
#include <vector>

namespace DCM {

using perm_tab_t = std::vector<std::vector<uint8_t>>;

constexpr uint32_t calc_fn(uint32_t n) {
  return (n == 1) ? 1 : n * calc_fn(n - 1);
}

void makePerms(int n, perm_tab_t &perm_tab);

template <size_t N>
void rClassMin(std::array<uint32_t, N> &p) {
  size_t min = p[0], km = 0;
  for (size_t k = 1; k < N; k++)
    if (p[k] < min) {
      min = p[k];
      km = k;
    };

  for (size_t r = 0; r < km; r++) {
    size_t tmp = p[0];
    for (size_t i = 1; i < N; i++) {
      p[i - 1] = p[i];
    };
    p[N - 1] = tmp;
  };
}

} // namespace DCM

#endif