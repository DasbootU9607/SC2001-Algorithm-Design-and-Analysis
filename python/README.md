# Python implementation

[Project_1.ipynb](Project_1.ipynb) contains the sorting algorithms, correctness
checks, comparison counters, experiments, three figures, and discussion.

## Open the notebook

From this directory:

```sh
python -m pip install -r requirements.txt
python -m jupyterlab Project_1.ipynb
```

The saved outputs use `FULL_RUN = False`: three trials per setting, 285
measurement records, and a maximum input size of 10,000. To run the full study,
set `FULL_RUN = True`, restart the kernel, and run the cells in order. Full mode
uses five trials and includes 10,000,000-element inputs. It may take substantial
time and memory with Python lists.

Each run writes CSVs, configuration, and figures to a new `results/quick/` or
`results/full/` subdirectory. Generated runs are ignored by Git; the notebook's
saved tables and figures remain visible without rerunning it. Output directory
messages use relative paths, and environment metadata records only the Python
version and operating-system name.

After a full run, update the discussion to describe those measurements before
treating the notebook as a full-scale experimental report.
