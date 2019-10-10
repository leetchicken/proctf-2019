use Mojolicious::Lite;

use Crypt::JWT 'decode_jwt';
use Crypt::Misc 'decode_b64u', 'encode_b64u';
use Data::UUID::MT;
use IO::Socket::SSL::Utils 'CERT_asHash';
use List::Util 'pairmap';
use Mojo::File 'tempfile', 'path';
use Mojo::JSON 'j';
use Mojo::SQLite;
use String::Random 'random_regex';

my $DOMAIN_RE = qr/\S+(.\S+){1,3}/;

my $sql = Mojo::SQLite->new('sqlite:data/ca.db');
$sql->migrations->from_data->migrate;
my $db = $sql->db;

my $uuid = Data::UUID::MT->new;

get '/' => sub {
  my $c = shift;

  my ($err, $email) = $c->_tls;
  return $c->render(json => {error => \1, message => 'only transactions with certificate allowed'}) if $err;

  my $acc = $db->select(accounts => '*', {contact => $email})->hash;
  return $c->render(json => {error => \1, message => 'invalid email'}) unless $acc;

  my $data = $db->select(data => 'data', {acc_id => $acc->{id}})->hashes->to_array;
  $c->render(json => {error => \0, email => $email, data => $data});
};

post '/' => sub {
  my $c = shift;

  my ($err, $email) = $c->_tls;
  return $c->render(json => {error => \1, message => 'only transactions with certificate allowed'}) if $err;

  my $acc = $db->select(accounts => '*', {contact => $email})->hash;
  return $c->render(json => {error => \1, message => 'invalid email'}) unless $acc;

  my $data = $c->req->json('/data');

  $db->insert(data => {acc_id => $acc->{id}, data => $data});

  $c->render(json => {error => \0});
};

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
  return $c->render(json => {type => 'malformed', detail => 'invalid jws'}) && $c->rendered(403) if $@;

  my $contact = $payload->{contact}[0];
  $contact =~ s/^mailto://;
  return $c->render(json => {type => 'malformed', detail => 'invalid contact'}) && $c->rendered(403)
    unless $contact =~ /^\w+\@$DOMAIN_RE$/;

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
  return $c->render(json => {type => 'malformed', detail => 'invalid account or jws'}) && $c->rendered(403) if $err;

  my $order = $db->select(orders => '*', {acc_id => $acc->{id}})->hash;
  return $c->render(json => {type => 'malformed', detail => 'invalid order UUID'}) && $c->rendered(403) unless $order;

  $c->_nonce;
  $c->render(json => {
    status      => 'valid',
    certificate => $c->url_for('cert', uuid => $order->{id})->to_abs
  });
} => 'order';

post '/acme/order' => sub {
  my $c = shift;

  my ($err, $acc, $payload) = $c->_validate;
  return $c->render(json => {type => 'malformed', detail => 'invalid account or jws'}) && $c->rendered(403) if $err;

  my $identifiers = $payload->{identifiers};
  my $domain = $identifiers->[0]{value};
  return $c->render(json => {type => 'malformed', detail => 'invalid domain'}) && $c->rendered(403)
    unless $domain =~ /^$DOMAIN_RE$/;

  my $order_id = $uuid->create_string;
  my $authz_id = $uuid->create_string;

  my $order = {
    id          => $order_id,
    acc_id      => $acc->{id},
    domain      => $domain,
    authz_id    => $authz_id,
    authz_value => encode_b64u('1234567890abcdef')
  };
  eval { $db->insert(orders => $order) };
  return $c->render(json => {type => 'malformed', detail => "$@"}) && $c->rendered(403) if $@;

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
  return $c->render(json => {type => 'malformed', detail => 'invalid account or jws'}) && $c->rendered(403) if $err;

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
  my $c = shift->render_later;

  my ($err, $acc, $payload) = $c->_validate;
  return $c->render(json => {type => 'malformed', detail => 'invalid account or jws'}) && $c->rendered(403) if $err;

  my $authz_id = $c->param('uuid');
  my $order = $db->select(orders => '*', {authz_id => $authz_id})->hash;
  return $c->render(json => {type => 'malformed', detail => 'invalid order UUID'}) && $c->rendered(403) unless $order;

  Mojo::IOLoop->subprocess(
    sub {
      alarm 5;

      # Convert DER to PEM
      my $csr = decode_b64u $payload->{csr};
      my $path = tempfile->spurt($csr);
      my $cmd = "openssl req -inform DER -in $path";
      $csr = qx/$cmd/;
      $path = tempfile->spurt($csr);

      # Sign
      my $subj = join '', pairmap { "/$a=$b" } (emailAddress => $acc->{contact}, countryName => 'AE', commonName => $order->{domain});
      my $opts = "-config ca.conf -batch -create_serial -rand_serial";
      $cmd = "openssl ca $opts -cert ssl/cert.pem -keyfile ssl/key.pem -subj $subj -in $path";
      my $crt = qx/$cmd/;
      return $c->render(json => {type => 'malformed', detail => 'invalid CSR'}) && $c->rendered(403) unless $crt;

      $db->insert(certs => {order_id => $order->{id}, data => $crt});

      return;
    },
    sub {
      my ($s, $err) = @_;

      return $c->render(json => {type => 'serverInternal', detail => $err}) && $c->rendered(403)
        if $err;

      $c->_nonce;
      $c->render(json => {
        status         => 'valid',
        identifiers    => [{type => 'dns', value => $order->{domain}}],
        authorizations => [$c->url_for('authz', uuid => $authz_id)->to_abs],
        finalize       => $c->url_for('authz_finalize', uuid => $authz_id)->to_abs,
        certificate    => $c->url_for('cert', uuid => $order->{id})->to_abs
      });
    }
  );
} => 'authz_finalize';

post '/acme/cert/:uuid' => sub {
  my $c = shift;

  my ($err, $acc) = $c->_validate;
  return $c->render(json => {type => 'malformed', detail => 'invalid account or jws'}) && $c->rendered(403) if $err;

  my $order_id = $c->param('uuid');
  my $cert = $db->select(certs => '*' => {order_id => $order_id})->hash;
  return $c->render(json => {type => 'malformed', detail => 'invalid cert UUID'}) && $c->rendered(403) unless $cert;

  $c->_nonce;
  my $asset = Mojo::Asset::Memory->new;
  $asset->add_chunk($cert->{data});
  $c->reply->asset($asset);
} => 'cert';

post '/acme/chall/:uuid' => sub { } => 'chall';

app->helper(_validate => sub {
  my $c = shift;

  my $kid = j(decode_b64u($c->req->json('/protected')))->{kid};
  return 1 unless $kid;

  my $acc = $db->select(accounts => ['id', 'jwk', 'contact'] => {kid => $kid})->expand(json => 'jwk')->hash;
  return 1 unless $acc;

  my $payload = eval { decode_jwt(token => $c->req->text, key => $acc->{jwk}) };
  return 1 if $@;

  return 0, $acc, $payload;
});

app->helper(_tls => sub {
  my $c = shift;

  my $id = $c->tx->connection;
  my $handle = Mojo::IOLoop->stream($id)->handle;

  my @certs = $handle->peer_certificates;
  return 1 unless @certs;

  my $cert = CERT_asHash($certs[0]);
  my $email = $cert->{subject}{emailAddress};
  return 0, $email;
});

app->helper(_nonce => sub {
  shift->res->headers->header('Replay-Nonce' => encode_b64u(random_regex('[a-z0-9]{16}')));
});

my $ca_dir = path('./data/ca')->make_path;
$ca_dir->child('crt')->make_path;
$ca_dir->child('index.txt')->touch;
$ca_dir->child('serial')->touch;

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
  domain      varchar(32) not null unique,
  authz_id    text,
  authz_value text
);

create table certs (
  id       integer primary key autoincrement,
  order_id text not null references orders(id),
  data     varchar(16384)
);

create table data (
  id     integer primary key autoincrement,
  acc_id text not null references accounts(id),
  data   varchar(128) not null
);
