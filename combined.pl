#! perl -w

use strict;
package cvspragma_example;

my $mode = shift || '';
my $program_secret = "world";

if ($mode eq 'client') {
	client();
} elsif ($mode eq 'server') {
	server();
} elsif ($mode eq 'callback') {
	callback();
} else {
	print "$0 Must be called with one of client, server, or callback\n";
}

sub client {
	print "This is the client side\n";

	system ("cvs pragma server combined $program_secret ".scalar(localtime()));
}

sub server {
	if ($ARGV[0] ne $program_secret) {
	   print "Who are you, imposter?\n";
	   exit 1;
	} else {
	   print "The time sponsored by CVS pragma is ".$ARGV[4]."\n";
	}
	return 0;
}

sub callback {
	print "Not supported yet\n";
}
