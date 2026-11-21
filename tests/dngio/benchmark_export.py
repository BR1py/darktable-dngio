"""Measure export wall time and CPU use; select a build with DARKTABLE_CLI."""

import argparse
import json
import os
import time

from test_dng import CLI, DngTests


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--threads", type=int, default=2, help="DT CPU thread limit (default: 2)")
    args = parser.parse_args()
    if not CLI:
        parser.error("set DARKTABLE_CLI to the executable to benchmark")
    if args.threads < 1:
        parser.error("--threads must be positive")

    cases = (("dng-no-preview", "TEST2.dng", 0, "false"),
             ("dng-preview", "TEST2.dng", 25, "false"),
             ("camera-raw", "TEST1.ARW", 0, "false"),
             ("crop-path", "TEST6.dng", 0, "true"))
    for name, fixture, preview, crop in cases:
        for repeat in range(2):
            case = DngTests()
            case.setUp()
            case.threads = args.threads
            before = os.times()
            start = time.perf_counter()
            try:
                case.export(case.fixture(fixture), "output.dng",
                            preview_size=preview, consider_crop=crop)
            finally:
                case.doCleanups()
            seconds = time.perf_counter() - start
            after = os.times()
            cpu = after.children_user + after.children_system - before.children_user - before.children_system
            print(json.dumps(dict(case=name, repeat=repeat, threads=args.threads,
                                  seconds=round(seconds, 3), cpu_seconds=round(cpu, 3),
                                  average_cpu_cores=round(cpu / seconds, 2))), flush=True)


if __name__ == "__main__":
    main()
