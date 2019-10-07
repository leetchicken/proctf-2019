#!/usr/bin/perl

use Mojo::Base -strict;

use Crypt::JWT 'encode_jwt';
use Crypt::Misc 'encode_b64u';
use Mojo::Collection 'c';
use Mojo::File 'path';
use Mojo::Log;
use Mojo::UserAgent;

no warnings 'experimental::smartmatch';

my ($SERVICE_OK, $SERVICE_CORRUPT, $SERVICE_MUMBLE, $SERVICE_FAIL, $INTERNAL_ERROR) =
  (101, 102, 103, 104, 110);
my %MODES = (check => \&check, get => \&get, put => \&put);
my ($mode, $ip) = splice @ARGV, 0, 2;
my @chars = ('A' .. 'Z', 'a' .. 'z', '_', '0' .. '9');

warn 'Invalid input. Empty mode or ip address.' and exit $INTERNAL_ERROR unless defined $mode and defined $ip;
warn 'Invalid mode.' and exit $INTERNAL_ERROR unless $mode ~~ %MODES;

my $agents = c(<DATA>)->map(sub {chomp; $_});
my $ua = _ua();

my $url = Mojo::URL->new("https://$ip:8443/");
my $log = Mojo::Log->new;
$log->info("URL for check: $url");

exit $MODES{$mode}->(@ARGV);

sub _corrupt { say $_[0] and $log->info($_[0]) and exit $SERVICE_CORRUPT }
sub _mumble  { say $_[0] and $log->info($_[0]) and exit $SERVICE_MUMBLE }
sub _fail    { say $_[0] and $log->info($_[0]) and exit $SERVICE_FAIL }

sub _ua {
  my $ua = Mojo::UserAgent->new(max_redirects => 3, insecure => 1);
  $ua->transactor->name($agents->shuffle->first);

  return $ua;
}

sub _check_http_response {
  my $tx = shift;

  if (my $err = $tx->error) {
    if ($err->{code}) {
      _mumble "$err->{code} response: $err->{message}";
    } else {
      _fail "Connection error: $err->{message}";
    }
  } else {
    return $tx->result;
  }
}

sub check {
  return $SERVICE_OK;
}

sub get {
  my ($id, $flag) = @_;

  return $SERVICE_OK;
}

sub put {
  my ($id, $flag) = @_;

  $log->info("Check /acme route");
  $url->path('/acme');
  $_ = _check_http_response($ua->get($url));
  $log->info("Responce: " . $_->text);

  my $new_nonce_url = $_->json('/newNonce');
  _mumble "Invalid nonce url" unless $new_nonce_url =~ qr/^$url/;
  my $new_account_url = $_->json('/newAccount');
  _mumble "Invalid account url" unless $new_account_url =~ qr/^$url/;
  my $new_order_url = $_->json('/newOrder');
  _mumble "Invalid order url" unless $new_order_url =~ qr/^$url/;


  my $login;
  $login .= $chars[rand @chars] for 1 .. 12;
  path("./accounts/$ip/$login")->make_path;
  my $key = path("./accounts/$ip/$login/key.pem")->touch;

  $log->info("Generate PK for $login");
  qx/openssl genrsa -out $key 2048/;
  my $account = {contact => ["mailto:$login\@example.com"], termsOfServiceAgreed => \1, resource => 'new-reg'};
  my $pk = Crypt::PK::RSA->new("$key");

  my $nonce = encode_b64u('1234567890abcdef');

  my $data = encode_jwt(
    payload => $account,
    alg => 'RS256',
    key => $pk,
    serialization => 'flattened',
    extra_headers => {nonce => $nonce, url => $new_account_url, jwk => $pk->export_key_jwk(public => 1)}
  );

  $log->info("Create account for $login");
  $_ = _check_http_response($ua->post($new_account_url => $data));
  $log->info("Responce: " . $_->text);
  _mumble "Invalid contact" unless ($_->json('/contact/0') // '') eq "$login\@example.com";
  my $kid = $_->headers->header('Location') // '';
  _mumble "Invalid kid" unless $kid =~ qr/$url/;

  my $payload = {identifiers => [{type => 'dns', value => "$login.com"}]};
  $data = encode_jwt(
    payload => $payload,
    alg => 'RS256',
    key => $pk,
    serialization => 'flattened',
    extra_headers => {nonce => $nonce, url => $new_order_url, kid => $kid}
  );

  $log->info("Create order for $login");
  $_ = _check_http_response($ua->post($new_order_url => $data));
  $log->info("Responce: " . $_->text);
  _mumble "Invalid authorizations urls" unless ($_->json('/authorizations/0') // '') =~ qr/$url/;
  _mumble "Invalid finalize url" unless ($_->json('/finalize') // '') =~ qr/$url/;
  _mumble "Invalid identifiers" unless ($_->json('/identifiers/0/value') // '') eq "$login.com";
  _mumble "Invalid order status" unless ($_->json('/status') // '') eq 'pending';

  my $finalize_url = $_->json('/finalize');
  my $csr = path("./accounts/$ip/$login/csr.der");
  qx|openssl req -new -key $key -outform DER -subj /C=AE/CN=$login -out $csr|;

  $payload = {resource => 'new-cert', csr => encode_b64u($csr->slurp)};
  $data = encode_jwt(
    payload => $payload,
    alg => 'RS256',
    key => $pk,
    serialization => 'flattened',
    extra_headers => {nonce => $nonce, url => $finalize_url, kid => $kid}
  );

  $log->info("Create finalize request");
  $_ = _check_http_response($ua->post($finalize_url => $data));
  $log->info("Responce: " . $_->text);
  _mumble "Invalid authorizations urls" unless ($_->json('/authorizations/0') // '') =~ qr/$url/;
  _mumble "Invalid identifiers" unless ($_->json('/identifiers/0/value') // '') eq "$login.com";
  _mumble "Invalid authz status" unless ($_->json('/status') // '') eq 'valid';
  _mumble "Invalid cert url" unless ($_->json('/certificate') // '') =~ qr/$url/;

  my $cert_url = $_->json('/certificate');
  $data = encode_jwt(
    payload => '',
    alg => 'RS256',
    key => $pk,
    serialization => 'flattened',
    extra_headers => {nonce => $nonce, url => $cert_url, kid => $kid}
  );

  my $cert = path("./accounts/$ip/$login/cert.pem");
  $log->info("Get cert");
  $_ = _check_http_response($ua->post($cert_url => $data));
  $log->info("Responce: " . $_->text);
  $_->save_to($cert);

  $url->path('/');
  my $ua2 = _ua;
  $log->info("GET / with cert");
  $_ = _check_http_response($ua2->cert($cert)->key($key)->get($url));
  $log->info("Responce: " . $_->text);
  _mumble "Invalid email" unless ($_->json('/email') // '') eq "$login\@example.com";

  $log->info("POST / with cert");
  $_ = _check_http_response($ua2->cert($cert)->key($key)->post($url => json => {data => $flag}));
  $log->info("Responce: " . $_->text);

  say $login;
  return $SERVICE_OK;
}

__DATA__
Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Windows NT 10.0; WOW64; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Windows NT 6.1; WOW64; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12) AppleWebKit/602.1.50 (KHTML, like Gecko) Version/10.0 Safari/602.1.50
Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko
Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/602.2.14 (KHTML, like Gecko) Version/10.0.1 Safari/602.2.14
Mozilla/5.0 (Macintosh; Intel Mac OS X 10.11; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/602.1.50 (KHTML, like Gecko) Version/10.0 Safari/602.1.50
Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.79 Safari/537.36 Edge/14.14393
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36
Mozilla/5.0 (Windows NT 6.3; WOW64; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10.12; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (Windows NT 6.1; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36
Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36
Mozilla/5.0 (Windows NT 10.0; WOW64; Trident/7.0; rv:11.0) like Gecko
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.59 Safari/537.36
Mozilla/5.0 (X11; Linux x86_64; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (X11; Linux x86_64; rv:45.0) Gecko/20100101 Firefox/45.0
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Ubuntu Chromium/53.0.2785.143 Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/601.7.7 (KHTML, like Gecko) Version/9.1.2 Safari/601.7.7
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/601.7.8 (KHTML, like Gecko) Version/9.1.3 Safari/601.7.8
Mozilla/5.0 (iPad; CPU OS 10_0_2 like Mac OS X) AppleWebKit/602.1.50 (KHTML, like Gecko) Version/10.0 Mobile/14A456 Safari/602.1
Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36
Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_5) AppleWebKit/602.1.50 (KHTML, like Gecko) Version/10.0 Safari/602.1.50
Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.59 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10.10; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.0; Trident/5.0;  Trident/5.0)
Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0;  Trident/5.0)
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/602.2.14 (KHTML, like Gecko) Version/10.0.1 Safari/602.2.14
Mozilla/5.0 (Windows NT 6.1; Trident/7.0; rv:11.0) like Gecko
Mozilla/5.0 (Windows NT 6.1; WOW64; rv:45.0) Gecko/20100101 Firefox/45.0
Mozilla/5.0 (Windows NT 5.1; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2486.0 Safari/537.36 Edge/13.10586
Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.59 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.59 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/52.0.2743.116 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.59 Safari/537.36
Mozilla/5.0 (Windows NT 6.1; WOW64; rv:47.0) Gecko/20100101 Firefox/47.0
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Windows NT 6.3; WOW64; Trident/7.0; rv:11.0) like Gecko
Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36
Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.59 Safari/537.36
Mozilla/5.0 (Windows NT 6.1; WOW64; rv:48.0) Gecko/20100101 Firefox/48.0
Mozilla/5.0 (iPhone; CPU iPhone OS 10_0_2 like Mac OS X) AppleWebKit/601.1 (KHTML, like Gecko) CriOS/53.0.2785.109 Mobile/14A456 Safari/601.1.46
Mozilla/5.0 (iPhone; CPU iPhone OS 10_0_2 like Mac OS X) AppleWebKit/602.1.50 (KHTML, like Gecko) Version/10.0 Mobile/14A456 Safari/602.1
Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:47.0) Gecko/20100101 Firefox/47.0
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_5) AppleWebKit/601.6.17 (KHTML, like Gecko) Version/9.1.1 Safari/601.6.17
Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:48.0) Gecko/20100101 Firefox/48.0
Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/52.0.2743.116 Safari/537.36
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Ubuntu Chromium/52.0.2743.116 Chrome/52.0.2743.116 Safari/537.36
Mozilla/5.0 (Windows NT 10.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.71 Safari/537.36
Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36
Mozilla/5.0 (X11; Fedora; Linux x86_64; rv:49.0) Gecko/20100101 Firefox/49.0
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36
Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_4) AppleWebKit/601.5.17 (KHTML, like Gecko) Version/9.1 Safari/601.5.17
Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36
Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.59 Safari/537.36
