package BB::Controller::Main;
use Mojo::Base 'Mojolicious::Controller';

sub index {
  my $c = shift;

  my $user_id = $c->session('user_id');
  my $own = $c->param('own');

  my ($count, $max) = $c->boards->size();
  my $page = int($c->param('page') // 1);
  $page = 1 if $page < 1;
  $page = $max if $page > $max;

  my $boards = $c->boards->list($own ? $user_id : undef, $page);

  if ($user_id) {
    for my $board (@$boards) {
      if ($board->{user_id} == $user_id) {
        $board->{threads} = $c->boards->list_threads($board->{id});
      }
    }
  }

  $c->render(boards => $boards, page => $page, max => $max);
}

sub do_login {
  my $c = shift;

  my $login = $c->param('login');
  my $password = $c->param('password');

  my $r = $c->users->check_password($login, $password);

  if ($r->{status} eq 'ok') {
    $c->session->{login} = $login;
    $c->session->{user_id} = $r->{user_id};

    $c->redirect_to('index');
  } else {
    $c->flash(message => $r->{message});
    $c->redirect_to('login');
  }
}

sub do_register {
  my $c = shift;

  my $login = $c->param('login');
  my $password = $c->param('password');

  my $r = $c->users->create($login, $password);

  if ($r->{status} eq 'ok') {
    $c->session->{login} = $login;
    $c->session->{user_id} = $r->{user_id};

    $c->redirect_to('index');
  } else {
    $c->flash(message => $r->{message});
    $c->redirect_to('register');
  }
}

sub logout {
  my $c = shift;

  $c->session(expires => 1);
  $c->redirect_to('index');
}

sub do_ad_create {
  my $c = shift;

  return $c->redirect_to('index') unless my $login = $c->session('login');

  my $user_id = $c->session('user_id');

  my $title = $c->param('title');
  my $descr = $c->param('descr');

  my $board_id = $c->boards->create($user_id, $title, $descr);
  $c->cookie(last_created_board_id => $board_id);

  $c->redirect_to('index');
}

sub messages {
  my $c = shift;

  return $c->redirect_to('index') unless my $login = $c->session('login');

  my $user_id   = $c->session('user_id');
  my $board_id  = $c->param('board_id');
  my $client_id = $c->param('client_id');

  my $thread_id = $c->boards->ensure_thread($board_id, $client_id // $user_id);
  my $messages = $c->boards->get_messages($thread_id);

  $c->render(messages => $messages, client_id => $client_id);
}

sub write_message {
  my $c = shift;

  return $c->redirect_to('index') unless my $login = $c->session('login');

  my $user_id   = $c->session('user_id');
  my $board_id  = $c->param('board_id');
  my $client_id = $c->param('client_id');

  my $message = $c->param('message');

  my $thread_id = $c->boards->ensure_thread($board_id, $client_id // $user_id);
  $c->boards->create_message($thread_id, $client_id ? 'to_client' : 'to_owner', $message);

  $c->redirect_to($c->url_for('messages', board_id => $board_id)->query(client_id => $client_id));
}

1;
