package Blog::Model::Users;
use Mojo::Base -base;

has 'pg';

sub create {
  my ($self, $login, $password) = @_;

  my $r = $self->pg->db->query(q{
    insert into users (login, pwhash, info)
    values ($1, crypt($2, gen_salt('md5')), '{}')
    on conflict (login) do nothing
    returning id
  }, $login, $password);

  if ($r->rows) {
    return {status => 'ok', message => 'user created', user_id => $r->hash->{id}};
  } else {
    return {status => 'fail', message => 'login already used'};
  }
}

sub check_password {
  my ($self, $login, $password) = @_;

  my $r = $self->pg->db->query(q{
    select (pwhash = crypt($2, pwhash)) as auth, id
    from users
    where login = $1
  }, $login, $password);

  if ($r->rows) {
    my $row = $r->hash;
    if ($row->{auth}) {
      return {status => 'ok', message => 'user authentificated', user_id => $row->{id}};
    } else {
      return {status => 'fail', message => 'password invalid'};
    }
  } else {
    return {status => 'fail', message => 'user not found'};
  }
}

1;
