#!/usr/bin/env perl
#
# This script converts sputnik-style binary log files into
# the normalized packet format used for testing the framework
#
# Those log files are binary dumps of network-endian packed
# structures, containing, in order:
# 	time (seconds, 4 bytes)
#	IPv4 address (4 bytes)
#   size (OpenAMD frame, 1 byte)
#		size - 3 bytes (OpenAMD packet data, fields in network endianness)
#   CRC16 (OpenAMD frame, really CRCCCITT, 2 bytes)
#
# This script duplicates framing logic but doesn't do a whole
# lot of checking before handing it off as a normalized line.
# It knows enough to do CRC checking if you want, but by default,
# to keep dependencies down, it's commented out

use strict;
use warnings;

# use Digest::CRC qw(crcccitt crc16) ;

                 #  T S 
                 #  I I
                 #  MIZ
                 #  EPE
                 #  441
my $PKT_HDR_FMT  = "NNC";
my $PKT_HDR_SIZE = 9;

my $fn = shift @ARGV;
my $fh;
open $fh, "<$fn" or die "Can't open '$fn' for reading: '$!'";

my $len;
my $hdr;
while(($len = read $fh, $hdr, $PKT_HDR_SIZE) == $PKT_HDR_SIZE) {
	my ($time, $ip, $size) = unpack $PKT_HDR_FMT, $hdr;

	my $pkt;
	((read $fh, $pkt, ($size - 1)) == ($size - 1)) or die "Payload read: $!";

	my $data = substr $pkt, 0, $size-3;
	my $crc = substr $pkt, $size-3, 2;
	# die if(crcccitt((chr $size).$data) != unpack "n", $crc);

    printf "%08X %016X 00000 %02X", $ip, $time, $size;
    map { printf "%02X", ord $_ } split //, ($data.$crc);
	printf "\n";
}

die "Header read: $!" if not defined $len;
