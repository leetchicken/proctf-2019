#!/usr/bin/env python3

import asyncio

from app.common import configure_logging
from app.config import get_config
from app.network import create_server_socket
from app.protocol import handle_client, register_handlers


async def run_server():
    while True:
        client_sock, _ = await loop.sock_accept(server)
        loop.create_task(handle_client(client_sock, loop))


config = get_config()

configure_logging(config.debug)
register_handlers()

server = create_server_socket(config.host, config.port)

loop = asyncio.get_event_loop()
loop.run_until_complete(run_server())
