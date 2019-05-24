#include "DCM_calc.h"

namespace DCM {
//	Generate all permutations of 1 ... n;
//	generates lists for lower values of n
//	in the process.

void makePerms(int n, perm_tab_t &perm_tab) {
  int fn = calc_fn(n);
  int size = n * fn;

  auto &res = perm_tab[n];
  res.reserve(n);
  if (n == 1) {
    res.push_back(1);
  } else {
    makePerms(n - 1, perm_tab);
    auto &prev = perm_tab[n - 1];
    int pf = calc_fn(n - 1);
    for (int k = 0; k < pf; k++) {
      for (int q = n - 1; q >= 0; q--) {
        int r = 0;
        for (int s = 0; s < n; s++) {
          if (s == q)
            res.push_back(n);
          else
            res.push_back(prev[k * (n - 1) + (r++)]);
        };
      };
    };
  };
}

} // namespace DCM