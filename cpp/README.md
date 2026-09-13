# SC2001 Project 1: Hybrid Merge Sort

A readable C++17 implementation of merge sort combined with insertion sort,
with reproducible experiments for every part of the assignment.

**This README shows actual measurements from the included 685-run experiment.**
The largest inputs contain **10,000,000 integers**. See the
[full report](docs/REPORT.md) for the algorithm and complexity analysis.

[A. Algorithm](#a-algorithm-implementation) | [B. Input data](#b-generate-input-data) |
[C. Complexity and experiments](#c-analyze-time-complexity) |
[D. Merge-sort comparison](#d-compare-with-original-merge-sort) |
[Run the project](#run-the-project)

The answers below follow the original assignment order: **A, B, C(i)-(iii), D**.

## A. Algorithm implementation

The hybrid algorithm uses **merge sort for large subarrays** and switches to
**insertion sort when the subarray length is at most S**. Its implementation is
in [`src/sorting.cpp`](src/sorting.cpp).

```text
hybrid_sort(array, begin, end, S):
    if end - begin <= S:
        insertion_sort(array[begin:end])
        return
    middle = begin + ceil((end - begin) / 2)
    hybrid_sort(array, begin, middle, S)
    hybrid_sort(array, middle, end, S)
    merge the two sorted halves
```

The code uses half-open ranges **[begin, end)** and one reusable scratch array.
During merging, the left key is selected on equality, preserving stability.
The original merge sort uses the same merge step but recurses to single-element
subarrays. Setting **S = 1** gives the hybrid the same comparison count as the
original merge sort.

A **key comparison** compares two input values. The counter includes the final
insertion comparison that stops the inner loop, but excludes index checks,
assignments, and copying the remaining keys after one merge side is exhausted.
Both sorting functions return a 64-bit comparison count. Exact-count examples
and correctness checks are in [`tests/sorting_tests.cpp`](tests/sorting_tests.cpp).

## B. Generate input data

The tested array sizes are **1,000; 3,000; 10,000; 30,000; 100,000; 300,000;
1,000,000; 3,000,000; and 10,000,000**. For each size, values are drawn uniformly
from **[1, x]**, with **x = 10,000,000**. Duplicate values are allowed.

`generate_input` in [`src/experiments.cpp`](src/experiments.cpp) uses this logic:

```cpp
std::mt19937 generator(seed);
std::uniform_int_distribution<int> distribution(1, 10'000'000);
std::vector<int> values(n);
for (int& value : values) {
    value = distribution(generator);
}
```

Arrays are generated in memory as needed. The seed for every measured run is
stored in [raw_measurements.csv](results/raw_measurements.csv), so inputs can be
recreated without storing large array files. Each setting uses five seeded
inputs, and all algorithms or thresholds within a trial receive the same input.

### Experiment setup

Recorded on **13 September 2026** using an **Intel Core i5-13420H**, Ubuntu 24.04
under WSL2, and **GCC 13.3.0 with `-O2`**. Each setting used five seeded inputs.
Comparison counts below are arithmetic means; CPU times are medians.

- Random integers are uniform in **[1, 10,000,000]**, including duplicates.
- Each trial has a recorded seed. Every candidate receives the same input within
  a trial. The final comparison uses seeds separate from tuning.
- Timing includes sorting and comparison counting. Input generation, copying,
  scratch allocation, validation, and file writing are outside timing.
- CPU time and elapsed wall time are recorded separately. Every sorted output
  is checked against `std::sort` outside the timed region.
- These results describe one machine and five trials per setting. Timing ranges
  show observed variation, not confidence intervals.

The [raw CSV](results/raw_measurements.csv), [run metadata](results/metadata.txt),
and [environment record](results/environment.txt) are included. Seeded input is
reproducible with the same C++ standard-library implementation.

## C. Analyze time complexity

For a subarray larger than S, recursive sorting is followed by a linear merge.
Insertion-sort leaves take quadratic time on random distinct keys on average
and in the worst case. For **1 <= S <= n**, this gives
**Theta(nS + n log2(n/S))**; with fixed S, the hybrid is **Theta(n log n)**.
The [report](docs/REPORT.md#c-analyze-time-complexity) derives the recurrence and
the distinct-key expectation curves used below.

The following three experiments answer C(i), C(ii), and C(iii), respectively.
All tables and figures are based on the included measured data.

### C(i). Fixed S = 16, increasing input size

| Input size n | Mean key comparisons | Median CPU time |
|---:|---:|---:|
| 1,000 | 10,233.4 | 0.000035 s |
| 3,000 | 33,711.2 | 0.000139 s |
| 10,000 | 127,157.6 | 0.000586 s |
| 30,000 | 451,993.0 | 0.001863 s |
| 100,000 | 1,640,033.0 | 0.009383 s |
| 300,000 | 5,257,949.2 | 0.029176 s |
| 1,000,000 | 20,221,519.6 | 0.082376 s |
| 3,000,000 | 63,548,450.6 | 0.273207 s |
| 10,000,000 | 226,420,296.4 | 1.035817 s |

The comparison count grows approximately as **n log2 n**, as expected for a
fixed threshold. Different experiments use different seeds and run at different
times; compare tuning candidates within the tuning experiment below.

![Key comparisons against input size, with S fixed at 16](results/figures/c1_comparisons_vs_n.png)

Source: [input-size experiment](results/input_sizes.csv).

### C(ii). Fixed n = 100,000, varying S

The experiment tested **S = 1 through 64, plus 96 and 128**. Selected rows:

| Threshold S | Mean key comparisons |
|---:|---:|
| 1 | 1,536,460.0 |
| 8 | 1,557,748.0 |
| 16 | 1,639,135.8 |
| 32 | 1,862,168.2 |
| 64 | 2,384,519.0 |
| 128 | 3,511,717.8 |

Large thresholds increase insertion-sort work. Plateaus occur when several
thresholds produce the same recursive partitions. The dashed curve is an
expected-count model for distinct random keys; the dots are measured results.

![Key comparisons against the insertion-sort threshold](results/figures/c2_comparisons_vs_s.png)

Source: [all 66 threshold settings](results/threshold_sweep.csv).

### C(iii). Selecting the threshold by CPU time

Candidates: **1, 2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128**.

| Input size n | Fastest tested S | Median CPU time |
|---:|---:|---:|
| 1,000 | 48 | 0.000023 s |
| 10,000 | 64 | 0.000449 s |
| 100,000 | 48 | 0.006200 s |
| 1,000,000 | 64 | 0.077435 s |
| 10,000,000 | 24 | 1.091716 s |

**S = 24 is the fastest tested candidate in this run**, selected from the
largest tuning size. It is not a universal optimum. At 10 million elements,
S = 24 and S = 32 produce identical leaf sizes, so their measured time difference
is noise. S = 48 was also close at 1.094072 s, with overlapping timing ranges.

![Normalized CPU time for each threshold and input size](results/figures/c3_threshold_cpu.png)

Sources: [tuning measurements](results/threshold_tuning.csv) and
[selected thresholds](results/best_threshold.csv).

## D. Compare with original Merge Sort

Both algorithms were compared on arrays of **10,000,000 integers**.
The tuning experiment selected **S = 24**. The final comparison used five new
inputs, with the same input given to both algorithms within each trial.

| Algorithm | Mean key comparisons | Median CPU time | Observed CPU range |
|---|---:|---:|---:|
| Original merge sort | 220,101,381.6 | 1.292713 s | 1.223015-1.423818 s |
| Hybrid merge sort, S = 24 | 242,314,340.8 | 1.147524 s | 1.077629-1.243052 s |

The hybrid took **11.23% less CPU time** while making **10.09% more key
comparisons**. It was faster in four of the five paired trials. Saving recursive
calls and small merges can outweigh the extra comparisons made by insertion sort.

![Measured comparison of original merge sort and hybrid sort](results/figures/d_merge_comparison.png)

Sources: [comparison summary](results/merge_comparison.csv) and
[all raw measurements](results/raw_measurements.csv).

## Run the project

Run commands from the `cpp/` directory (`cd cpp` from the repository root).
The verified environment is
**Linux / Ubuntu WSL with GCC and Make**. Python is only needed to check CSVs
and generate plots.

### Build and test

```sh
make
make test
make quick
make check-results
```

`make test` checks sorting and exact comparison counts. `make quick` runs 285
small experiments and writes to `results/quick/`. `make check-results` validates
the included full results. Check a quick run with:

```sh
python3 scripts/check_results.py results/quick
```

From **Windows PowerShell**, use the same commands through WSL:

```powershell
wsl -d Ubuntu --exec make
wsl -d Ubuntu --exec make test
wsl -d Ubuntu --exec make quick
wsl -d Ubuntu --exec make check-results
wsl -d Ubuntu --exec python3 scripts/check_results.py results/quick
```

### Run the full experiment

```sh
make run
python3 scripts/check_results.py results/local --full
```

This runs 685 measured sorts, including the 10-million-element cases, and saves
new results in **`results/local/`**. Expect a few minutes and approximately
160 MB of array storage on the verified WSL build.

The executable also accepts an output directory directly:

```sh
./build/project1 --help
./build/project1 --quick --output results/quick
./build/project1 --output results/local
```

Running the executable without arguments writes to `results/`. Existing files
in the chosen directory are replaced. Use `results/local/` for new full runs to
keep the measurements shown above. README and report tables are a snapshot of
the included run; rerunning experiments does not rewrite them.

### Generate figures

Python 3.10+ and Matplotlib are required. On Linux / WSL:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
.venv/bin/python scripts/plot_results.py --results results/local
```

Or use a **Windows Python environment** from PowerShell:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe scripts/plot_results.py --results results/local
```

Figures are written to `<results-directory>/figures/`. To regenerate the included
figures, pass `--results results` instead. Create the plotting environment on
one platform; Windows and Linux virtual environments are not interchangeable.

### Compile without Make

```sh
mkdir -p build
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc src/main.cpp src/experiments.cpp src/sorting.cpp -o build/project1
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc tests/sorting_tests.cpp src/sorting.cpp -o build/tests
./build/tests
```

The code also contains native Windows CPU accounting for MinGW, including
batched timing for small inputs. That branch has not been compiled in this
environment; the results and commands above were verified with WSL/GCC.

## C++ directory structure

```text
.
|-- README.md                 # Results, figures, and run instructions
|-- Makefile                  # Build, test, quick, full-run, and data-check targets
|-- requirements.txt          # Matplotlib dependency for plotting
|-- src/
|   |-- main.cpp              # Command-line arguments and error handling
|   |-- experiments.h         # Experiment-runner interface
|   |-- experiments.cpp       # Input generation, timing, summaries, experiments
|   |-- sorting.h             # Sorting interfaces
|   `-- sorting.cpp           # Hybrid sort, original merge sort, shared merge step
|-- tests/
|   `-- sorting_tests.cpp      # Exact counts, edge cases, random correctness tests
|-- scripts/
|   |-- check_results.py      # Check CSVs, seeds, and experiment coverage
|   `-- plot_results.py       # Render figures from measured CSV data
|-- docs/
|   |-- assignment.pdf        # Original assignment
|   `-- REPORT.md             # Full report covering parts (a)-(d)
`-- results/
    |-- *.csv                 # Published raw data and summaries
    |-- metadata.txt          # Experiment settings and seed formula
    |-- environment.txt       # Environment of the recorded run
    `-- figures/              # Four figures embedded in this README
```

Local builds, virtual environments, quick runs, and new full runs are ignored
by Git. The CSVs and figures directly under `results/` are the recorded results.

## Read the code

Start with [sorting.h](src/sorting.h), then [sorting.cpp](src/sorting.cpp).
The algorithm functions use clear half-open ranges **[begin, end)**:

1. `insertion_sort_range` inserts each key into a sorted prefix.
2. `merge_ranges` merges two sorted halves using reusable scratch storage.
3. `hybrid_sort_range` switches to insertion sort when the range length is at
   most S. `merge_sort_range` always recurses to single-element ranges.

Both public sorts modify the input and return a 64-bit key-comparison count.
A terminating insertion comparison counts; index checks and copying do not.
Taking the left key on a tie preserves stability.

```cpp
#include "sorting.h"

std::vector<int> values{5, 2, 4, 1, 3};
std::vector<int> scratch(values.size());
const auto comparisons = sorting::hybrid_sort(values, scratch, 16);
// values is now {1, 2, 3, 4, 5}; comparisons stores the key-comparison count.
```

For the experiment flow, read `experiments::run` at the bottom of
[experiments.cpp](src/experiments.cpp). Its steps are labeled **(c)(i)**,
**(c)(ii)**, **(c)(iii)**, and **(d)**. Part **(b)** is handled by `generate_input`.
[main.cpp](src/main.cpp) only parses arguments and starts this sequence.

See the [report](docs/REPORT.md) for the recurrence and the
`Theta(nS + n log(n/S))` analysis; with fixed S, the hybrid remains
**Theta(n log n)**.
