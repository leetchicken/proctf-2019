import logging

from app.common import handler
from app.enums import Response, Request, TrackAccess

log = logging.getLogger()


# FIXME: copy-paste?
async def auth(db, secret):
    row = await db.fetchrow("SELECT * FROM secret WHERE value=$1", secret)
    if row is None:
        return None
    return row["user_id"]


@handler(Request.TRACK_LIST)
async def track_list(db, secret):
    user_id = await auth(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    rows = await db.fetch("SELECT * FROM track WHERE user_id=$1", user_id)
    tracks = [(r["id"], r["started_at"], r["access"]) for r in rows]

    return Response.OK, tracks


@handler(Request.TRACK_GET)
async def track_get(db, secret, track_id):
    user_id = await auth(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    row = await db.fetchrow("SELECT * FROM track WHERE id=$1", track_id)
    if row is None:
        return Response.NOT_FOUND

    access = row["access"]
    if access is TrackAccess.PRIVATE or access is TrackAccess.PENDING:
        if user_id != row["user_id"]:
            log.warning("Get track is forbidden for user %d (owner: %d)", user_id, row["user_id"])
            return Response.FORBIDDEN

    if TrackAccess.GROUP_ACCESS_MIN <= access <= TrackAccess.GROUP_ACCESS_MAX:
        # FIXME: check access == user_group
        return Response.FORBIDDEN

    rows = await db.fetch("SELECT timestamp, latitude, longitude, meta FROM point "
                          "WHERE track_id=$1 ORDER BY id", track_id)
    points = [(r["timestamp"], r["latitude"], r["longitude"], r["meta"]) for r in rows]
    return Response.OK, points


@handler(Request.TRACK_DELETE)
async def track_delete(db, secret, track_id):
    user_id = await auth(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    row = await db.fetchrow("SELECT * FROM track WHERE id=$1 AND user_id=$2", track_id, user_id)
    if row is None:
        return Response.NOT_FOUND

    await db.execute("DELETE FROM track WHERE id=$1 AND user_id=$2", track_id, user_id)
    await db.execute("DELETE FROM point WHERE track_id=$1", track_id)

    return Response.OK


@handler(Request.TRACK_REQUEST_SHARE)
async def track_request_share(db, secret, track_id):
    user_id = await auth(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    row = await db.fetchrow("SELECT * FROM track WHERE id=$1", track_id)
    if row is None:
        return Response.NOT_FOUND

    if row["access"] is not TrackAccess.PRIVATE:
        return Response.FORBIDDEN

    await db.fetchrow("UPDATE track SET access=$1 WHERE id=$2", TrackAccess.PENDING, track_id)
    return Response.OK


@handler(Request.TRACK_SHARE)
async def track_share(db, secret, track_id):
    user_id = await auth(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    row = await db.fetchrow("SELECT * FROM track WHERE id=$1 AND user_id=$2", track_id, user_id)
    if row is None:
        return Response.NOT_FOUND

    if row["access"] is not TrackAccess.PENDING:
        return Response.BAD_REQUEST

    await db.fetchrow("UPDATE track SET access=$1 WHERE id=$2", TrackAccess.PUBLIC, track_id)
    return Response.OK
