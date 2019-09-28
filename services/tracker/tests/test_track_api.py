import time

from tests.common import *
from app.enums import Request, Response, TrackAccess


def point_add(client, token, lat=1.0, lon=2.0, meta="x"):
    return client.query(Request.POINT_ADD, token, lat, lon, meta, expect=Response.OK)


def track_list(client, secret):
    return client.query(Request.TRACK_LIST, secret, expect=Response.OK)


def track_get(client, secret, track_id, expect=Response.OK):
    return client.query(Request.TRACK_GET, secret, track_id, expect=expect)


def test__track_list__empty(client, secret):
    assert track_list(client, secret) == []


def test__track_list__one(client, secret, token):
    track_id = point_add(client, token)
    tracks = track_list(client, secret)
    assert len(tracks) == 1
    assert tracks[0][0] == track_id
    assert tracks[0][1] - time.time() <= 2  # created_at
    assert tracks[0][2] == TrackAccess.PRIVATE  # access


def test__track_list__two(client, secret, token):
    track_id_1 = point_add(client, token)
    time.sleep(3)
    track_id_2 = point_add(client, token)
    tracks = track_list(client, secret)
    assert len(tracks) == 2
    assert sorted([track_id for track_id, _, _ in tracks]) == sorted([track_id_1, track_id_2])


def test__track_delete(client, secret, token):
    track_id = point_add(client, token)
    assert len(track_list(client, secret)) == 1
    client.query(Request.TRACK_DELETE, secret, track_id, expect=Response.OK)
    assert len(track_list(client, secret)) == 0


def test__track_get(client, secret, token):
    track_id = point_add(client, token, lat=1.2, lon=3.4, meta="f")
    point_add(client, token, lat=5.6, lon=7.8, meta="o")
    point_add(client, token, lat=9.0, lon=1.2, meta="o")

    points = track_get(client, secret, track_id)
    assert len(points) == 3

    assert (points[0][1], points[0][2], points[0][3]) == (1.2, 3.4, "f")
    assert (points[1][1], points[1][2], points[1][3]) == (5.6, 7.8, "o")
    assert (points[2][1], points[2][2], points[2][3]) == (9.0, 1.2, "o")


def test__track_request_share__make_public(client, secret, token):
    track_id = point_add(client, token)

    other_secret = create_secret(client, login="test2")
    track_get(client, other_secret, track_id, expect=Response.FORBIDDEN)

    client.query(Request.TRACK_REQUEST_SHARE, other_secret, track_id)
    client.query(Request.TRACK_SHARE, secret, track_id)

    points = track_get(client, other_secret, track_id, expect=Response.OK)
    assert len(points) == 1


def test__track_request_share__vulnerability(client, secret, token):
    track_id = point_add(client, token)

    other_secret = create_secret(client, login="test2")
    track_get(client, other_secret, track_id, expect=Response.FORBIDDEN)

    client.query(Request.TRACK_REQUEST_SHARE, other_secret, track_id)
    points = track_get(client, other_secret, track_id, expect=Response.OK)
    assert len(points) == 1
