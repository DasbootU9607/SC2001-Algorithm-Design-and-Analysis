// SC2001 Project 1: reproducible experiments for parts (b), (c), and (d).
// The actual sorting algorithms are in sorting.cpp.
#include "experiments.h"
#include "sorting.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
// Shared input settings. Each experiment adds its own seed offset.
constexpr int MAX_VALUE = 10'000'000;
constexpr std::uint32_t BASE_SEED = 20'260'913;

// One timed sort on one seeded input.
struct Run {
    std::string algorithm;
    std::size_t n;
    std::size_t threshold;
    int trial;
    std::uint32_t seed;
    std::uint64_t comparisons;
    double cpu_seconds;
    double wall_seconds;
};

// Five full-mode trials (or three quick trials) summarized for a single setting.
struct Summary {
    std::string algorithm;
    std::size_t n;
    std::size_t threshold;
    std::size_t trials;
    double mean_comparisons;
    double median_cpu;
    double min_cpu;
    double max_cpu;
    double median_wall;
};

std::ofstream open_output(const std::filesystem::path& path) {
    std::ofstream file;
    file.exceptions(std::ios::failbit | std::ios::badbit);
    file.open(path);
    file << std::fixed << std::setprecision(9);
    return file;
}

double process_cpu_seconds() {
#ifdef _WIN32
    // On Windows std::clock() measures elapsed time, so use process CPU times.
    FILETIME created, exited, kernel, user;
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) {
        throw std::runtime_error("Cannot read process CPU time");
    }
    ULARGE_INTEGER kernel_ticks, user_ticks;
    kernel_ticks.LowPart = kernel.dwLowDateTime;
    kernel_ticks.HighPart = kernel.dwHighDateTime;
    user_ticks.LowPart = user.dwLowDateTime;
    user_ticks.HighPart = user.dwHighDateTime;
    return (kernel_ticks.QuadPart + user_ticks.QuadPart) / 10'000'000.0;
#else
    const std::clock_t ticks = std::clock();
    if (ticks == static_cast<std::clock_t>(-1)) {
        throw std::runtime_error("Cannot read process CPU time");
    }
    return static_cast<double>(ticks) / CLOCKS_PER_SEC;
#endif
}

// Part (b): generate n random integers in the inclusive range [1, MAX_VALUE].
std::vector<int> generate_input(std::size_t n, std::uint32_t seed) {
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(1, MAX_VALUE);
    std::vector<int> values(n);
    for (int& value : values) {
        value = distribution(generator);
    }
    return values;
}

// Copy and allocate before timing; validate the complete output after timing.
Run measure(const std::vector<int>& input, const std::vector<int>& expected,
            std::size_t threshold, bool original, int trial, std::uint32_t seed) {
#ifdef _WIN32
    // Windows process CPU accounting is coarse for small, individual sorts.
    // Pre-copy a batch, time only sorting, then report seconds per sort.
    if (input.size() < 1'000'000) {
        const std::size_t batch_size = (16'000'000 + input.size() - 1) / input.size();
        std::vector<std::vector<int>> batch(batch_size, input);
        std::vector<int> batch_scratch(input.size());
        const auto wall_start = std::chrono::steady_clock::now();
        const double cpu_start = process_cpu_seconds();
        std::uint64_t comparisons = 0;
        for (auto& values : batch) {
            comparisons = original ? sorting::merge_sort(values, batch_scratch)
                : sorting::hybrid_sort(values, batch_scratch, threshold);
        }
        const double cpu_elapsed = process_cpu_seconds() - cpu_start;
        const double wall_elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - wall_start).count();
        for (const auto& values : batch) {
            if (values != expected) {
                throw std::runtime_error("Sorting result differs from std::sort");
            }
        }
        return {original ? "merge" : "hybrid", input.size(), threshold, trial, seed,
                comparisons, cpu_elapsed / batch_size, wall_elapsed / batch_size};
    }
#endif
    std::vector<int> values = input;
    std::vector<int> scratch(input.size());

    const auto wall_start = std::chrono::steady_clock::now();
    const double cpu_start = process_cpu_seconds();
    const std::uint64_t comparisons = original
        ? sorting::merge_sort(values, scratch)
        : sorting::hybrid_sort(values, scratch, threshold);
    const double cpu_elapsed = process_cpu_seconds() - cpu_start;
    const double wall_elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - wall_start).count();

    // std::sort is only the correctness oracle, never the measured algorithm.
    if (values != expected) {
        throw std::runtime_error("Sorting result differs from std::sort");
    }
    return {original ? "merge" : "hybrid", input.size(), threshold, trial, seed,
            comparisons, cpu_elapsed, wall_elapsed};
}

double median(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    const std::size_t middle = values.size() / 2;
    return values.size() % 2 == 1 ? values[middle]
        : (values[middle - 1] + values[middle]) / 2.0;
}

// Count comparisons with a mean, and reduce timing noise with a median.
Summary summarize(const std::vector<Run>& runs, std::size_t n,
                  std::size_t threshold, const std::string& algorithm) {
    std::vector<double> cpu_times, wall_times;
    double comparison_sum = 0;
    for (const Run& run : runs) {
        if (run.n == n && run.threshold == threshold && run.algorithm == algorithm) {
            cpu_times.push_back(run.cpu_seconds);
            wall_times.push_back(run.wall_seconds);
            comparison_sum += static_cast<double>(run.comparisons);
        }
    }
    if (cpu_times.empty()) {
        throw std::runtime_error("No measurements for summary");
    }
    return {algorithm, n, threshold, cpu_times.size(), comparison_sum / cpu_times.size(),
            median(cpu_times), *std::min_element(cpu_times.begin(), cpu_times.end()),
            *std::max_element(cpu_times.begin(), cpu_times.end()), median(wall_times)};
}

void write_run(std::ofstream& file, const std::string& experiment, const Run& run) {
    file << experiment << ',' << run.algorithm << ',' << run.n << ',' << run.threshold
         << ',' << run.trial << ',' << run.seed << ',' << run.comparisons << ','
         << run.cpu_seconds << ',' << run.wall_seconds << '\n';
}

void write_summaries(const std::filesystem::path& folder, const std::string& experiment,
                     const std::vector<Summary>& summaries) {
    auto file = open_output(folder / (experiment + ".csv"));
    file << "experiment,algorithm,n,threshold,trials,mean_key_comparisons,"
            "median_cpu_seconds,min_cpu_seconds,max_cpu_seconds,median_wall_seconds\n";
    for (const Summary& row : summaries) {
        file << experiment << ',' << row.algorithm << ',' << row.n << ',' << row.threshold
             << ',' << row.trials << ',' << row.mean_comparisons << ',' << row.median_cpu
             << ',' << row.min_cpu << ',' << row.max_cpu << ',' << row.median_wall << '\n';
    }
}

// Shared loop for (c)(i)-(iii): vary n and S, keeping inputs paired by seed.
std::vector<Summary> run_hybrid_experiment(
        const std::string& experiment, const std::vector<std::size_t>& sizes,
        const std::vector<std::size_t>& thresholds, int repetitions,
        std::uint32_t seed_offset, const std::filesystem::path& folder,
        std::ofstream& raw_file) {
    std::vector<Summary> summaries;
    for (std::size_t n : sizes) {
        std::vector<Run> runs;
        for (int trial = 1; trial <= repetitions; ++trial) {
            const auto seed = BASE_SEED + seed_offset
                + static_cast<std::uint32_t>(n * 10) + trial;
            const auto input = generate_input(n, seed);
            auto expected = input;
            std::sort(expected.begin(), expected.end());

            // Vary execution order while preserving identical input for each S.
            auto order = thresholds;
            std::mt19937 order_generator(seed);
            std::shuffle(order.begin(), order.end(), order_generator);
            for (std::size_t threshold : order) {
                const auto run = measure(input, expected, threshold, false, trial, seed);
                write_run(raw_file, experiment, run);
                runs.push_back(run);
            }
        }
        for (std::size_t threshold : thresholds) {
            summaries.push_back(summarize(runs, n, threshold, "hybrid"));
        }
        std::cout << experiment << ": n=" << n << " completed" << std::endl;
    }
    write_summaries(folder, experiment, summaries);
    return summaries;
}

std::size_t choose_threshold(const std::vector<Summary>& tuning,
                            const std::vector<std::size_t>& sizes,
                            const std::filesystem::path& folder) {
    auto file = open_output(folder / "best_threshold.csv");
    file << "n,best_threshold,median_cpu_seconds,mean_key_comparisons\n";
    std::size_t selected = 1;
    for (std::size_t n : sizes) {
        const Summary* best = nullptr;
        for (const Summary& candidate : tuning) {
            if (candidate.n != n) {
                continue;
            }
            // Choose the lower CPU median; break exact ties with the smaller S.
            if (best == nullptr || candidate.median_cpu < best->median_cpu
                || (candidate.median_cpu == best->median_cpu
                    && candidate.threshold < best->threshold)) {
                best = &candidate;
            }
        }
        if (best == nullptr) {
            throw std::runtime_error("Missing tuning size");
        }
        file << n << ',' << best->threshold << ',' << best->median_cpu << ','
             << best->mean_comparisons << '\n';
        selected = best->threshold;  // Sizes are ascending: last is the largest.
    }
    return selected;
}

void compare_with_merge(std::size_t n, std::size_t threshold, int repetitions,
                        const std::filesystem::path& folder, std::ofstream& raw_file) {
    std::vector<Run> runs;
    for (int trial = 1; trial <= repetitions; ++trial) {
        const std::uint32_t seed = BASE_SEED + 400'000'000
            + static_cast<std::uint32_t>(n * 10) + trial;
        const auto input = generate_input(n, seed);
        auto expected = input;
        std::sort(expected.begin(), expected.end());
        for (int position = 0; position < 2; ++position) {
            const bool original = (trial + position) % 2 == 0;
            const auto run = measure(input, expected, original ? 1 : threshold,
                                     original, trial, seed);
            write_run(raw_file, "merge_comparison", run);
            runs.push_back(run);
        }
        std::cout << "merge_comparison: trial " << trial << " completed" << std::endl;
    }
    write_summaries(folder, "merge_comparison",
                    {summarize(runs, n, 1, "merge"), summarize(runs, n, threshold, "hybrid")});
}

void write_metadata(const std::filesystem::path& folder, bool quick, int repetitions,
                    std::size_t threshold, std::size_t comparison_size) {
    auto file = open_output(folder / "metadata.txt");
    file << "mode=" << (quick ? "quick" : "full") << '\n'
         << "compiler=" << __VERSION__ << '\n'
         << "cplusplus=" << __cplusplus << '\n'
         << "integer_bytes=" << sizeof(int) << '\n'
         << "max_value=" << MAX_VALUE << '\n'
         << "base_seed=" << BASE_SEED << '\n'
         << "repetitions=" << repetitions << '\n'
         << "comparison_size=" << comparison_size << '\n'
         << "selected_threshold=" << threshold << '\n'
         << "counting_included_in_timing=true\n"
         << "timed_region=sorting only; input copies and scratch allocation excluded\n"
         << "validation=every output compared with std::sort outside timing\n"
         << "cpu_summary=median of independent trials\n"
         << "comparison_summary=arithmetic mean of independent trials\n"
         << "random_engine=std::mt19937 with std::uniform_int_distribution<int>\n"
         << "seed_formula=base_seed + experiment_offset + 10*n + trial\n"
         << "experiment_offsets=input_sizes:100000000; threshold_sweep:200000000; "
            "threshold_tuning:300000000; merge_comparison:400000000\n";
#ifdef _WIN32
    file << "platform=Windows\ncpu_clock=GetProcessTimes (kernel + user)\n";
    file << "small_input_timing=n<1000000: pre-copied batches totaling at least 16000000 keys; time per sort\n";
#else
    file << "platform=POSIX\ncpu_clock=std::clock\nclocks_per_second=" << CLOCKS_PER_SEC << '\n';
#endif
}
}  // namespace

namespace experiments {

void run(bool quick, const std::filesystem::path& folder) {
    std::filesystem::create_directories(folder);

    // Full mode matches the assignment; quick mode only checks the workflow.
    const int repetitions = quick ? 3 : 5;
    const std::vector<std::size_t> sizes = quick
        ? std::vector<std::size_t>{1'000, 3'000, 10'000}
        : std::vector<std::size_t>{1'000, 3'000, 10'000, 30'000, 100'000,
                                  300'000, 1'000'000, 3'000'000, 10'000'000};
    const std::vector<std::size_t> tuning_sizes = quick
        ? std::vector<std::size_t>{1'000, 10'000}
        : std::vector<std::size_t>{1'000, 10'000, 100'000, 1'000'000, 10'000'000};
    const std::vector<std::size_t> tuning_thresholds = {
        1, 2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128
    };
    std::vector<std::size_t> sweep(64);
    std::iota(sweep.begin(), sweep.end(), 1);
    sweep.push_back(96);
    sweep.push_back(128);

    // A small untimed warm-up before collecting any measurements.
    auto warmup = generate_input(10'000, BASE_SEED);
    std::vector<int> scratch(warmup.size());
    sorting::hybrid_sort(warmup, scratch, 16);
    warmup = generate_input(10'000, BASE_SEED);
    sorting::merge_sort(warmup, scratch);

    auto raw_file = open_output(folder / "raw_measurements.csv");
    raw_file << "experiment,algorithm,n,threshold,trial,seed,key_comparisons,"
                "cpu_seconds,wall_seconds\n";

    // (c)(i): keep S = 16 and increase the input size.
    run_hybrid_experiment("input_sizes", sizes, {16}, repetitions,
                          100'000'000, folder, raw_file);

    // (c)(ii): keep n fixed and vary S.
    run_hybrid_experiment("threshold_sweep", {quick ? 10'000u : 100'000u}, sweep,
                          repetitions, 200'000'000, folder, raw_file);

    // (c)(iii): select S using CPU time on the largest tuning size.
    const auto tuning = run_hybrid_experiment("threshold_tuning", tuning_sizes,
        tuning_thresholds, repetitions, 300'000'000, folder, raw_file);
    const std::size_t threshold = choose_threshold(tuning, tuning_sizes, folder);
    std::cout << "Selected S=" << threshold << " from the largest tuning size." << std::endl;

    // (d): compare both algorithms on new, identical inputs in each trial.
    compare_with_merge(sizes.back(), threshold, repetitions, folder, raw_file);
    write_metadata(folder, quick, repetitions, threshold, sizes.back());
    std::cout << "Results saved to " << folder.string() << std::endl;
}

}  // namespace experiments
