import time

import pytest

from tests.common import *
from app.enums import Request, Response


def test__add(client, token):
    track_id_1 = client.query(Request.POINT_ADD, token, 1.0, 2.0, "x", expect=Response.OK)
    time.sleep(0.2)
    track_id_2 = client.query(Request.POINT_ADD, token, 1.1, 2.1, "y", expect=Response.OK)
    assert track_id_1 == track_id_2
    time.sleep(3)
    track_id_3 = client.query(Request.POINT_ADD, token, 1.2, 2.2, "z", expect=Response.OK)
    assert track_id_3 != track_id_2
