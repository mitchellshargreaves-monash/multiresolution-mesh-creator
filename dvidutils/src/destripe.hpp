#ifndef DESTRIPE_HPP
#define DESTRIPE_HPP

#include <vector>
#include <cstdint>
#include "destripe.hpp"

std::vector<uint8_t> destripe(uint8_t * image, size_t w, size_t h, size_t YC,
                              std::vector<int> const & seam, bool writeplot=false);

#endif // DESTRIPE_HPP
