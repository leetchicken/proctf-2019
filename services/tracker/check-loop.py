#!/usr/bin/env python3
import time
from collections import Counter

import checker
import logging
import random
import string

HOST = "127.0.0.1"


def rand_str(charset, length):
    return ''.join(random.choice(charset) for i in range(length))


def gen_flag():
    return rand_str(string.ascii_uppercase, 31) + "="


def gen_id():
    return rand_str(string.ascii_lowercase, 16)


class MeasureTime:
    def __init__(self, logger, name):
        self.round = round
        self.name = name
        self.logger = logger
        self.logger.info("Starting %r ...", name)

    def __enter__(self):
        self.start = time.monotonic()
        return self

    def __exit__(self, type, value, traceback):
        duration_ms = (time.monotonic() - self.start) * 1000
        self.logger.info("Finished %r, elapsed %d ms", self.name, duration_ms)


def main():
    flags = dict()
    check_counter = Counter()
    put_counter = Counter()
    get_counter = Counter()
    get_rand_counter = Counter()

    logging.basicConfig(level=logging.DEBUG,
                        filename="check-loop.log",
                        format="%(asctime)s %(name)s %(levelname)-8s %(message)s")
    root_logger = logging.getLogger("loop")

    round = 1
    while True:
        logger = logging.LoggerAdapter(root_logger, extra={"round": round})

        round_start = time.monotonic()
        logger.info("[%d] Starting round ...", round)

        with MeasureTime(logger, "check"):
            result = checker.check(HOST)
        logger.info("[%d] Check result: %d", round, result)
        check_counter[result] += 1

        id = gen_id()
        flag = gen_flag()
        with MeasureTime(logger, "put"):
            result = checker.put(HOST, id, flag)
        logger.info("[%d] put result: %d", round, result)
        put_counter[result] += 1
        if result == checker.ExitCode.OK:
            flags[id] = flag

        with MeasureTime(logger, "get"):
            result = checker.get(HOST, id, flag)
        logger.info("[%d] get result: %d", round, result)
        get_counter[result] += 1

        rand_id, rand_flag = random.choice(list(flags.items()))
        with MeasureTime(logger, "get_rand"):
            result = checker.get(HOST, rand_id, rand_flag)
        logger.info("[%d] get_rand result: %d", round, result)
        get_rand_counter[result] += 1

        round_duration_ms = (time.monotonic() - round_start) * 1000
        logger.info("[%d] Round end. Elapsed: %d ms. Flags: %d", round, round_duration_ms, len(flags))

        summary_format = "[%d] Check: %s. Put: %s. Get: %s. Get_Rand: %s"
        summary_args = (round, check_counter, put_counter, get_counter, get_rand_counter)
        logger.info(summary_format, *summary_args)
        print(summary_format % summary_args)

        round += 1


if __name__ == "__main__":
    main()
