use Mojolicious::Lite;

use Crypt::JWT 'decode_jwt';
use Crypt::Misc 'decode_b64u', 'encode_b64u';
use Data::UUID::MT;
use IPC::Run 'run';
use Mojo::JSON 'j';
use Mojo::SQLite;
use String::Random 'random_regex';

my $sql = Mojo::SQLite->new('sqlite:data/ca.db');
$sql->migrations->from_data->migrate;
my $db = $sql->db;

my $uuid = Data::UUID::MT->new();

get '/acme' => sub {
  my $c = shift;

  $c->render(json => {
    newNonce   => $c->url_for('nonce')->to_abs,
    newAccount => $c->url_for('create_account')->to_abs,
    newOrder   => $c->url_for('order')->to_abs
  });
};

get '/acme/nonce' => sub {
  my $c = shift;

  $c->_nonce;
  $c->rendered(200);
} => 'nonce';

post '/acme/account' => sub {
  my $c = shift;

  my ($header, $payload) = eval { decode_jwt(token => $c->req->text, key_from_jwk_header => 1, decode_header => 1) };
  $c->rendered(400) if $@;

  my $contact = $payload->{contact}[0];
  my $jwk = $header->{jwk};

  my $uuid = $uuid->create_string;
  my $kid  = $c->url_for('account', uuid => $uuid)->to_abs->to_string;
  $db->insert(accounts => {id => $uuid, contact => $contact, jwk => {json => $jwk}, kid => $kid});

  $c->_nonce;
  $c->res->headers->header(Location => $c->url_for('account', uuid => $uuid)->to_abs);
  $c->render(json => {status => 'valid', contact => [$contact]});
  $c->rendered(201);
} => 'create_account';

get '/acme/account/:uuid' => sub { } => 'account';

post '/acme/order/:uuid' => sub {
  my $c = shift;

  my ($err, $acc) = $c->_validate;
  return $c->rendered(400) if $err;

  my $order = $db->select(orders => '*', {acc_id => $acc->{id}})->hash;
  return $c->rendered(400) unless $order;

  $c->_nonce;
  $c->render(json => {
    status      => 'valid',
    certificate => $c->url_for('cert', uuid => $order->{id})->to_abs
  });
} => 'order';

post '/acme/order' => sub {
  my $c = shift;

  my ($err, $acc, $payload) = $c->_validate(1);
  return $c->rendered(400) if $err;

  my $identifiers = $payload->{identifiers};
  my $domain = $identifiers->[0]{value};

  my $order_id = $uuid->create_string;
  my $authz_id = $uuid->create_string;

  my $authz_value = encode_b64u '1234567890abcdef';
  my $order = {id => $order_id, acc_id => $acc->{id}, domain => $domain, authz_id => $authz_id, authz_value => $authz_value};
  $db->insert(orders => $order);

  $c->_nonce;
  $c->res->headers->header('Location' => $c->url_for('order', uuid => $order_id)->to_abs);
  $c->render(json => {
    status         => 'pending',
    identifiers    => [{type => 'dns', value => $domain}],
    authorizations => [$c->url_for('authz', uuid => $authz_id)->to_abs],
    finalize       => $c->url_for('authz_finalize', uuid => $authz_id)->to_abs
  });
  $c->rendered(201);
} => 'order';

post '/acme/authz/:uuid' => sub {
  my $c = shift;

  my ($err, $acc) = $c->_validate;
  return $c->rendered(400) if $err;

  my $authz_id = $c->param('uuid');
  my $order = $db->select(orders => '*', {authz_id => $authz_id})->hash;

  $c->_nonce;
  $c->render(json => {
    status      => 'valid',
    identifier => {type => 'dns', value => $order->{domain}},
    challenges  => [{
      type  => 'http-01',
      url   => $c->url_for('chall', uuid => 'test')->to_abs,
      token => $order->{authz_value}
    }]
  });
} => 'authz';

post '/acme/authz/:uuid/finalize' => sub {
  my $c = shift;

  my ($err, $acc, $payload) = $c->_validate(1);
  return $c->rendered(400) if $err;

  my $authz_id = $c->param('uuid');
  my $order = $db->select(orders => '*', {authz_id => $authz_id})->hash;
  return $c->rendered(400) unless $order;

  # Convert DER to PEM
  my $csr = decode_b64u $payload->{csr};
  my @cmd = split ' ', 'openssl req -inform DER';
  my ($stdout, $stderr) = ('', '');
  run \@cmd, \$csr, \$stdout, \$stderr;

  # Sign
  $csr = $stdout;
  @cmd = split ' ', 'openssl x509 -req -CA ssl/cert.pem -CAkey ssl/key.pem -CAcreateserial';
  run \@cmd, \$csr, \$stdout, \$stderr;

  $db->insert(certs => {order_id => $order->{id}, data => $stdout});

  $c->_nonce;
  $c->render(json => {
    status         => 'valid',
    identifiers    => [{type => 'dns', value => $order->{domain}}],
    authorizations => [$c->url_for('authz', uuid => $authz_id)->to_abs],
    finalize       => $c->url_for('authz_finalize', uuid => $authz_id)->to_abs,
    certificate    => $c->url_for('cert', uuid => $order->{id})->to_abs
  });
} => 'authz_finalize';

post '/acme/cert/:uuid' => sub {
  my $c = shift;

  my ($err, $acc) = $c->_validate;
  return $c->rendered(400) if $err;

  my $order_id = $c->param('uuid');
  my $cert = $db->select(certs => '*' => {order_id => $order_id})->hash;
  return $c->rendered(400) unless $cert;

  $c->_nonce;
  my $asset = Mojo::Asset::Memory->new;
  $asset->add_chunk($cert->{data});
  $c->reply->asset($asset);
} => 'cert';

post '/acme/chall/:uuid' => sub { } => 'chall';

app->helper(_validate => sub {
  my ($c, $p) = @_;

  my $kid = j(decode_b64u($c->req->json('/protected')))->{kid};
  return 1 unless $kid;

  my $acc = $db->select(accounts => ['id', 'jwk'] => {kid => $kid})->expand(json => 'jwk')->hash;
  return 1 unless $acc;

  my @return = (0, $acc);

  if ($p) {
    my $payload = eval { decode_jwt(token => $c->req->text, key => $acc->{jwk}) };
    return 1 if $@;

    push @return, $payload;
  }

  return @return;
});

app->helper(_nonce => sub {
  shift->res->headers->header('Replay-Nonce' => encode_b64u(random_regex('[a-z0-9]{16}')));
});

app->start;

__DATA__
@@ migrations

-- 1 up
create table accounts (
  id      text primary key,
  contact text not null,
  jwk     json not null,
  kid     text not null
);

create table orders (
  id          text primary key,
  acc_id      text not null references accounts(id),
  domain      text not null,
  authz_id    text,
  authz_value text
);

create table certs (
  id       integer primary key autoincrement,
  order_id text not null references orders(id),
  data     text
);
