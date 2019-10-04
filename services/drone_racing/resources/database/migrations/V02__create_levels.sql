create table levels
(
    id        serial primary key,
    author_id integer references users (id),
    title     varchar(100),
    map       varchar(2500)
)