import asyncio
import logging

from app.common import auth_user
from app.enums import Response, TrackAccess


async def track_list(db, secret):
    user_id = await auth_user(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    rows = await db.fetch("SELECT * FROM track WHERE user_id=$1", user_id)
    tracks = [(r["id"], r["started_at"], r["access"]) for r in rows]

    return Response.OK, tracks


async def track_get(db, secret, track_id):
    user_id = await auth_user(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    await asyncio.sleep(1)

    row = await db.fetchrow("SELECT * FROM track WHERE id=$1", track_id)
    if row is None:
        return Response.NOT_FOUND

    access = row["access"]
    if access is TrackAccess.PRIVATE or access is TrackAccess.PENDING:
        if user_id != row["user_id"]:
            logging.warning("Get track is forbidden for user %d (owner: %d)", user_id, row["user_id"])
            return Response.FORBIDDEN

    if TrackAccess.GROUP_ACCESS_MIN <= access <= TrackAccess.GROUP_ACCESS_MAX:
        # TODO: check access for user_group
        return Response.FORBIDDEN

    rows = await db.fetch("SELECT timestamp, latitude, longitude, meta FROM point "
                          "WHERE track_id=$1 ORDER BY id", track_id)
    points = [(r["timestamp"], r["latitude"], r["longitude"], r["meta"]) for r in rows]
    return Response.OK, points


async def track_delete(db, secret, track_id):
    user_id = await auth_user(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    row = await db.fetchrow("SELECT * FROM track WHERE id=$1 AND user_id=$2", track_id, user_id)
    if row is None:
        return Response.NOT_FOUND

    await db.execute("DELETE FROM track WHERE id=$1 AND user_id=$2", track_id, user_id)
    await db.execute("DELETE FROM point WHERE track_id=$1", track_id)

    return Response.OK


async def track_request_share(db, secret, track_id):
    user_id = await auth_user(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    row = await db.fetchrow("SELECT * FROM track WHERE id=$1", track_id)
    if row is None:
        return Response.NOT_FOUND

    if row["access"] is not TrackAccess.PRIVATE:
        return Response.FORBIDDEN

    await db.fetchrow("UPDATE track SET access=$1 WHERE id=$2", TrackAccess.PENDING, track_id)
    return Response.OK, str(row["user_id"])


async def track_share(db, secret, track_id):
    user_id = await auth_user(db, secret)
    if user_id is None:
        return Response.FORBIDDEN

    row = await db.fetchrow("SELECT * FROM track WHERE id=$1 AND user_id=$2", track_id, user_id)
    if row is None:
        return Response.NOT_FOUND

    if row["access"] is TrackAccess.PENDING:
        await db.fetchrow("UPDATE track SET access=$1 WHERE id=$2", TrackAccess.PUBLIC, track_id)

    return Response.OK
