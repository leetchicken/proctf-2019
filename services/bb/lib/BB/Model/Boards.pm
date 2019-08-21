package Blog::Model::Boards;
use Mojo::Base -base;

has 'pg';
has 'enc_key';

has page_size => 15;

sub create {
  my ($self, $user_id, $title, $descr) = @_;

  $self->pg->db->insert(
    boards =>
    {user_id => $user_id, title => $title, descr => $descr},
    {returning => 'id'}
  )->hash->{id};
}

sub size {
  my ($self) = @_;

  my $count = $self->pg->db->select('boards', 'count(*)')->hash->{count};
  my $max = int($count / $self->page_size) + 1;
  return ($count, $max);
}

sub list {
  my ($self, $user_id, $page) = @_;
  $page //= 1;
  my $offset = ($page - 1) * $self->page_size;

  my $where = $user_id ? {user_id => $user_id} : {};
  my $r = $self->pg->db->select(
    ['boards', ['users', id => 'user_id']],
    ['boards.id', 'boards.title', 'boards.descr', 'boards.user_id', 'boards.created_at', 'users.login'],
    $where,
    {limit => $self->page_size, offset => $offset, order_by => {-desc => 'boards.id'}}
  );

  return $r->hashes;
}

sub get {
  my ($self, $board_id) = @_;

  return $self->pg->db->select('boards', '*', {id => $board_id})->hash;
}

sub list_threads {
  my ($self, $board_id) = @_;

  my $r = $self->pg->db->select(
    ['threads', ['users', id => 'client_id']],
    ['threads.id', 'threads.client_id', 'users.login'],
    {board_id => $board_id}
  );

  return $r->hashes;
}

sub ensure_thread {
  my ($self, $board_id, $client_id) = @_;

  my $r = $self->pg->db->insert(
    threads =>
    {board_id => $board_id, client_id => $client_id, created_at => \'now()'},
    {returning => 'id', on_conflict => undef}
  );

  return $r->hash->{id} if $r->rows;

  return $self->pg->db->select(
    threads => 'id' => {board_id => $board_id, client_id => $client_id}
  )->hash->{id};
}

sub get_messages {
  my ($self, $thread_id) = @_;

  my $decrypted_data = sprintf "(select pgp_sym_decrypt(data::bytea, '%s')) as data", $self->enc_key;

  my $r = $self->pg->db->select(
    messages => ['id', 'created_at', 'direction', \$decrypted_data] =>
    [thread_id => $thread_id],
    {order_by => {-asc => 'id'}}
  );

  return $r->hashes;
}

sub create_message {
  my ($self, $thread_id, $direction, $data) = @_;

  my $encrypted_data = sprintf "(select pgp_sym_encrypt('%s', '%s'))", $data, $self->enc_key;

  $self->pg->db->insert(
    messages =>
    {thread_id => $thread_id, direction => $direction, data => \$encrypted_data, created_at => \'now()'},
    {returning => 'id'}
  )->hash->{id};
}

1;
