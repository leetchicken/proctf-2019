from typing import List

import asyncio
import aiohttp
from aiohttp.client_proto import ResponseHandler
from aiohttp.client import ClientRequest, ClientTimeout, Trace, TCPConnector
from random import choice

with open("user-agents") as user_agents:
    AGENTS = [x.strip() for x in user_agents.readlines()]


class MaskingConnector(aiohttp.BaseConnector):
    async def _create_connection(self, req: ClientRequest, traces: List[Trace],
                                 timeout: ClientTimeout) -> ResponseHandler:
        req.headers["User-Agent"] = choice(AGENTS)
        return await TCPConnector(loop=asyncio.get_event_loop())\
            ._create_connection(req, traces, timeout)
