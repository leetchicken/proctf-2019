create table levels
(
    id        serial primary key,
    author_id integer references users (id),
    title     varchar(100),
    height    integer,
    width     integer,
    map       varchar(2500)
)