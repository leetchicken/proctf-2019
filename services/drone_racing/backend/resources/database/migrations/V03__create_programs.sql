create table programs
(
    id        serial primary key,
    author_id integer references users (id),
    level_id  integer references levels (id),
    title     varchar(100),
    file      varchar(100)
)