import pytest

from tests.common import *
from app.enums import Request, Response


def test__tracker_list(client, secret):
    client.query(Request.TRACKER_LIST, secret, expect=Response.OK)


def test__tracker_list__bad_secret(client):
    client.query(Request.TRACKER_LIST, "bad_secret", expect=Response.FORBIDDEN)


def test__tracker_add_list(client, secret):
    trackers = client.query(Request.TRACKER_LIST, secret, expect=Response.OK)
    assert len(trackers) == 0

    client.query(Request.TRACKER_ADD, secret, "foo", expect=Response.OK)
    trackers = client.query(Request.TRACKER_LIST, secret, expect=Response.OK)
    assert len(trackers) == 1
    assert trackers[0][1] == "foo"

    client.query(Request.TRACKER_ADD, secret, "bar", expect=Response.OK)
    trackers = client.query(Request.TRACKER_LIST, secret, expect=Response.OK)
    assert sorted([name for _, name in trackers]) == ["bar", "foo"]


def test__tracker_add_delete_list(client, secret):
    client.query(Request.TRACKER_ADD, secret, "foo", expect=Response.OK)
    trackers = client.query(Request.TRACKER_LIST, secret, expect=Response.OK)
    assert len(trackers) == 1

    tracker_id = trackers[0][0]
    client.query(Request.TRACKER_DELETE, secret, tracker_id, expect=Response.OK)
    client.query(Request.TRACKER_DELETE, secret, tracker_id, expect=Response.NOT_FOUND)

    trackers = client.query(Request.TRACKER_LIST, secret, expect=Response.OK)
    assert len(trackers) == 0


def test__tracker_add_limit(client, secret):
    for i in range(8):
        client.query(Request.TRACKER_ADD, secret, "foo-{}".format(i), expect=Response.OK)

    assert client.query(Request.TRACKER_ADD, secret, "bar") == [Response.BAD_REQUEST, "Too many trackers."]

    trackers = client.query(Request.TRACKER_LIST, secret, expect=Response.OK)
    assert len(trackers) == 8
