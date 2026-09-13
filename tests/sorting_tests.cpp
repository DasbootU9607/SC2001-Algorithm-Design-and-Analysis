// Run with `make test`. The test suite has no external dependencies.
#include "sorting.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

template <typename Function>
void check_invalid_argument(Function function, const std::string& message) {
    try {
        function();
        check(false, message);
    } catch (const std::invalid_argument&) {
        // Expected.
    } catch (...) {
        check(false, message + " (wrong exception type)");
    }
}

void check_both_sorts(const std::vector<int>& input, std::size_t threshold,
                      const std::string& description) {
    std::vector<int> expected = input;
    std::sort(expected.begin(), expected.end());

    std::vector<int> hybrid_values = input;
    std::vector<int> hybrid_scratch(input.size());
    sorting::hybrid_sort(hybrid_values, hybrid_scratch, threshold);
    check(hybrid_values == expected, "hybrid sort: " + description);

    std::vector<int> merge_values = input;
    std::vector<int> merge_scratch(input.size());
    sorting::merge_sort(merge_values, merge_scratch);
    check(merge_values == expected, "merge sort: " + description);
}

void test_exact_counts() {
    {
        std::vector<int> values{3, 2, 1};
        std::vector<int> scratch(values.size());
        check(sorting::hybrid_sort(values, scratch, 3) == 3,
              "insertion sort counts all three comparisons for reversed length three");
    }
    {
        std::vector<int> values{3, 2, 1};
        std::vector<int> scratch(values.size());
        check(sorting::merge_sort(values, scratch) == 2,
              "merge sort uses a larger left half and counts two comparisons");
    }
    {
        std::vector<int> values{1, 2, 3};
        std::vector<int> scratch(values.size());
        check(sorting::hybrid_sort(values, scratch, 3) == 2,
              "insertion sort counts failed exit comparisons for ordered input");
    }
    {
        std::vector<int> values{1, 2, 3};
        std::vector<int> scratch(values.size());
        check(sorting::merge_sort(values, scratch) == 3,
              "merge sort ordered length-three count follows the documented split");
    }
}

void test_edge_cases_and_oracle() {
    check_both_sorts({}, 1, "empty input");
    check_both_sorts({42}, 1, "singleton input");
    check_both_sorts({4, 1, 4, 2, 4, 1}, 3, "duplicate values");
    check_both_sorts({1, 2, 3, 4, 5, 6}, 2, "ordered input");
    check_both_sorts({6, 5, 4, 3, 2, 1}, 2, "reversed input");

    std::mt19937 generator(2001);
    std::uniform_int_distribution<int> distribution(-20, 20);
    for (std::size_t length = 0; length <= 128; ++length) {
        std::vector<int> values(length);
        for (int& value : values) {
            value = distribution(generator);
        }
        check_both_sorts(values, 7, "random input of length " + std::to_string(length));
    }
}

void test_threshold_behavior() {
    const std::vector<int> input{8, 3, 7, 4, 9, 2, 6, 5, 1};

    std::vector<int> merge_values = input;
    std::vector<int> merge_scratch(input.size());
    const std::uint64_t merge_count = sorting::merge_sort(merge_values, merge_scratch);

    std::vector<int> hybrid_values = input;
    std::vector<int> hybrid_scratch(input.size());
    const std::uint64_t hybrid_count = sorting::hybrid_sort(hybrid_values, hybrid_scratch, 1);
    check(hybrid_values == merge_values, "threshold one produces the merge-sort ordering");
    check(hybrid_count == merge_count, "threshold one produces the merge-sort count");

    std::vector<int> insertion_values{3, 2, 1};
    std::vector<int> insertion_scratch(insertion_values.size());
    check(sorting::hybrid_sort(insertion_values, insertion_scratch, 100) == 3,
          "threshold larger than n sorts the whole input by insertion sort");
}

void test_invalid_arguments() {
    std::vector<int> values{2, 1};
    std::vector<int> scratch(values.size());
    check_invalid_argument(
        [&] { sorting::hybrid_sort(values, scratch, 0); },
        "hybrid sort rejects a zero threshold");

    std::vector<int> too_small(1);
    check_invalid_argument(
        [&] { sorting::hybrid_sort(values, too_small, 1); },
        "hybrid sort rejects undersized scratch storage");
    check_invalid_argument(
        [&] { sorting::merge_sort(values, too_small); },
        "merge sort rejects undersized scratch storage");

    check_invalid_argument(
        [&] { sorting::hybrid_sort(values, values, 1); },
        "hybrid sort rejects aliased scratch storage");
    check_invalid_argument(
        [&] { sorting::merge_sort(values, values); },
        "merge sort rejects aliased scratch storage");
}

}  // namespace

int main() {
    test_exact_counts();
    test_edge_cases_and_oracle();
    test_threshold_behavior();
    test_invalid_arguments();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "All sorting tests passed\n";
    return 0;
}
