// Part (a): hybrid merge/insertion sort and the original merge-sort baseline.
#include "sorting.h"

#include <stdexcept>

namespace sorting {
namespace {

// All internal ranges are half-open: [begin, end).
void validate_scratch(const std::vector<int>& values,
                      const std::vector<int>& scratch) {
    if (&values == &scratch) {
        throw std::invalid_argument("scratch storage must not alias the input");
    }
    if (scratch.size() < values.size()) {
        throw std::invalid_argument("scratch storage is smaller than the input");
    }
}

std::uint64_t insertion_sort_range(std::vector<int>& values,
                                  std::size_t begin,
                                  std::size_t end) {
    std::uint64_t comparisons = 0;
    for (std::size_t index = begin + 1; index < end; ++index) {
        const int key = values[index];
        std::size_t position = index;
        while (position > begin) {
            // Count this comparison even when it stops the insertion loop.
            ++comparisons;
            if (values[position - 1] <= key) {
                break;
            }
            values[position] = values[position - 1];
            --position;
        }
        values[position] = key;
    }
    return comparisons;
}

std::uint64_t merge_ranges(std::vector<int>& values,
                          std::vector<int>& scratch,
                          std::size_t begin,
                          std::size_t middle,
                          std::size_t end) {
    for (std::size_t index = begin; index < end; ++index) {
        scratch[index] = values[index];
    }

    std::size_t left = begin;
    std::size_t right = middle;
    std::size_t output = begin;
    std::uint64_t comparisons = 0;

    while (left < middle && right < end) {
        ++comparisons;
        // Taking the left value on a tie preserves the original equal-key order.
        if (scratch[left] <= scratch[right]) {
            values[output++] = scratch[left++];
        } else {
            values[output++] = scratch[right++];
        }
    }
    while (left < middle) {
        values[output++] = scratch[left++];
    }
    while (right < end) {
        values[output++] = scratch[right++];
    }
    return comparisons;
}

std::uint64_t merge_sort_range(std::vector<int>& values,
                              std::vector<int>& scratch,
                              std::size_t begin,
                              std::size_t end) {
    const std::size_t length = end - begin;
    if (length <= 1) {
        return 0;
    }

    // Give the left half the extra element so exact counts are reproducible.
    const std::size_t middle = begin + length / 2 + length % 2;
    std::uint64_t comparisons = merge_sort_range(values, scratch, begin, middle);
    comparisons += merge_sort_range(values, scratch, middle, end);
    comparisons += merge_ranges(values, scratch, begin, middle, end);
    return comparisons;
}

std::uint64_t hybrid_sort_range(std::vector<int>& values,
                               std::vector<int>& scratch,
                               std::size_t begin,
                               std::size_t end,
                               std::size_t threshold) {
    const std::size_t length = end - begin;
    if (length <= threshold) {
        return insertion_sort_range(values, begin, end);
    }

    const std::size_t middle = begin + length / 2 + length % 2;
    std::uint64_t comparisons =
        hybrid_sort_range(values, scratch, begin, middle, threshold);
    comparisons += hybrid_sort_range(values, scratch, middle, end, threshold);
    comparisons += merge_ranges(values, scratch, begin, middle, end);
    return comparisons;
}

}  // namespace

std::uint64_t hybrid_sort(std::vector<int>& values,
                         std::vector<int>& scratch,
                         std::size_t threshold) {
    if (threshold == 0) {
        throw std::invalid_argument("threshold must be positive");
    }
    validate_scratch(values, scratch);
    return hybrid_sort_range(values, scratch, 0, values.size(), threshold);
}

std::uint64_t merge_sort(std::vector<int>& values,
                        std::vector<int>& scratch) {
    validate_scratch(values, scratch);
    return merge_sort_range(values, scratch, 0, values.size());
}

}  // namespace sorting
