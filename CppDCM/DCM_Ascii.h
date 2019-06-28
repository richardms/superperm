#ifndef _CPPDCM_DCM_ASCII_H
#define _CPPDCM_DCM_ASCII_H
#include <utility>

namespace DCM {

static std::string &&fromAscii(std::string const &ascii) {
  std::string natural;
  natural.reserve(ascii.size());
  std::transform(ascii.cbegin(), ascii.cend(), natural.begin(), [](char c) {
    return c + '0';
  });
  return std::move(natural);
}

static std::string &&toAscii(std::string const &natural) {
  std::string ascii;
  ascii.reserve(natural.size());
  std::transform(natural.cbegin(), natural.cend(), ascii.begin(), [](char c) {
    return c - '0';
  });
  return std::move(ascii);
}

} // namespace DCM

#endif