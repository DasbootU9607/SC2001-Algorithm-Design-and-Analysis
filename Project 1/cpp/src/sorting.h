#ifndef SORTING_H
#define SORTING_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sorting {

// Sort values in place and return the number of comparisons between keys.
// scratch must be a different vector with at least values.size() elements.
// The hybrid uses insertion sort for subarrays of length <= threshold (S >= 1).
std::uint64_t hybrid_sort(std::vector<int>& values,
                         std::vector<int>& scratch,
                         std::size_t threshold);

// Original merge sort: recursively split until each range has at most one key.
std::uint64_t merge_sort(std::vector<int>& values,
                        std::vector<int>& scratch);

}  // namespace sorting

#endif
