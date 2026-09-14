# Python implementation

[Project_1.ipynb](Project_1.ipynb) contains the sorting algorithms, correctness
checks, comparison counters, experiments, three figures, and discussion.

Read the [experimental report](REPORT.md) for the full methods, results, and
analysis. Its supporting CSVs, configuration, and figures are preserved in the
[recorded full run](recorded_results/full/).

## Open the notebook

From this directory:

```sh
python -m pip install -r requirements.txt
python -m jupyterlab Project_1.ipynb
```

The saved outputs use `FULL_RUN = True`: five trials per setting, 685
measurement records, and a maximum input size of 10,000,000. To reproduce the
study, restart the kernel and run the cells in order. Full mode may take
substantial time and memory with Python lists. Set `FULL_RUN = False` for a
smaller workflow check.

At 10,000,000 elements, threshold tuning selected `S = 32`. On fresh inputs,
the hybrid's median CPU time was 32.40625 seconds versus 36.125 seconds for
merge sort: a 10.29% reduction, with 10.09% more key comparisons. The hybrid
was faster in four of five paired trials. These are measurements from one
environment, not a universal threshold or performance guarantee.

Each new run writes CSVs, configuration, and figures to a new `results/quick/`
or `results/full/` subdirectory. Generated runs are ignored by Git; the report's
reference run is explicitly tracked under `recorded_results/full/`. The notebook's
saved tables and figures remain visible without rerunning it. Output directory
messages use relative paths, and environment metadata records only the Python
version and operating-system name.

The saved run was resumed after an interruption: the first 663 measurements
were preserved and the remaining 22 were completed in a second session with
the same Python version and operating system. All final paired comparisons
were collected in that second session. The notebook explains this limitation.

After rerunning with different settings or hardware, update the discussion to
describe the new measurements.
