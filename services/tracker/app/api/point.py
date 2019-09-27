import logging
import time

from app.common import handler
from app.enums import Response, Request, TrackAccess

log = logging.getLogger()

NEW_TRACK_GAP_SECONDS = 2


# FIXME: copy-paste?
async def auth(db, token):
    row = await db.fetchrow("SELECT id, user_id FROM tracker WHERE token=$1", token)
    if row is None:
        return None, None
    return row["id"], row["user_id"]


@handler(Request.POINT_ADD)
async def add(db, token, latitude, longitude, meta):
    try:
        latitude = float(latitude)
        longitude = float(longitude)
    except ValueError:
        return Response.BAD_REQUEST

    tracker_id, user_id = await auth(db, token)
    if tracker_id is None:
        return Response.FORBIDDEN

    now = time.time()

    prev = await db.fetchrow("SELECT track_id FROM point WHERE tracker_id=$1 AND timestamp >= $2 "
                             "ORDER BY timestamp DESC LIMIT 1", tracker_id, now - NEW_TRACK_GAP_SECONDS)
    if prev is None:
        track = await db.fetchrow("INSERT INTO track(user_id, started_at, access) VALUES($1, $2, $3) RETURNING id",
                                  user_id, now, TrackAccess.PRIVATE)
        track_id = track["id"]
        log.debug("Created new track: %d.", track_id)
    else:
        track_id = prev["track_id"]
        log.debug("Using old track: %d.", track_id)

    await db.execute("INSERT INTO point(latitude, longitude, tracker_id, track_id, meta, timestamp) "
                     "VALUES($1, $2, $3, $4, $5, $6)",
                     latitude, longitude, tracker_id, track_id, meta, time.time())
    log.debug("Added new point.")

    return Response.OK, track_id
