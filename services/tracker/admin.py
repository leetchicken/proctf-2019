#!/usr/bin/env python3
import argparse
import asyncio
import logging
from collections import OrderedDict
from typing import Callable

import asyncpg

from app.common import configure_logging
from app.config import get_config


log = logging.getLogger()


async def drop(conn):
    with open("db/drop.sql") as f:
        sql = f.read()
    await conn.execute(sql)
    log.info("All tables have been dropped.")


async def create(conn):
    with open("db/create.sql") as f:
        sql = f.read()
    await conn.execute(sql)
    log.info("All tables have been created.")


COMMANDS = OrderedDict([
    ("create", create),
    ("drop", drop),
    # ("dump", dump),
    # ("count", count)
])


async def do(command: Callable):
    config = get_config()
    conn = await asyncpg.connect(config.database_uri)
    await command(conn)
    await conn.close()


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=COMMANDS.keys())
    return parser.parse_args()


def main():
    configure_logging()
    args = parse_args()
    command = COMMANDS[args.command]
    asyncio.get_event_loop().run_until_complete(do(command))


if __name__ == "__main__":
    main()
