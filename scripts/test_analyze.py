import io
import math
import unittest

import analyze

CSV = """variant,width,height,ksize,threads,run,time_ms
seq,8,8,3,1,0,10.0
seq,8,8,3,1,1,12.0
ai,8,8,3,1,0,10.0
ai,8,8,3,1,1,10.0
ai,8,8,3,2,0,5.0
ai,8,8,3,2,1,5.0
"""


class AnalyzeTest(unittest.TestCase):
    def setUp(self):
        self.rows = analyze.load(io.StringIO(CSV))
        self.s = analyze.summarize(self.rows)
        analyze.add_speedup(self.s)

    def test_mean_and_sample_std(self):
        seq = self.s[("seq", 8, 3, 1)]
        self.assertAlmostEqual(seq["mean"], 11.0)
        self.assertAlmostEqual(seq["std"], math.sqrt(2.0))
        self.assertEqual(seq["n"], 2)

    def test_speedup_and_efficiency_vs_sequential(self):
        ai2 = self.s[("ai", 8, 3, 2)]
        self.assertAlmostEqual(ai2["sp"], 2.2)
        self.assertAlmostEqual(ai2["ep"], 1.1)
        self.assertAlmostEqual(ai2["sp_self"], 2.0)
        self.assertAlmostEqual(ai2["ep_rel"], 1.0)

    def test_overview_table_lists_seq_time_and_best_speedup(self):
        buf = io.StringIO()
        analyze.write_overview_table(self.s, buf)
        tex = buf.getvalue()
        self.assertIn(r"8$\times$8 & 3$\times$3 & 11.00", tex)
        self.assertIn("2.20 (2)", tex)

    def test_median_is_reported(self):
        self.assertAlmostEqual(self.s[("seq", 8, 3, 1)]["median"], 11.0)

    def test_mann_whitney_separated_samples(self):
        p = analyze.mann_whitney_p([1, 2, 3, 4, 5], [6, 7, 8, 9, 10])
        self.assertAlmostEqual(p, 0.0122, places=3)

    def test_mann_whitney_identical_samples(self):
        self.assertAlmostEqual(analyze.mann_whitney_p([5, 5, 5], [5, 5, 5]), 1.0)

    def test_latex_table_kernel_filter(self):
        import os
        import tempfile
        rows = self.rows + [dict(r, ksize=5) for r in self.rows]
        s = analyze.summarize(rows)
        analyze.add_speedup(s)
        path = os.path.join(tempfile.mkdtemp(), "t.tex")
        analyze.write_latex_table(s, 8, path, ksizes=[5])
        tex = open(path, encoding="utf-8").read()
        self.assertIn("5$" + chr(92) + "times$5", tex)
        self.assertNotIn("3$" + chr(92) + "times$3", tex)

    def test_sensitivity_table_compares_flags(self):
        alt = {k: dict(v, mean=v["mean"] / 2) for k, v in self.s.items()}
        analyze.add_speedup(alt)
        buf = io.StringIO()
        analyze.write_sensitivity_table(self.s, alt, 8, buf, threads=[1, 2])
        tex = buf.getvalue()
        self.assertIn("IA directa & 3 & 2 & 5.00 & 2.50 & 0.50", tex)

    def test_mann_whitney_csv_has_one_row_per_shared_config(self):
        import os
        import tempfile
        path = os.path.join(tempfile.mkdtemp(), "mw.csv")
        analyze.write_mann_whitney_csv(self.rows, "ai", "seq", path)
        lines = open(path, encoding="utf-8").read().strip().splitlines()
        self.assertEqual(lines[0], "n,ksize,threads,p_value,median_a_ms,median_b_ms")
        self.assertEqual(len(lines), 2)
        self.assertTrue(lines[1].startswith("8,3,1,"))

    def test_missing_seq_raises(self):
        rows = [r for r in self.rows if r["variant"] != "seq"]
        s = analyze.summarize(rows)
        with self.assertRaisesRegex(ValueError, "seq"):
            analyze.add_speedup(s)


if __name__ == "__main__":
    unittest.main()
