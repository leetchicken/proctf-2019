create table runs
(
    id          serial primary key,
    program_id  integer references programs (id),
    level_id    integer references levels (id),
    start_time  integer,
    finish_time integer,
    success     boolean,
    score       integer
)