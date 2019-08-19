package BB;
use Mojo::Base 'Mojolicious';

use Mojo::Pg;

use BB::Model::Users;
use BB::Model::Boards;

sub startup {
  my $app = shift;

  my $config = $app->plugin('Config');

  $app->secrets($config->{secrets});

  $app->helper(pg => sub { state $pg = Mojo::Pg->new(shift->config('pg')) });
  $app->helper(users => sub { state $users = Blog::Model::Users->new(pg => shift->pg) });
  $app->helper(boards => sub { state $boards = Blog::Model::Boards->new(pg => shift->pg, enc_key => $config->{enc_key}) });

  my $schema = $app->home->child('bb.sql');
  $app->pg->auto_migrate(1)->migrations->name('bb')->from_file($schema);

  my $r = $app->routes;

  $r->get('/')->to('main#index')->name('index');
  $r->get('/login')->to('main#login')->name('login');
  $r->post('/login')->to('main#do_login');
  $r->get('/logout')->to('main#logout')->name('logout');

  $r->get('/register')->to('main#register')->name('register');
  $r->post('/register')->to('main#do_register');

  $r->get('/ad/create')->to('main#ad_create')->name('ad_create');
  $r->post('/ad/create')->to('main#do_ad_create');

  $r->get('/messages/board/:board_id')->to('main#messages')->name('messages');
  $r->post('/messages/board/:board_id')->to('main#write_message');
}

1;
