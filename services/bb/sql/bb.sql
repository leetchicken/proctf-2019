-- 1 up
create table users (
  id         serial primary key,
  login      varchar(50) not null unique,
  pwhash     text not null,
  info       jsonb,
  created_at timestamptz not null default now()
);

create table boards (
  id         serial primary key,
  user_id    int not null references users(id),
  title      varchar(50) not null,
  descr      text,
  created_at timestamptz not null default now()
);

create table threads (
  id         serial primary key,
  board_id   int not null references boards(id),
  client_id  int not null references users(id),
  created_at timestamptz not null default now(),
  unique (board_id, client_id)
);

create table messages (
  id         serial primary key,
  thread_id  int references threads(id),
  direction  text check (direction = 'to_client' or direction = 'to_owner'),
  data       bytea,
  created_at timestamptz not null default now()
);
