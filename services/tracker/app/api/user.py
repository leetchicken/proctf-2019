import logging
import time
import uuid

from app.common import hash_password, is_uuid, generate_secret, connect_db, handler
from app.enums import Response, Request

MIN_PASSWORD_LEN = 8
MAX_PASSWORD_LEN = 64
MAX_ACTIVE_SESSIONS = 3

log = logging.getLogger()


@handler(Request.USER_REGISTER)
async def register(db, password):
    if len(password) < MIN_PASSWORD_LEN:
        return Response.BAD_REQUEST, "Password is too short."
    if len(password) > MAX_PASSWORD_LEN:
        return Response.BAD_REQUEST, "Password is too long."

    # FIXME: check proof of work

    user_id = uuid.uuid4()
    await db.execute('INSERT INTO "user"(id, hash, timestamp) VALUES($1, $2, $3)',
                     user_id, hash_password(password), time.time())

    return Response.OK, str(user_id)


@handler(Request.USER_LOGIN)
async def login(db, user_id, password):
    if not is_uuid(user_id):
        return Response.BAD_REQUEST

    user = await db.fetchrow('SELECT * FROM "user" WHERE id=$1 AND hash=$2',
                             user_id, hash_password(password))
    if user is None:
        return Response.FORBIDDEN

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
async def logout(db, secret):
    res = await db.execute('DELETE FROM secret WHERE value=$1', secret)
    return Response.OK if res == "DELETE 1" else Response.NOT_FOUND
