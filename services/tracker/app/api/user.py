import logging
import time
import uuid

from app.common import hash_password, generate_secret, handler, auth_user
from app.enums import Response, Request

MIN_PASSWORD_LEN = 8
MAX_PASSWORD_LEN = 64
MAX_LOGIN_LEN = 16
MAX_ACTIVE_SESSIONS = 3

log = logging.getLogger()


@handler(Request.USER_REGISTER)
async def user_register(db, login, password):
    if len(password) < MIN_PASSWORD_LEN:
        return Response.BAD_REQUEST, "Password is too short."
    if len(password) > MAX_PASSWORD_LEN:
        return Response.BAD_REQUEST, "Password is too long."
    if len(login) > MAX_LOGIN_LEN:
        return Response.BAD_REQUEST, "Login is too long."

    user = await db.fetchrow('SELECT id FROM "user" WHERE login=$1', login)
    if user is not None:
        return Response.FORBIDDEN, "Such login already exists."

    # FIXME: check proof of work

    user_id = uuid.uuid4()
    await db.execute('INSERT INTO "user"(id, login, hash, timestamp) VALUES($1, $2, $3, $4)',
                     user_id, login, hash_password(password), time.time())

    return Response.OK


@handler(Request.USER_LOGIN)
async def user_login(db, login, password):
    user = await db.fetchrow('SELECT id FROM "user" WHERE login=$1 AND hash=$2',
                             login, hash_password(password))
    if user is None:
        return Response.FORBIDDEN
    user_id = user["id"]

    secrets = await db.fetch("SELECT * FROM secret WHERE user_id=$1", user_id)
    if len(secrets) >= MAX_ACTIVE_SESSIONS:
        log.warning("Too many active secrets (%d), will remove old.", len(secrets))
        secrets_to_delete = sorted(secrets, key=lambda s: s["order"], reverse=True)[MAX_ACTIVE_SESSIONS-1:]
        for secret in secrets_to_delete:
            log.debug("Removing secret %s", secret["value"])
            await db.execute("DELETE FROM secret WHERE value=$1", secret["value"])

    secret = generate_secret()
    await db.execute("INSERT INTO secret(value, user_id, timestamp) VALUES($1, $2, $3)",
                     secret, user_id, time.time())
    log.debug("Login successful.")
    return Response.OK, secret


@handler(Request.USER_LOGOUT)
async def user_logout(db, secret):
    res = await db.execute('DELETE FROM secret WHERE value=$1', secret)
    return Response.OK if res == "DELETE 1" else Response.NOT_FOUND


@handler(Request.USER_DELETE)
async def user_delete(db, secret):
    user_id = await auth_user(db, secret)
    if user_id is None:
        return Response.FORBIDDEN
    await db.execute('DELETE FROM track WHERE user_id=$1', user_id)
    await db.execute('DELETE FROM tracker WHERE user_id=$1', user_id)
    await db.execute('DELETE FROM "user" WHERE id=$1', user_id)
    await db.execute('DELETE FROM secret WHERE user_id=$1', user_id)
    return Response.OK
