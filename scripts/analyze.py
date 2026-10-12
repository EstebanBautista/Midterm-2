"""Statistics, figures and LaTeX tables for the convolution benchmark."""
import csv
import os
import sys
from collections import defaultdict

import numpy as np

VARIANT_ORDER = ["seq", "ai", "ai_opt", "manual"]
LABELS = {
    "seq": "Secuencial",
    "ai": "IA directa",
    "ai_opt": "IA + opt. equipo",
    "manual": "Manual (GitHub)",
}


def load(src):
    """Reads the raw CSV from a path or a file-like object."""
    f = open(src, newline="") if isinstance(src, str) else src
    try:
        return [
            {
                "variant": r["variant"],
                "n": int(r["width"]),
                "ksize": int(r["ksize"]),
                "threads": int(r["threads"]),
                "time_ms": float(r["time_ms"]),
            }
            for r in csv.DictReader(f)
        ]
    finally:
        if isinstance(src, str):
            f.close()


def summarize(rows):
    groups = defaultdict(list)
    for r in rows:
        groups[(r["variant"], r["n"], r["ksize"], r["threads"])].append(r["time_ms"])
    out = {}
    for key, times in groups.items():
        t = np.array(times)
        std = float(t.std(ddof=1)) if len(t) > 1 else 0.0
        mean = float(t.mean())
        out[key] = {"mean": mean, "std": std, "min": float(t.min()),
                    "max": float(t.max()), "cv": std / mean, "n": len(t),
                    "median": float(np.median(t))}
    return out


def add_speedup(summary):
    for (variant, n, ks, p), st in summary.items():
        seq = summary.get(("seq", n, ks, 1))
        if seq is None:
            raise ValueError(f"missing seq baseline for n={n} ksize={ks}")
        own1 = summary.get((variant, n, ks, 1))
        st["sp"] = seq["mean"] / st["mean"]
        st["ep"] = st["sp"] / p
        st["sp_self"] = own1["mean"] / st["mean"] if own1 else float("nan")
        st["ep_rel"] = st["sp_self"] / p


def mann_whitney_p(a, b):
    """Two-sided Mann-Whitney U test, normal approximation with tie and continuity correction."""
    a = np.asarray(a, dtype=float)
    b = np.asarray(b, dtype=float)
    n1, n2 = len(a), len(b)
    allv = np.concatenate([a, b])
    order = allv.argsort(kind="mergesort")
    ranks = np.empty(len(allv))
    sorted_v = allv[order]
    i = 0
    tie_term = 0.0
    while i < len(sorted_v):
        j = i
        while j + 1 < len(sorted_v) and sorted_v[j + 1] == sorted_v[i]:
            j += 1
        ranks[order[i:j + 1]] = (i + j) / 2.0 + 1.0
        t = j - i + 1
        tie_term += t ** 3 - t
        i = j + 1
    u1 = ranks[:n1].sum() - n1 * (n1 + 1) / 2.0
    mu = n1 * n2 / 2.0
    n = n1 + n2
    var = n1 * n2 / 12.0 * ((n + 1) - tie_term / (n * (n - 1)))
    if var <= 0:
        return 1.0
    z = (abs(u1 - mu) - 0.5) / np.sqrt(var)
    if z <= 0:
        return 1.0
    from math import erfc, sqrt
    return float(erfc(z / sqrt(2.0)))


def compare_variants(rows, a, b):
    """Mann-Whitney p-value per (n, ksize, threads) between variants a and b."""
    groups = defaultdict(list)
    for r in rows:
        groups[(r["variant"], r["n"], r["ksize"], r["threads"])].append(r["time_ms"])
    out = {}
    for (v, n, ks, p), times in groups.items():
        if v == a and (b, n, ks, p) in groups:
            other = groups[(b, n, ks, p)]
            out[(n, ks, p)] = {"p": mann_whitney_p(times, other),
                               "median_a": float(np.median(times)),
                               "median_b": float(np.median(other))}
    return out


def write_summary_csv(summary, path):
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["variant", "n", "ksize", "threads", "mean_ms", "std_ms", "min_ms",
                    "max_ms", "cv", "runs", "speedup", "efficiency", "speedup_self",
                    "median_ms", "efficiency_self"])
        for key in sorted(summary, key=lambda k: (k[1], k[2], VARIANT_ORDER.index(k[0]), k[3])):
            s = summary[key]
            w.writerow([*key, f"{s['mean']:.4f}", f"{s['std']:.4f}", f"{s['min']:.4f}",
                        f"{s['max']:.4f}", f"{s['cv']:.4f}", s["n"], f"{s['sp']:.4f}",
                        f"{s['ep']:.4f}", f"{s['sp_self']:.4f}", f"{s['median']:.4f}",
                        f"{s['ep_rel']:.4f}"])


def plot_metric(summary, n, metric, ylabel, path):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    ksizes = sorted({k[2] for k in summary if k[1] == n})
    threads = sorted({k[3] for k in summary if k[1] == n})
    fig, axes = plt.subplots(2, 2, figsize=(9, 7), sharex=True)
    for ax, ks in zip(axes.flat, ksizes):
        for v in VARIANT_ORDER:
            if v == "seq":
                continue
            pts = [(p, summary[(v, n, ks, p)]) for p in threads if (v, n, ks, p) in summary]
            if not pts:
                continue
            xs = [p for p, _ in pts]
            ys = [s[metric] for _, s in pts]
            err = [s["std"] for _, s in pts] if metric == "mean" else None
            ax.errorbar(xs, ys, yerr=err, marker="o", capsize=3, label=LABELS[v])
        if metric == "mean" and ("seq", n, ks, 1) in summary:
            ax.axhline(summary[("seq", n, ks, 1)]["mean"], color="gray", ls="--", label=LABELS["seq"])
        if metric in ("sp", "sp_self"):
            ax.plot(threads, threads, color="gray", ls=":", label="Ideal")
        if metric == "ep":
            ax.axhline(1.0, color="gray", ls=":", label="Ideal")
        if metric == "mean":
            ax.set_yscale("log")
        if 8 in threads and max(threads) > 8:
            ax.axvline(8, color="red", ls="--", lw=0.8, alpha=0.6, label="8 núcleos físicos")
        ax.set_xscale("log", base=2)
        ax.set_xticks(threads)
        ax.set_xticklabels([str(t) for t in threads])
        ax.set_title(f"Kernel {ks}×{ks}")
        ax.set_xlabel("Hilos (p)")
        ax.set_ylabel(ylabel)
        ax.grid(True, alpha=0.3)
    axes.flat[0].legend(fontsize=8)
    fig.suptitle(f"Imagen {n}×{n}")
    fig.tight_layout()
    fig.savefig(path + ".pdf")
    fig.savefig(path + ".png", dpi=150)
    plt.close(fig)


def write_latex_table(summary, n, path, ksizes=None):
    ksizes = sorted(ksizes or {k[2] for k in summary if k[1] == n})
    threads = sorted({k[3] for k in summary if k[1] == n})
    lines = [
        "\\begin{tabular}{llrrrrr}",
        "\\toprule",
        "Kernel & Variante & $p$ & $\\bar{T}$ (ms) $\\pm\\sigma$ & $S_p$ & $E_p$ & $E^{rel}_p$ \\\\",
        "\\midrule",
    ]
    for ks in ksizes:
        for v in VARIANT_ORDER:
            for p in threads:
                s = summary.get((v, n, ks, p))
                if s is None:
                    continue
                lines.append(f"{ks}$\\times${ks} & {LABELS[v]} & {p} & "
                             f"{s['mean']:.2f} $\\pm$ {s['std']:.2f} & {s['sp']:.2f} & {s['ep']:.2f} & {s['ep_rel']:.2f} \\\\")
        lines.append("\\midrule")
    lines[-1] = "\\bottomrule"
    lines.append("\\end{tabular}")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def write_overview_table(summary, out):
    """One row per (image, kernel): sequential time and best speedup (with its p) per variant."""
    variants = [v for v in VARIANT_ORDER[1:] if any(k[0] == v for k in summary)]
    cols = "ll" + "r" * (1 + len(variants))
    head = " & ".join(LABELS[v] for v in variants)
    lines = [f"\\begin{{tabular}}{{{cols}}}", "\\toprule",
             f"Imagen & Kernel & $T_{{seq}}$ (ms) & {head} \\\\", "\\midrule"]
    for n, ks in sorted({(k[1], k[2]) for k in summary}):
        cells = []
        for v in variants:
            pts = [(s["sp"], k[3]) for k, s in summary.items() if k[0] == v and k[1] == n and k[2] == ks]
            sp, p = max(pts)
            cells.append(f"{sp:.2f} ({p})")
        seq = summary[("seq", n, ks, 1)]["mean"]
        lines.append(f"{n}$\\times${n} & {ks}$\\times${ks} & {seq:.2f} & " + " & ".join(cells) + " \\\\")
    lines += ["\\bottomrule", "\\end{tabular}"]
    out.write("\n".join(lines) + "\n")


def write_sensitivity_table(base, alt, n, out, threads=(1, 16)):
    """Compares mean time and speedup between the main build (base) and an alternative build (alt)."""
    bs = chr(92)
    lines = [bs + "begin{tabular}{lrrrrrrr}", bs + "toprule",
             "Variante & Kernel & $p$ & $" + bs + "bar{T}_{O2}$ (ms) & $" + bs + "bar{T}_{O3}$ (ms) & "
             "$" + bs + "bar{T}_{O3}/" + bs + "bar{T}_{O2}$ & $S_p^{O2}$ & $S_p^{O3}$ " + bs + bs,
             bs + "midrule"]
    for v in VARIANT_ORDER:
        for ks in sorted({k[2] for k in base if k[1] == n}):
            for p in threads:
                key = (v, n, ks, p)
                if key not in base or key not in alt:
                    continue
                t0, t1 = base[key]["mean"], alt[key]["mean"]
                lines.append(f"{LABELS[v]} & {ks} & {p} & {t0:.2f} & {t1:.2f} & {t1 / t0:.2f} & "
                             f"{base[key]['sp']:.2f} & {alt[key]['sp']:.2f} " + bs + bs)
    lines += [bs + "bottomrule", bs + "end{tabular}"]
    out.write("\n".join(lines) + "\n")


def main(raw_path):
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    summary = summarize(load(raw_path))
    add_speedup(summary)
    fig_dir = os.path.join(root, "results", "figures")
    tab_dir = os.path.join(root, "paper", "tables")
    os.makedirs(fig_dir, exist_ok=True)
    os.makedirs(tab_dir, exist_ok=True)
    write_summary_csv(summary, os.path.join(root, "results", "summary.csv"))
    for n in sorted({k[1] for k in summary}):
        plot_metric(summary, n, "mean", "Tiempo (ms)", os.path.join(fig_dir, f"tiempo_{n}"))
        plot_metric(summary, n, "sp", "Speedup $S_p$", os.path.join(fig_dir, f"speedup_{n}"))
        plot_metric(summary, n, "ep", "Eficiencia $E_p$", os.path.join(fig_dir, f"eficiencia_{n}"))
        plot_metric(summary, n, "sp_self", "Speedup relativo $T_1/T_p$",
                    os.path.join(fig_dir, f"speedup_relativo_{n}"))
        write_latex_table(summary, n, os.path.join(tab_dir, f"tabla_{n}.tex"))
        write_latex_table(summary, n, os.path.join(tab_dir, f"tabla_{n}_a.tex"), ksizes=[3, 5])
        write_latex_table(summary, n, os.path.join(tab_dir, f"tabla_{n}_b.tex"), ksizes=[7, 11])
    with open(os.path.join(tab_dir, "tabla_resumen.tex"), "w", encoding="utf-8") as f:
        write_overview_table(summary, f)
    print(f"analyzed {len(summary)} configurations")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "results/raw.csv")
