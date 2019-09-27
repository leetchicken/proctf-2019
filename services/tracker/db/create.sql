CREATE TABLE "user" (
    id          UUID PRIMARY KEY,
    hash        VARCHAR NOT NULL,
    timestamp   INTEGER NOT NULL
);

CREATE TABLE secret (
    value       character varying(34) PRIMARY KEY,
    user_id     uuid NOT NULL,
    timestamp   integer NOT NULL,
    "order"     serial NOT NULL
);
CREATE INDEX ix_secret_user_id ON secret USING btree (timestamp);

CREATE TABLE tracker (
    id          serial PRIMARY KEY,
    user_id     uuid NOT NULL,
    name        character varying(16) NOT NULL,
    token       character varying(18) NOT NULL
);
CREATE INDEX ix_tracker_token ON tracker USING btree (token);
CREATE INDEX ix_tracker_user_id ON tracker USING btree (user_id);

CREATE TABLE point (
    id          serial PRIMARY KEY,
    latitude    double precision NOT NULL,
    longitude   double precision NOT NULL,
    tracker_id  integer NOT NULL,
    track_id    integer NOT NULL,
    meta        character varying(1),
    timestamp   integer NOT NULL
);
CREATE INDEX ix_point_timestamp ON point USING btree (timestamp);
CREATE INDEX ix_point_track_id ON point USING btree (track_id);
CREATE INDEX ix_point_tracker_id ON point USING btree (tracker_id);

CREATE TABLE track (
    id          serial PRIMARY KEY,
    user_id     uuid NOT NULL,
    started_at  integer NOT NULL,
    access      integer NOT NULL
);
CREATE INDEX ix_track_user_id ON track USING btree (user_id);
CREATE TABLE zone (
    id          serial PRIMARY KEY,
    user_id     uuid NOT NULL,
    latitude    double precision NOT NULL,
    longitude   double precision NOT NULL,
    radius      double precision NOT NULL
);
CREATE INDEX ix_zone_user_id ON zone USING btree (user_id);
