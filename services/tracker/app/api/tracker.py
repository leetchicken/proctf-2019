import logging

from app.common import handler, generate_token
from app.enums import Response, Request

MAX_TRACKERS = 8

log = logging.getLogger()


# FIXME: copy-paste?
async def auth(db, secret):
    row = await db.fetchrow("SELECT * FROM secret WHERE value=$1", secret)
    if row is None:
        return None
    return row["user_id"]


@handler(Request.TRACKER_LIST)
async def list(db, secret):
    user_id = await auth(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    trackers = await db.fetch("SELECT id, name FROM tracker WHERE user_id=$1", user_id)
    return Response.OK, [(t["id"], t["name"]) for t in trackers]


@handler(Request.TRACKER_ADD)
async def add(db, secret, name):
    user_id = await auth(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    row = await db.fetchrow("SELECT COUNT(*) AS count FROM tracker WHERE user_id=$1", user_id)
    if row["count"] >= MAX_TRACKERS:
        return Response.BAD_REQUEST, "Too many trackers."

    token = generate_token()
    await db.execute("INSERT INTO tracker(user_id, name, token) VALUES($1, $2, $3)",
                     user_id, name, token)
    log.debug("Added new tracker.")

    return Response.OK, token


@handler(Request.TRACKER_DELETE)
async def delete(db, secret, id):
    user_id = await auth(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    res = await db.execute("DELETE FROM tracker WHERE id=$1 AND user_id=$2", id, user_id)
    return Response.OK if res == "DELETE 1" else Response.NOT_FOUND
