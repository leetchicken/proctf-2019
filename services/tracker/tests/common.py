import msgpack
import socket

import pytest

from app.common import connect_db
from app.enums import Response, Request

BUFFER_SIZE = 1024

SOCK_DCCP = 6
IPROTO_DCCP = 33
SOL_DCCP = 269
DCCP_SOCKOPT_SERVICE = 2


class Client:
    def __init__(self, host="127.0.0.1", port=9090):
        sock = socket.socket(socket.AF_INET, SOCK_DCCP, IPROTO_DCCP)
        sock.setsockopt(SOL_DCCP, DCCP_SOCKOPT_SERVICE, True)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.connect((host, port))
        self.sock = sock
        self.key = self.receive_key()

    def query(self, *args, expect=None):
        response = self.query_raw(args)
        if expect is not None:
            assert response[0] == expect
            response = response[1:]
        return response[0] if len(response) == 1 else response

    def receive_key(self):
        key_raw = self.sock.recv(BUFFER_SIZE)
        if len(key_raw) != 1:
            raise Exception("Unexpected key size")
        return key_raw[0]

    def query_raw(self, obj):
        raw_bytes = msgpack.packb(obj)
        raw_bytes = bytes(b ^ ((self.key + i) & 0xFF) for i, b in enumerate(raw_bytes))
        self.sock.send(raw_bytes)
        return msgpack.unpackb(self.sock.recv(BUFFER_SIZE), raw=False)


@pytest.fixture
def client():
    return Client()


@pytest.mark.asyncio
@pytest.fixture(autouse=True)
async def cleanup():
    db = await connect_db()
    try:
        for table in ["user", "secret", "tracker", "point", "track", "zone"]:
            await db.execute('DELETE FROM "{}"'.format(table))
    finally:
        await db.close()


def create_secret(client, login="test", password="1234567890"):
    client.query(Request.USER_REGISTER, login, password, expect=Response.OK)
    return client.query(Request.USER_LOGIN, login, password, expect=Response.OK)


@pytest.fixture
def secret(client):
    return create_secret(client)


@pytest.fixture
def token(client, secret):
    return client.query(Request.TRACKER_ADD, secret, "test", expect=Response.OK)
