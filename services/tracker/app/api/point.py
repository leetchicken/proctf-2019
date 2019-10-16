import logging
import time

from app.common import handler, auth_tracker
from app.enums import Response, Request, TrackAccess

NEW_TRACK_GAP_SECONDS = 2


async def get_or_create_track_id(db, user_id, tracker_id, now):
    prev = await db.fetchrow("SELECT track_id FROM point WHERE tracker_id=$1 AND timestamp >= $2 "
                             "ORDER BY timestamp DESC LIMIT 1", tracker_id, now - NEW_TRACK_GAP_SECONDS)
    if prev is None:
        track = await db.fetchrow("INSERT INTO track(user_id, started_at, access) VALUES($1, $2, $3) RETURNING id",
                                  user_id, now, TrackAccess.PRIVATE)
        track_id = track["id"]
        logging.debug("Created new track: %d.", track_id)
    else:
        track_id = prev["track_id"]
        logging.debug("Using old track: %d.", track_id)

    return track_id


@handler(Request.POINT_ADD)
async def point_add(db, token, latitude, longitude, meta):
    try:
        latitude = float(latitude)
        longitude = float(longitude)
    except ValueError:
        return Response.BAD_REQUEST

    tracker_id, user_id = await auth_tracker(db, token)
    if tracker_id is None:
        return Response.FORBIDDEN

    now = time.time()
    track_id = await get_or_create_track_id(db, user_id, tracker_id, now)

    await db.execute("INSERT INTO point(latitude, longitude, tracker_id, track_id, meta, timestamp) "
                     "VALUES($1, $2, $3, $4, $5, $6)",
                     latitude, longitude, tracker_id, track_id, meta, time.time())
    logging.debug("Added new point to track %d.", track_id)

    return Response.OK, track_id


@handler(Request.POINT_ADD_BATCH)
async def point_add_batch(db, token, points):
    tracker_id, user_id = await auth_tracker(db, token)
    if tracker_id is None:
        return Response.FORBIDDEN

    now = time.time()
    track_id = await get_or_create_track_id(db, user_id, tracker_id, now)

    columns = ["latitude", "longitude", "tracker_id", "track_id", "meta", "timestamp"]
    records = [(lat, lon, tracker_id, track_id, meta, now) for lat, lon, meta in points]

    await db.copy_records_to_table("point", columns=columns, records=records)

    logging.debug("Added %d point(s) to track %s.", len(points), track_id)

    return Response.OK, track_id
