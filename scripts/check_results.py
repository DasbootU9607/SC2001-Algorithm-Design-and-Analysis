"""Check benchmark CSVs using only Python's standard library.

Usage (from the repository root): python scripts/check_results.py results [--full]
"""

import csv
from collections import defaultdict
from pathlib import Path
from statistics import mean, median
import sys


def read_csv(path):
    """Keep CSV values as strings until the relevant validation converts them."""
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


def check_results(folder, full=False):
    """Recompute summaries and verify that the experiment used paired inputs."""
    # Index every individual measurement by its experiment setting.
    raw = read_csv(folder / "raw_measurements.csv")
    groups = defaultdict(list)
    for row in raw:
        key = (row["experiment"], row["algorithm"], row["n"], row["threshold"])
        groups[key].append(row)
        assert int(row["key_comparisons"]) >= 0
        assert float(row["cpu_seconds"]) > 0
        assert float(row["wall_seconds"]) > 0

    # Recompute saved means, medians, and ranges from the raw measurements.
    summary_files = ["input_sizes", "threshold_sweep", "threshold_tuning", "merge_comparison"]
    for name in summary_files:
        summary = read_csv(folder / f"{name}.csv")
        assert summary, f"Empty summary: {name}"
        for row in summary:
            key = (row["experiment"], row["algorithm"], row["n"], row["threshold"])
            runs = groups.pop(key)
            assert len(runs) == int(row["trials"])
            assert len({run["trial"] for run in runs}) == len(runs)
            comparisons = mean(int(run["key_comparisons"]) for run in runs)
            cpu = median(float(run["cpu_seconds"]) for run in runs)
            assert abs(comparisons - float(row["mean_key_comparisons"])) < 0.000001
            assert abs(cpu - float(row["median_cpu_seconds"])) < 0.00000001
            times = [float(run["cpu_seconds"]) for run in runs]
            assert abs(min(times) - float(row["min_cpu_seconds"])) < 0.00000001
            assert abs(max(times) - float(row["max_cpu_seconds"])) < 0.00000001
            wall = median(float(run["wall_seconds"]) for run in runs)
            assert abs(wall - float(row["median_wall_seconds"])) < 0.00000001
    assert not groups, "Raw measurements missing from summary tables"

    # Each parameter choice must receive exactly the same seed within a trial.
    seeds = defaultdict(set)
    for row in raw:
        seeds[(row["experiment"], row["n"], row["trial"])].add(row["seed"])
    assert all(len(values) == 1 for values in seeds.values())

    # Check threshold selection and the use of fresh inputs for part (d).
    tuning = read_csv(folder / "threshold_tuning.csv")
    best_rows = read_csv(folder / "best_threshold.csv")
    for best in best_rows:
        candidates = [row for row in tuning if row["n"] == best["n"]]
        winner = min(candidates, key=lambda row: (float(row["median_cpu_seconds"]), int(row["threshold"])))
        assert best["best_threshold"] == winner["threshold"]

    comparison = read_csv(folder / "merge_comparison.csv")
    assert {row["algorithm"] for row in comparison} == {"merge", "hybrid"}
    largest = max(best_rows, key=lambda row: int(row["n"]))
    assert next(row for row in comparison if row["algorithm"] == "hybrid")["threshold"] == largest["best_threshold"]
    tuning_seeds = {row["seed"] for row in raw if row["experiment"] == "threshold_tuning"}
    final_seeds = {row["seed"] for row in raw if row["experiment"] == "merge_comparison"}
    assert tuning_seeds.isdisjoint(final_seeds)

    if full:
        # Full mode must cover every size and threshold required by our study.
        sizes = {int(row["n"]) for row in read_csv(folder / "input_sizes.csv")}
        assert sizes == {1_000, 3_000, 10_000, 30_000, 100_000, 300_000, 1_000_000, 3_000_000, 10_000_000}
        assert all(row["threshold"] == "16" for row in read_csv(folder / "input_sizes.csv"))
        assert all(int(row["n"]) == 10_000_000 for row in comparison)
        sweep = read_csv(folder / "threshold_sweep.csv")
        assert all(int(row["n"]) == 100_000 for row in sweep)
        assert {int(row["threshold"]) for row in sweep} == set(range(1, 65)) | {96, 128}
        expected_tuning = {(n, s) for n in [1_000, 10_000, 100_000, 1_000_000, 10_000_000]
                           for s in [1, 2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128]}
        assert {(int(row["n"]), int(row["threshold"])) for row in tuning} == expected_tuning
        assert all(int(row["trials"]) == 5 for name in summary_files
                   for row in read_csv(folder / f"{name}.csv"))
        assert len(raw) == 685
    print(f"PASS: {len(raw)} runs; summaries, shared inputs, threshold selection and final comparison verified.")


if __name__ == "__main__":
    check_results(Path(sys.argv[1] if len(sys.argv) > 1 else "results"), "--full" in sys.argv)
