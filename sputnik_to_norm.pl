#!/usr/bin/env perl
use strict;
use warnings;

use Digest::CRC qw(crcccitt crc16) ;

                 #  T 
                 #  I 
                 #  MI
                 #  EP
                 #  44
my $PKT_HDR_FMT  = "NN";
                 #  S F   R
                 #  IPLSSOE
                 #  ZRATEIS
                 #  EOGRQDV
                 #  1111442
my $PKT_DATA     = "CCCCNNn";

my $PKT_SIZE = 24;

my $fn = shift @ARGV;
my $fh;
open $fh, "<$fn" or die "Can't open '$fn' for reading: '$!'";

my $pkt;
while((read $fh, $pkt, $PKT_SIZE) == $PKT_SIZE) {
	my ($time, $ip) = unpack $PKT_HDR_FMT, (substr $pkt, 0, 8);
	my $data = substr $pkt, 8, 14;
	my $crc = substr $pkt, 22, 2;

	die if(crcccitt($data) != unpack "n", $crc);

    printf "%08X %016X 00000 ", $ip, $time;
    map { printf "%02X", ord $_ } split //, ($data.$crc);
	printf "\n";
}
