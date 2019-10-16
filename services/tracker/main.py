#!/usr/bin/env python3
import argparse
import asyncio
import logging
import time

import asyncpg

from app.common import configure_logging, connect_db
from app.config import get_config
from app.network import create_server_socket
from app.protocol import handle_client


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--drop", action="store_true")
    return parser.parse_args()


async def create_tables(db):
    with open("db/create.sql") as f:
        sql = f.read()
    try:
        await db.execute(sql)
        logging.info("Empty DB found. New tables have been created.")
    except asyncpg.exceptions.DuplicateTableError:
        pass


async def drop_tables(conn):
    logging.warning("Dropping database tables ...")
    with open("db/drop.sql") as f:
        sql = f.read()
    await conn.execute(sql)
    logging.warning("All tables have been dropped.")


async def async_main(loop, config, args):
    db = await wait_for_db_connection()
    if args.drop:
        await drop_tables(db)
        return
    await create_tables(db)
    await db.close()

    server = create_server_socket(config.host, config.port)
    while True:
        client_sock, _ = await loop.sock_accept(server)
        loop.create_task(handle_client(client_sock, loop))


async def wait_for_db_connection():
    db_retry_time_sec = 5
    while True:
        try:
            return await connect_db()
        except Exception as e:
            logging.error("DB not available: %s. Retrying in %d seconds ...", e, db_retry_time_sec)
            time.sleep(db_retry_time_sec)


def main():
    args = parse_args()
    config = get_config()
    configure_logging(config.debug)

    loop = asyncio.get_event_loop()
    loop.run_until_complete(async_main(loop, config, args))


if __name__ == "__main__":
    main()
