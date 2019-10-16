import logging
import random
import sys

import msgpack

from app.common import connect_db
from app.enums import Response

MESSAGE_SIZE = 1024

handlers = {}


def register_handlers():
    import inspect
    for module_name, module in list(sys.modules.items()):
        if module_name != "__main__" and not module_name.startswith("app.api."):
            continue
        for obj_name, obj in inspect.getmembers(module):
            if inspect.isfunction(obj):
                func = obj
                if hasattr(obj, "request_id"):
                    request_id = func.request_id
                    if request_id in handlers:
                        logging.warning("Handler with request_id %d already registered, %s/%s skipped.",
                                        request_id, module_name, obj_name)
                        continue
                    handlers[func.request_id] = func
                    logging.debug("Registered handler: %s/%s", module_name, obj_name)


class Client:
    def __init__(self, client_sock, loop):
        self.client_sock = client_sock
        self.loop = loop
        self.key = random.randint(0, 255)

    def __enter__(self):
        self.host_port = self.client_sock.getpeername()
        logging.info("Client connected: %s:%d", *self.host_port)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        logging.info("Client disconnected: %s:%d", *self.host_port)
        self.client_sock.close()

    async def recv(self):
        raw_request = await self.loop.sock_recv(self.client_sock, MESSAGE_SIZE)
        if not raw_request:
            # Probably client disconnected
            return None
        if len(raw_request) >= MESSAGE_SIZE:
            logging.warning(" <=  [Too big: %d bytes]", len(raw_request))
            await self.send((Response.BAD_REQUEST, "Too big request."))
            return None

        raw_request = bytes(b ^ ((self.key + i) & 0xFF) for i, b in enumerate(raw_request))
        request = msgpack.unpackb(raw_request, raw=False)
        logging.debug(" <=  %s", request)
        return request

    async def send_key(self):
        await self.loop.sock_sendall(self.client_sock, bytes([self.key]))

    async def send(self, response):
        logging.debug(" =>  %s", response)
        raw_response = msgpack.packb(response)
        await self.loop.sock_sendall(self.client_sock, raw_response)


async def handle_request(request):
    if not isinstance(request, list):
        return Response.BAD_REQUEST, "Wrong data type."
    if not request:
        return Response.BAD_REQUEST, "Empty request."

    request_type = request[0]
    handler = handlers.get(request_type)
    if not handler:
        return Response.BAD_REQUEST, "Unknown request type."

    db = await connect_db()
    try:
        response = await handler(db, *request[1:])
    finally:
        await db.close()
    if not isinstance(response, (list, tuple)):
        response = (response, )
    return response


async def handle_client(client_sock, loop):
    with Client(client_sock, loop) as client:
        await client.send_key()
        while True:
            try:
                request = await client.recv()
                if request is None:
                    break
                response = await handle_request(request)
                await client.send(response)
            except Exception as e:
                logging.exception("Exception while handling client request")
                try:
                    await client.send([Response.INTERNAL_ERROR])
                except Exception as e:
                    logging.warning("Exception while sending error to client: %s", e)
                break
