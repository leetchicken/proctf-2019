create table users
(
    id            serial primary key,
    login         varchar(100) unique,
    password_hash varchar(60),
    name          varchar(100)
)