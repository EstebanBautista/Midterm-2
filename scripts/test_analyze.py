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

    def test_overview_table_lists_seq_time_and_best_speedup(self):
        buf = io.StringIO()
        analyze.write_overview_table(self.s, buf)
        tex = buf.getvalue()
        self.assertIn(r"8$\times$8 & 3$\times$3 & 11.00", tex)
        self.assertIn("2.20 (2)", tex)

    def test_missing_seq_raises(self):
        rows = [r for r in self.rows if r["variant"] != "seq"]
        s = analyze.summarize(rows)
        with self.assertRaisesRegex(ValueError, "seq"):
            analyze.add_speedup(s)


if __name__ == "__main__":
    unittest.main()
