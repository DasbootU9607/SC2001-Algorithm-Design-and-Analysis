# SC2001 Algorithm Design and Analysis

Project 1 investigates a hybrid sorting algorithm that switches from merge sort
to insertion sort when a subarray contains at most **S** elements.

## Implementations

| Version | Entry point | Included results |
|---|---|---|
| C++17 | [Code and instructions](cpp/README.md) | Full experiment: 685 measurement records, up to 10,000,000 integers |
| Python | [Jupyter Notebook](python/Project_1.ipynb) | Quick experiment: 285 measurement records, up to 10,000 integers; full mode is available |

Both versions study comparison counts for fixed S and fixed input size, select a
threshold using CPU time, and compare the hybrid with original merge sort.

```text
cpp/       C++ source, tests, experiment scripts, report, and recorded results
python/    Python notebook, dependencies, and usage instructions
```

## Getting started

**C++** (GCC and Make; Linux or WSL):

```sh
cd cpp
make
make test
make quick
```

**Python**:

```sh
cd python
python -m pip install -r requirements.txt
python -m jupyterlab Project_1.ipynb
```

See the [Python instructions](python/README.md) for full-mode experiments and the
[C++ report](cpp/docs/REPORT.md) for the recorded full-scale analysis.
The [assignment](cpp/docs/assignment.pdf) defines the project requirements.

Measurements describe each implementation's own environment and inputs; timings
and optimal thresholds should not be assumed to be identical.
