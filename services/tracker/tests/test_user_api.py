from tests.common import *
from app.enums import Request, Response


LOGIN = "test"
PASSWORD = "1234567890"


def test__wrong_data_type(client):
    expected = [Response.BAD_REQUEST, "Wrong data type."]

    assert client.query_raw("abc") == expected
    assert client.query_raw({"key": "value"}) == expected


def test__empty_request(client):
    assert client.query_raw([]) == [Response.BAD_REQUEST, "Empty request."]


def test__user_register__not_enough_args(client):
    client.query(Request.USER_REGISTER, expect=Response.INTERNAL_ERROR)


def test__user_register__short_password(client):
    assert client.query(Request.USER_REGISTER, LOGIN, "123") \
           == [Response.BAD_REQUEST, "Password is too short."]


def test__user_register__long_password(client):
    long_password = "x" * 65
    assert client.query(Request.USER_REGISTER, LOGIN, long_password) \
           == [Response.BAD_REQUEST, "Password is too long."]


def test__user_register__big_request(client):
    long_password = "x" * 1024
    assert client.query(Request.USER_REGISTER, LOGIN, long_password) \
           == [Response.BAD_REQUEST, "Too big request."]


def test__user_login__invalid_login(client):
    client.query(Request.USER_REGISTER, LOGIN, PASSWORD, expect=Response.OK)
    client.query(Request.USER_LOGIN, "unknown", PASSWORD, expect=Response.FORBIDDEN)


def test__user_login__duplicate(client):
    client.query(Request.USER_REGISTER, LOGIN, PASSWORD, expect=Response.OK)
    client.query(Request.USER_REGISTER, LOGIN, PASSWORD, expect=Response.FORBIDDEN)


def test__user_register_login__wrong_password(client):
    client.query(Request.USER_REGISTER, LOGIN, PASSWORD, expect=Response.OK)
    client.query(Request.USER_LOGIN, LOGIN, "wrong", expect=Response.FORBIDDEN)


def test__user_register_login_logout__success(client):
    client.query(Request.USER_REGISTER, LOGIN, PASSWORD, expect=Response.OK)
    secret = client.query(Request.USER_LOGIN, LOGIN, PASSWORD, expect=Response.OK)
    client.query(Request.USER_LOGOUT, secret, expect=Response.OK)
    client.query(Request.USER_LOGOUT, secret, expect=Response.NOT_FOUND)


def test__user_register_login__old_sessions_are_closed(client):
    client.query(Request.USER_REGISTER, LOGIN, PASSWORD, expect=Response.OK)

    secrets = []
    for i in range(4):
        secret = client.query(Request.USER_LOGIN, LOGIN, PASSWORD, expect=Response.OK)
        secrets.append(secret)

    for i in range(4):
        expected_response = Response.NOT_FOUND if i == 0 else Response.OK
        client.query(Request.USER_LOGOUT, secrets[i], expect=expected_response)


def test__user_register_login_delete__success(client):
    client.query(Request.USER_REGISTER, LOGIN, PASSWORD, expect=Response.OK)
    secret = client.query(Request.USER_LOGIN, LOGIN, PASSWORD, expect=Response.OK)
    client.query(Request.USER_DELETE, secret, expect=Response.OK)
    client.query(Request.USER_DELETE, secret, expect=Response.FORBIDDEN)
    client.query(Request.USER_LOGIN, LOGIN, PASSWORD, expect=Response.FORBIDDEN)
