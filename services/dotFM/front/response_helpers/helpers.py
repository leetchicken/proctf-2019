from sanic.response import json
from sanic.response import HTTPResponse


def __base_response(message: str, status: int) -> HTTPResponse:
    return json({"message": message}, status)


def bad_request(message: str) -> HTTPResponse:
    return __base_response(message, 400)


def ok(message: str) -> HTTPResponse:
    return __base_response(message, 200)


def conflict(message: str) -> HTTPResponse:
    return __base_response(message, 409)