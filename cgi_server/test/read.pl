#/usr/bin/perl
print "ARGV=",@ARGV, "\n";
print "ARGV[0]=",$ARGV[0], "\n";
print "ARGV[1]=",$ARGV[1], "\n";
open(DNAFILE, $ARGV[0]) or die "cant open $ARGV[0]: $!";
while(<DNAFILE>) { 
	print "line $_\n";
}
close(DNAFILE);
