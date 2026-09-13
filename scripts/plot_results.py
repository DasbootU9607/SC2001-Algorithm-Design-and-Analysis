"""Create the four report figures from measured CSV data.

Usage (from the repository root): python scripts/plot_results.py --results results
"""

import argparse
import csv
from functools import lru_cache
from math import log2
from pathlib import Path

import matplotlib

matplotlib.use("Agg")  # Save figures without requiring a desktop window.
import matplotlib.pyplot as plt


def read_rows(folder, filename):
    with (folder / filename).open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


@lru_cache(maxsize=None)
def expected_comparisons(n, threshold):
    """Expected count for uniformly shuffled DISTINCT keys; ties are ignored.

    An insertion leaf of length k uses k(k-1)/4 + k - H_k comparisons on
    average. A merge of lengths a,b uses a+b-a/(b+1)-b/(a+1) on average.
    Use the same ceil/floor split as the C++ implementation.
    """
    if n <= threshold:
        harmonic_number = sum(1 / k for k in range(1, n + 1))
        return n * (n - 1) / 4 + n - harmonic_number
    left = (n + 1) // 2
    right = n // 2
    return (expected_comparisons(left, threshold)
            + expected_comparisons(right, threshold)
            + n - left / (right + 1) - right / (left + 1))


def save_figure(figure, folder, filename):
    figure.savefig(folder / filename, dpi=180, bbox_inches="tight")
    plt.close(figure)
    print(f"Saved {folder / filename}")


def plot_input_sizes(results, figures):
    rows = read_rows(results, "input_sizes.csv")
    sizes = [int(row["n"]) for row in rows]
    counts = [float(row["mean_key_comparisons"]) for row in rows]
    threshold = int(rows[0]["threshold"])
    model = [expected_comparisons(n, threshold) for n in sizes]

    figure, axes = plt.subplots(1, 2, figsize=(10.5, 4.2))
    axes[0].loglog(sizes, counts, "o-", label="Measured mean (5 trials)" if rows[0]["trials"] == "5" else "Measured mean")
    axes[0].loglog(sizes, model, "--", label="Distinct-key expectation")
    axes[0].set(xlabel="Input size n", ylabel="Key comparisons",
                title=f"(c)(i) Comparisons vs n, S = {threshold}")
    axes[0].legend(fontsize=8)

    ratios = [count / (n * log2(n)) for n, count in zip(sizes, counts)]
    axes[1].semilogx(sizes, ratios, "o-", color="#ca641d")
    axes[1].set(xlabel="Input size n", ylabel="Comparisons / (n log2 n)",
                title="Normalized comparison growth", ylim=(0, max(ratios) * 1.2))
    for axis in axes:
        axis.grid(True, alpha=0.22)
    figure.tight_layout()
    save_figure(figure, figures, "c1_comparisons_vs_n.png")


def plot_threshold_sweep(results, figures):
    rows = read_rows(results, "threshold_sweep.csv")
    n = int(rows[0]["n"])
    thresholds = [int(row["threshold"]) for row in rows]
    counts = [float(row["mean_key_comparisons"]) / 1_000_000 for row in rows]
    model_thresholds = list(range(1, max(thresholds) + 1))
    model = [expected_comparisons(n, s) / 1_000_000 for s in model_thresholds]
    figure, axis = plt.subplots(figsize=(8.5, 4.4))
    axis.plot(thresholds, counts, "o", markersize=4, label="Measured mean")
    axis.plot(model_thresholds, model, "--", label="Distinct-key expectation")
    axis.set(xlabel="Insertion-sort threshold S", ylabel="Key comparisons (millions)",
             title=f"(c)(ii) Comparisons vs S, n = {n:,}")
    axis.grid(True, alpha=0.22)
    axis.legend()
    figure.tight_layout()
    save_figure(figure, figures, "c2_comparisons_vs_s.png")


def plot_threshold_tuning(results, figures):
    rows = read_rows(results, "threshold_tuning.csv")
    sizes = sorted({int(row["n"]) for row in rows})
    figure, axis = plt.subplots(figsize=(8.5, 4.8))
    for n in sizes:
        group = [row for row in rows if int(row["n"]) == n]
        thresholds = [int(row["threshold"]) for row in group]
        baseline = float(next(row for row in group if row["threshold"] == "1")["median_cpu_seconds"])
        if baseline <= 0:
            raise ValueError("CPU clock resolution is insufficient for the tuning plot")
        medians = [float(row["median_cpu_seconds"]) / baseline for row in group]
        minimums = [float(row["min_cpu_seconds"]) / baseline for row in group]
        maximums = [float(row["max_cpu_seconds"]) / baseline for row in group]
        line, = axis.plot(thresholds, medians, "o-", markersize=4, label=f"n = {n:,}")
        axis.fill_between(thresholds, minimums, maximums, color=line.get_color(), alpha=0.08)
    axis.axhline(1, color="#666666", linestyle="--", linewidth=1)
    axis.set(xlabel="Insertion-sort threshold S", ylabel="CPU time / median CPU time at S = 1",
             title="(c)(iii) Threshold tuning (lower is faster)")
    axis.grid(True, alpha=0.22)
    axis.legend(fontsize=8, ncol=2)
    figure.text(0.5, 0.01, "Lines: medians. Shading: observed minimum-to-maximum range; not confidence intervals.",
                ha="center", fontsize=8)
    figure.tight_layout(rect=(0, 0.035, 1, 1))
    save_figure(figure, figures, "c3_threshold_cpu.png")


def plot_merge_comparison(results, figures):
    rows = read_rows(results, "merge_comparison.csv")
    raw = [row for row in read_rows(results, "raw_measurements.csv")
           if row["experiment"] == "merge_comparison"]
    labels = ["Original merge" if row["algorithm"] == "merge"
              else f"Hybrid (S = {row['threshold']})" for row in rows]
    counts = [float(row["mean_key_comparisons"]) / 1_000_000 for row in rows]
    cpu = [float(row["median_cpu_seconds"]) for row in rows]
    colors = ["#577590", "#e69f45"]
    figure, axes = plt.subplots(1, 2, figsize=(10, 4.4))
    axes[0].bar(labels, counts, color=colors, width=0.55)
    axes[0].set(ylabel="Mean key comparisons (millions)", title="Key comparisons")
    axes[1].bar(labels, cpu, color=colors, width=0.55)
    for index, row in enumerate(rows):
        observations = [float(run["cpu_seconds"]) for run in raw if run["algorithm"] == row["algorithm"]]
        offsets = [(j - (len(observations) - 1) / 2) * 0.04 for j in range(len(observations))]
        axes[1].scatter([index + offset for offset in offsets], observations,
                        color="#333333", s=15, zorder=3)
    axes[1].set(ylabel="Process CPU time (seconds)", title="Median CPU time; dots are individual trials")
    for axis, values in zip(axes, (counts, cpu)):
        axis.bar_label(axis.containers[0], labels=[f"{value:.3f}" for value in values],
                       label_type="center", color="white", fontweight="bold")
        axis.set_ylim(0, max(axis.get_ylim()[1], max(values) * 1.22))
        axis.grid(axis="y", alpha=0.2)
        axis.set_axisbelow(True)
    figure.suptitle(f"(d) Original merge sort vs hybrid, n = {int(rows[0]['n']):,}")
    figure.tight_layout()
    save_figure(figure, figures, "d_merge_comparison.png")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--results", type=Path, default=Path("results"))
    arguments = parser.parse_args()
    figures = arguments.results / "figures"
    figures.mkdir(parents=True, exist_ok=True)
    plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 10})
    plot_input_sizes(arguments.results, figures)
    plot_threshold_sweep(arguments.results, figures)
    plot_threshold_tuning(arguments.results, figures)
    plot_merge_comparison(arguments.results, figures)


if __name__ == "__main__":
    main()
