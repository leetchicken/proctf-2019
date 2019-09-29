#!/usr/bin/env python3
import argparse
import time
from collections import Counter

import checker
import logging
import math
import random


# http://code.activestate.com/recipes/511478/
def percentile(N, percent, key=lambda x:x):
    """
    Find the percentile of a list of values.

    @parameter N - is a list of values. Note N MUST BE already sorted.
    @parameter percent - a float value from 0.0 to 1.0.
    @parameter key - optional key function to compute value from each element of N.

    @return - the percentile of the values
    """
    if not N:
        return None
    k = (len(N)-1) * percent
    f = math.floor(k)
    c = math.ceil(k)
    if f == c:
        return key(N[int(k)])
    d0 = key(N[int(f)]) * (c-k)
    d1 = key(N[int(c)]) * (k-f)
    return d0 + d1


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--iterations", metavar="N", type=int, required=True)
    parser.add_argument("--host", metavar="IP", default="127.0.0.1")
    return parser.parse_args()


def main():
    logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)-8s %(message)s")

    args = parse_args()

    check_results, check_timings = Counter(), []
    global_start = time.monotonic()

    rnd = random.randint(0, 100000)
    flag = "QQ539Y5FAJ7A8VV8WHZDM3C15A22A55="

    for i in range(args.iterations):
        start = time.monotonic()
        # result = checker.check(args.host)
        result = checker.put(args.host, "id-{}-{}".format(rnd, i), flag)
        elapsed_ms = (time.monotonic() - start) * 1000

        check_results[result] += 1
        check_timings.append(elapsed_ms)

        logging.info(">>>: result=%d, elapsed=%d ms", result, elapsed_ms)

    global_elapsed_sec = time.monotonic() - global_start

    logging.info("RESULTS")
    for code in sorted(check_results.keys()):
        logging.info("  code %3d was met %3d time(s)", code, check_results[code])

    logging.info("TIMES (%d requests in %f sec ~~ %f RPS)", len(check_timings), global_elapsed_sec, len(check_timings)/global_elapsed_sec)
    logging.info("  max    = %5d ms", max(check_timings))
    logging.info("  p99    = %5d ms", percentile(check_timings, 0.99))
    logging.info("  p95    = %5d ms", percentile(check_timings, 0.95))
    logging.info("  median = %5d ms", percentile(check_timings, 0.5))
    logging.info("  min    = %5d ms", min(check_timings))


if __name__ == "__main__":
    main()
