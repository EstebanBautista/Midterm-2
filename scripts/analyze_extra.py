"""Complementary analyses: Mann-Whitney ai vs ai_opt and -O3 -march=native sensitivity table."""
import os

import analyze

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    raw = os.path.join(ROOT, "results", "raw.csv")
    rows = analyze.load(raw)
    analyze.write_mann_whitney_csv(rows, "ai", "ai_opt",
                                   os.path.join(ROOT, "results", "mann_whitney_ai_vs_ai_opt.csv"))
    print("wrote results/mann_whitney_ai_vs_ai_opt.csv")

    sens_raw = os.path.join(ROOT, "results", "sensitivity_o3native", "raw.csv")
    if os.path.exists(sens_raw):
        base = analyze.summarize(rows)
        analyze.add_speedup(base)
        alt = analyze.summarize(analyze.load(sens_raw))
        analyze.add_speedup(alt)
        analyze.write_summary_csv(alt, os.path.join(ROOT, "results", "sensitivity_o3native", "summary.csv"))
        with open(os.path.join(ROOT, "paper", "tables", "tabla_sensibilidad.tex"), "w", encoding="utf-8") as f:
            analyze.write_sensitivity_table(base, alt, 4096, f)
        print("wrote results/sensitivity_o3native/summary.csv and paper/tables/tabla_sensibilidad.tex")


if __name__ == "__main__":
    main()
