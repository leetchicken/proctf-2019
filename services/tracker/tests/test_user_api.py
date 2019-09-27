from tests.common import *
from app.enums import Request, Response


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
    assert client.query(Request.USER_REGISTER, "123") == [Response.BAD_REQUEST, "Password is too short."]


def test__user_register__long_password(client):
    long_password = "x" * 65
    assert client.query(Request.USER_REGISTER, long_password) == [Response.BAD_REQUEST, "Password is too long."]


def test__user_register__big_request(client):
    long_password = "x" * 1024
    assert client.query(Request.USER_REGISTER, long_password) == [Response.BAD_REQUEST, "Too big request."]


def test__user_login__invalid_user_id(client):
    client.query(Request.USER_LOGIN, "not_uuid", "wrong", expect=Response.BAD_REQUEST)


def test__user_register_login__wrong_password(client):
    user_id = client.query(Request.USER_REGISTER, PASSWORD, expect=Response.OK)
    client.query(Request.USER_LOGIN, user_id, "wrong", expect=Response.FORBIDDEN)


def test__user_register_login_logout__success(client):
    user_id = client.query(Request.USER_REGISTER, PASSWORD, expect=Response.OK)
    secret = client.query(Request.USER_LOGIN, user_id, PASSWORD, expect=Response.OK)
    client.query(Request.USER_LOGOUT, secret, expect=Response.OK)
    client.query(Request.USER_LOGOUT, secret, expect=Response.NOT_FOUND)


def test__user_register_login__old_sessions_are_closed(client):
    user_id = client.query(Request.USER_REGISTER, PASSWORD, expect=Response.OK)

    secrets = []
    for i in range(4):
        secret = client.query(Request.USER_LOGIN, user_id, PASSWORD, expect=Response.OK)
        secrets.append(secret)

    for i in range(4):
        expected_response = Response.NOT_FOUND if i == 0 else Response.OK
        client.query(Request.USER_LOGOUT, secrets[i], expect=expected_response)
