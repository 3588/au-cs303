#!/usr/bin/perl -w

#
# Generate and run a series of random (but non-trivial) test cases for
# the Bowtie suite of tools, including bowtie, bowtie-build,
# bowtie-maptool, and bowtie-inspect.  Most of the problems turned up
# this way are in the form of assertions in the tools.  However, we
# also do some sanity-checking of the results; e.g. we use the
# pe_verify.pl script to verify that paired-end alignments are
# consistent with matched-up single-end alignments.  This script does
# not compare single-ended results against an oracle.
#
# Usage: perl random_tester.pl [rand seed] \
#                              [# outer iters] \
#                              [# inner iters] \
#                              [min # text bases] \
#                              [max text bases to add to min] \
#                              [min # read bases] \
#                              [max read bases to add to min]
#
# Options:
#   -n         don't attempt to compile binaries; use existing binaries
#   -p "<pol>" use only the specified alignment policy (e.g. "-n 3")
#

use List::Util qw[min max];
use Getopt::Std;

my %options=();
getopts("mhnop:we",\%options);

if(defined $options{h}) {
	print "Usage: perl random_bowtie_tests.pl seed outer inner tbase trand pbase prand\n";
	exit 0;
}

# Let user specify a policy that will always override pickPolicy()
my $setPolicy;
$setPolicy = $options{p} if defined($options{p});

# make all the relevant binaries, unless we were asked not to
unless(defined $options{n}) {
	system("make bowtie bowtie-debug bowtie-build-debug ".
	       "bowtie-maptool-debug bowtie-inspect-debug") == 0 || die "Error building";
}

# Alignment policies
my @policies = (
	"-n 3",
	"-n 2",
	"-n 1",
	"-n 0",
	"-v 3",
	"-v 2",
	"-v 1",
	"-v 0"
);

sub pickPolicy {
	my $pe = shift;
	my $r = int(rand($#policies + 1));
	my $pol = $policies[$r];
	$pol = $setPolicy if defined($setPolicy);
	if($pe && int(rand(2)) == 0) {
		my $min = int(rand(200))+10;
		$pol .= " -I $min";
		$pol .= " -X ".int(rand(30)+$min);
	}
	if(!$pe && int(rand(2)) == 0) {
		# Possibly ask for best alignments
		$pol .= " --best";
	} elsif(!$pe && int(rand(2)) == 0) {
		$pol .= " --better";
	} elsif(!$pe && int(rand(2)) == 0) {
		$pol .= " --oldbest";
	}
	if($pol =~ /-n/ && int(rand(2)) == 0) {
		$pol .= " --nomaqround";
	}
	if($pol =~ /-n/ && int(rand(2)) == 0) {
		$pol .= " -l ".int(rand(30)+8);
	}
	return $pol;
}

my $seed = 0;
$seed = int $ARGV[0] if defined($ARGV[0]);
srand $seed;

my $outer = 5000;
$outer = int $ARGV[1] if defined($ARGV[1]);
my $limit = $outer;

my $inner = 100;
$inner = int $ARGV[2] if defined($ARGV[2]);

my $tbase = 100;
$tbase = int $ARGV[3] if defined($ARGV[3]);
my $trand = 300;
$trand = int $ARGV[4] if defined($ARGV[4]);

my $pbase = 10;
$pbase = int $ARGV[5] if defined($ARGV[5]);
my $prand = 30;
$prand = int $ARGV[6] if defined($ARGV[6]);

my $ibase = 50;
$ibase = int $ARGV[7] if defined($ARGV[7]);
my $irand = 250;
$irand = int $ARGV[8] if defined($ARGV[8]);

my $verbose = 0;
my $exitOnFail = 1;
my @dnaMap = ('A', 'T', 'C', 'G',
              'N',
              'M', 'R', 'W', 'S', 'Y', 'K', 'V', 'H', 'D', 'B', 'X');

sub nonACGTtoN {
	my $t = shift;
	$t =~ tr/-NnMmRrWwSsYyKkVvHhDdBbXx/N/;
	$t =~ /[ACGTN]+/ || die "Bad N-ized DNA string: $t";
	return $t;
}

sub randGap() {
	my $or = int(rand(4));
	my $gap = "";
	if($or == 0) {
		my $ir = int(rand(100))+1;
		if(($ir & 1) == 1) {
			for(my $i = 0; $i < $ir; $i++) {
				$gap .= 'N';
			}
		} else {
			for(my $i = 0; $i < $ir; $i++) {
				$gap .= '-';
			}
		}
	}
	return $gap;
}

# Utility function generates a random DNA string of the given length
sub randDna($) {
	my $num = shift;
	my $i;
	my $t = '';
	my $noAmbigs = int(rand(3)) == 0;
	for($i = 0; $i < $num; $i++) {
		my $or = int(rand(50));
		if($or == 0 && !$noAmbigs) {
			# Add a random, possibly ambiguous character
			$t .= $dnaMap[int(rand($#dnaMap+1))];
		} elsif($or == 1 && !$noAmbigs) {
			# Add a random-length streak of Ns (max: 20)
			my $streak = int(rand(20))+1;
			for(my $j = $i; $j < $num && $j < $streak; $j++) {
				$t .= 'N';
			}
		} else {
			# Add a random non-ambiguous character
			$t .= $dnaMap[int(rand(4))];
		}
	}
	return $t;
}

# Utility function that returns the reverse complement of its argument
sub reverseComp($) {
	my $r = shift;
	$r = reverse($r);
	$r =~ tr/aAcCgGtT/tTgGcCaA/;
	return $r;
}

# Add some random quality values to encourage excercising the
# backtracking code
sub addQual($) {
	my $r = shift;
	my $len = length($r);
	$r .= ":";
	for(my $i = 0; $i < $len; $i++) {
		my $c = "-";
		while(not $c =~ /[0-9A-Z\/=@%]/) {
			$c = chr(33 + int(rand(41)));
		}
		$r .= $c;
	}
	return $r;
}

# Trim whitespace from a string argument
sub trim($) {
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	return $string;
}

# Build an Ebwt based on given arguments
sub build {
	my($t, $lineRate, $linesPerSide, $offRate, $isaRate, $ftabChars, $endian) = @_;
	my $ret = 0;
	
	my $file1 = "-c";
	my $file2 = "\"$t\"";
	if(substr($t, 0, 1) eq '-') {
		# Add backslash to escape first dash
		$file2 = "\"\\$t\"";
	}
	
	# Write reference sequences to a FASTA file
	open FA, ">.randtmp$seed.fa" || die "Could not open temporary fasta file";
	my @seqs = split(/,/, $t);
	for(my $i = 0; $i <= $#seqs; $i++) {
		print FA ">$i\n";
		print FA "$seqs[$i]\n";
	}
	close(FA);
	
	# Make a version of the FASTA file where all non-A/C/G/T characters
	# are Ns.
	open FAN, ">.randtmp$seed.ns.fa" || die "Could not open temporary fasta file";
	for(my $i = 0; $i <= $#seqs; $i++) {
		print FAN ">$i\n";
		my $t = nonACGTtoN($seqs[$i]);
		print FAN "$t\n";
	}
	close(FAN);
	
	my $fasta = int(rand(2)) == 0;
	if($fasta) {
		# Use the FASTA file as input
		$file1 = "-f";
		$file2 = ".randtmp$seed.fa";
	}
	
	my $bucketArg = "";
	my $bucketRand = int(rand(3));
	if($bucketRand == 0) {
		$bucketArg = "--bmaxdivn ";
		$bucketArg .= (int(rand(30))+1);
	} elsif($bucketRand == 1) {
		$bucketArg = "-a ";
	}
	
	my $isaArg = "";
	if($isaRate >= 0) {
		$isaArg = "--isarate $isaRate";
	}

	my $args = "-q $isaArg --sanity $file1 --offrate $offRate --ftabchars $ftabChars $bucketArg $endian $file2";
	
	# Do unpacked version
	my $cmd = "./bowtie-build-debug $args .tmp$seed";
	system("echo \"$cmd\" > .tmp$seed.cmd");
	print "$cmd\n";
	my $out = trim(`$cmd 2>&1`);
	if($out eq "") {
		$ret++;
	} else {
		print "$out\n";
		if($exitOnFail) {
			exit 1;
		}
	}
	
	# Use bowtie-inspect to compare the output of bowtie-build to the
	# original reference sequences
	$cmd = "./bowtie-inspect-debug -a -1 .tmp$seed > .tmp$seed.inspect.ref";
	print "$cmd\n";
	system($cmd) == 0 || die "$cmd - failed";
	$cmd = "diff .randtmp$seed.ns.fa .tmp$seed.inspect.ref";
	print "$cmd\n";
	system($cmd) == 0 || die "$cmd - failed";

	# Do packed version and assert that it matches unpacked version
	# (sometimes, but not all the time because it takes a while)
	if(int(rand(4)) == 0) {
		$cmd = "./bowtie-build-debug -p $args .tmp$seed.packed";
		print "$cmd\n";
		$out = trim(`$cmd 2>&1`);
		if($out eq "") {
			if(system("diff .tmp$seed.1.ebwt .tmp$seed.packed.1.ebwt") != 0) {
				if($exitOnFail) {
					exit 1;
				}
			} elsif(system("diff .tmp$seed.2.ebwt .tmp$seed.packed.2.ebwt") != 0) {
				if($exitOnFail) {
					exit 1;
				}
			} else {
				$ret++;
			}
		} else {
			print "$out\n";
			if($exitOnFail) {
				exit 1;
			}
		}
	}
	
	return $ret;
}

sub deleteReadParts {
	system("rm -f .tmp.un$seed". ".* .tmp.un$seed". "_1.* .tmp.un$seed". "_2.*");
	system("rm -f .tmp.max$seed".".* .tmp.max$seed"."_1.* .tmp.max$seed"."_2.*");
	system("rm -f .tmp.al$seed". ".* .tmp.al$seed". "_1.* .tmp.al$seed". "_2.*");
}

sub search {
	my($t, $pe, $p1, $p2, $policy, $oneHit, $requireResult, $offRate, $isaRate) = @_;
	my $ret = doSearch($t, $pe, $p1, $p2, $policy, $oneHit, $requireResult, $offRate, $isaRate);
	deleteReadParts();
	return $ret;
}

##
# Make sure that a verbose alignment is consistent with the content of
# the reference
#
sub checkRefVerbose {
	my ($l, $ttsr) = @_;
	my @tts = @{$ttsr};
	my @ls = split(/\t/, $l);
	my ($fw, $ref, $off, $seq, $mms) = ($ls[1], $ls[2], $ls[3], $ls[4], $ls[7]);
	# Parse mismatches
	my %mmhash = ();
	my @mmss;
	@mmss = split(/,/, $mms) if defined($mms);
	for my $m (@mmss) {
		my @ms = split(/:/, $m);
		my $moff = $ms[0];
		$moff == int($moff) || die "Mismatch offset must be integer";
		$moff = length($seq)-$moff-1 if $fw eq "-";
		$mmhash{$ms[0]}{ref} = substr($ms[1], 0, 1);
		$mmhash{$ms[0]}{sub} = substr($ms[1], 2, 1);
		substr($seq, $moff, 1) eq $mmhash{$ms[0]}{sub} || die;
		substr($seq, $moff, 1) = $mmhash{$ms[0]}{ref};
	}
	$ref == int($ref) || die;
	$ref <= $#tts || die "Ref idx $ref exceeds number of texts ".($#tts+1)."\n";
	my $refstr = substr($tts[$ref], $off, length($seq));
	print $tts[$ref].'\n';
	for(my $i = 0; $i < length($seq); $i++) {
		substr($refstr, $i, 1) ne "N" || die;
		substr($refstr, $i, 1) eq substr($seq, $i, 1) || die "\n$seq\n$refstr";
	}
	return 1;
}

sub checkRefConcise {
	my ($l, @tts) = @_;
	return 1;
}

# Search for a pattern in an existing Ebwt
sub doSearch {
	my($t, $pe, $p1, $p2, $policy, $oneHit, $requireResult, $offRate, $isaRate) = @_;
	
	my @tts = split(/,/, $t);
	
	my $patarg = "-c";
	my $patstr = "\"$p1\"";
	$patstr = "-1 \"$p1\" -2 \"$p2\"" if $pe;
	my $outformat = int(rand(3));
	my $maptool_cmd = "";
	my $outfile = ".tmp$seed.out";
	
	if($outformat == 0) {
		$outformat = ""; # default (verbose)
		$maptool_cmd = " && ./bowtie-maptool-debug -d -C .tmp$seed.out"; # default
	} elsif($outformat == 1) {
		$outformat = "-b"; # binary
		$maptool_cmd = " && ./bowtie-maptool-debug -b -C .tmp$seed.out"; # binary
	} else {
		$outformat = "--concise"; # concise
	}
	
	my $ext = ".fq";
	my $format = int(rand(5));
	if($format == 0) {
		# FASTA
		open FA, ">.randread$seed.fa" || die "Could not open temporary fasta file";
		$ext = ".fa";
		my @cs = split /[,]/, $p1;
		my $idx = 0;
		foreach my $c (@cs) {
			my @cms = split /[:]/, $c;
			print FA ">$idx\n$cms[0]\n";
			$idx++;
		}
		close FA;
		$patarg = "-f";
		$patstr = ".randread$seed.fa";
		if($pe) {
			system("mv .randread$seed.fa .randread$seed"."_1.fa");
			$patstr = "-1 .randread$seed"."_1.fa";
			open FA, ">.randread$seed"."_2.fa" || die "Could not open temporary fasta file";
			@cs = split /[,]/, $p2;
			my $idx = 0;
			foreach my $c (@cs) {
				my @cms = split /[:]/, $c;
				print FA ">$idx\n$cms[0]\n";
				$idx++;
			}
			close FA;
			$patstr .= " -2 .randread$seed"."_2.fa";
		}
	} elsif($format == 1) {
		# FASTQ with ASCII qualities
		open FQ, ">.randread$seed.fq" || die "Could not open temporary fastq file";
		$ext = ".fq";
		my @cs = split /[,]/, $p1;
		my $idx = 0;
		foreach my $c (@cs) {
			my @cms = split /[:]/, $c;
			print FQ "\@$idx\n$cms[0]\n+\n$cms[1]\n";
			$idx++;
		}
		close FQ;
		$patarg = "-q";
		$patstr = ".randread$seed.fq";
		if($pe) {
			system("mv .randread$seed.fq .randread$seed"."_1.fq");
			$patstr = "-1 .randread$seed"."_1.fq";
			open FQ, ">.randread$seed"."_2.fq" || die "Could not open temporary fastq file";
			@cs = split /[,]/, $p2;
			my $idx = 0;
			foreach my $c (@cs) {
				my @cms = split /[:]/, $c;
				print FQ "\@$idx\n$cms[0]\n+\n$cms[1]\n";
				$idx++;
			}
			close FQ;
			$patstr .= " -2 .randread$seed"."_2.fq";
		}
	} elsif($format == 2) {
		# FASTQ with integer qualities
		open FQ, ">.randread$seed.integer.fq" || die "Could not open temporary solexa fastq file";		my $ext = ".fa";
		$ext = ".fq";
		my @cs = split /[,]/, $p1;
		my $idx = 0;
		foreach my $c (@cs) {
			my @cms = split /[:]/, $c;
			print FQ "\@$idx\n$cms[0]\n+\n";
			for(my $i = 0; $i < length($cms[1]); $i++) {
				my $q = substr($cms[1], $i, 1);
				$q = ord($q) - 33;
				print FQ "$q ";
			}
			print FQ "\n";
			$idx++;
		}
		close FQ;
		$patarg = "-q --integer-quals";
		$patstr = ".randread$seed.integer.fq";
		if($pe) {
			system("mv .randread$seed.integer.fq .randread$seed.integer_1.fq");
			$patstr = "-1 .randread$seed.integer_1.fq";
			open FQ, ">.randread$seed.integer_2.fq" || die "Could not open temporary fastq file";
			@cs = split /[,]/, $p2;
			my $idx = 0;
			foreach my $c (@cs) {
				my @cms = split /[:]/, $c;
				print FQ "\@$idx\n$cms[0]\n+\n";
				for(my $i = 0; $i < length($cms[1]); $i++) {
					my $q = substr($cms[1], $i, 1);
					$q = ord($q) - 33;
					print FQ "$q ";
				}
				$idx++;
				print FQ "\n";
			}
			close FQ;
			$patstr .= " -2 .randread$seed.integer_2.fq";
		}
	} elsif($format == 3) {
		# Raw
		open RAW, ">.randread$seed.raw" || die "Could not open temporary raw file";
		$ext = ".raw";
		my @cs = split /[,]/, $p1;
		foreach my $c (@cs) {
			my @cms = split /[:]/, $c;
			print RAW "$cms[0]\n";
		}
		close RAW;
		$patarg = "-r";
		$patstr = ".randread$seed.raw";
		if($pe) {
			system("mv .randread$seed.raw .randread$seed"."_1.raw");
			$patstr = "-1 .randread$seed"."_1.raw";
			open RAW, ">.randread$seed"."_2.raw" || die "Could not open temporary fastq file";
			@cs = split /[,]/, $p2;
			foreach my $c (@cs) {
				my @cms = split /[:]/, $c;
				print RAW "$cms[0]\n";
			}
			close RAW;
			$patstr .= " -2 .randread$seed"."_2.raw";
		}
	}
	
	# Perhaps use phased search w/ -z option
	my $phased = "";
	if(!$pe && int(rand(3)) == 0) {
		$phased = "-z";
		# These aren't available in -z mode
		$policy =~ s/--best//;
		$policy =~ s/--oldbest//;
		$policy =~ s/--better//;
	}
	
	# Perhaps dump unaligned reads using --un argument
	my $unalignArg = "";
	my $unalignReconArg = "";
	my $unalign = int(rand(5));
	if($unalign == 0 || $unalign == 2) {
		$unalignArg .= "--un .tmp.un$seed$ext ";
		if($pe) {
			$unalignReconArg .= " .tmp.un$seed"."_1$ext .tmp.un$seed"."_2$ext";
		} else {
			$unalignReconArg .= " .tmp.un$seed$ext";
		}
	}
	if($unalign == 1 || $unalign == 2) {
		$unalignArg .= "--max .tmp.max$seed$ext ";
		if($unalign == 2) {
			if($pe) {
				$unalignReconArg .= " .tmp.max$seed"."_1$ext .tmp.max$seed"."_2$ext";
			} else {
				$unalignReconArg .= " .tmp.max$seed$ext";
			}
		}
	}
	if(($unalign == 2 || $unalign == 3) && $phased eq "") {
		$unalignArg .= "--al .tmp.al$seed$ext ";
		if($unalign == 2) {
			if($pe) {
				$unalignReconArg .= " .tmp.al$seed"."_1$ext .tmp.al$seed"."_2$ext";
			} else {
				$unalignReconArg .= " .tmp.al$seed$ext";
			}
		}
	}
	
	my $isaArg = "";
	if($isaRate >= 0) {
		$isaArg = "--isarate $isaRate";
	}
	
	my $khits = "-k 1";
	my $mhits = 0;
	if($phased eq "-z") {
		# A phased search may optionally be a non-stratified all-hits
		# search
		if(int(rand(2)) == 0) {
			$khits = "-a";
		}
	} else {
		if(int(rand(2)) == 0) {
			$khits = "-a";
		} else {
			$khits = "-k " . (int(rand(20))+2);
		}
		if(int(rand(3)) == 0) {
			$policy =~ s/--oldbest//;
			$policy =~ s/--better//;
			$khits .= " --strata --best";
		}
		if(int(rand(2)) == 0) {
			$requireResult = 0;
			$mhits = (int(rand(20))+2);
		}
	}
	
	if($mhits > 0) {
		$khits .= " -m $mhits";
	}
	
	my $strand = "";
	if(int(rand(4)) == 0) {
		if(int(rand(2)) == 0) {
			# Reverse-complement reference strand only
			$strand = "--nofw";
		} else {
			# Forward reference strand only
			$strand = "--norc";
		}
	}
	
	if($oneHit || 1) {
		$oneHit = "";
	} else {
		$oneHit = "-a";
	}
	my $offRateStr = "";
	if(int(rand(3)) == 0) {
		$offRateStr = "--offrate " . ($offRate + 1 + int(rand(4)));
	}
	my $cmd = "./bowtie-debug $policy $strand $unalignArg $khits $outformat $isaArg $offRateStr --orig \"$t\" $phased $oneHit --sanity $patarg .tmp$seed $patstr $outfile $maptool_cmd";
	print "$cmd\n";
	my $out = trim(`$cmd 2>.tmp$seed.stderr | tee .tmp$seed.stdout`);
	
	# Bad exitlevel?
	if($? != 0) {
		print "Exitlevel: $?\n";
		if($exitOnFail) {
			my $err = trim(`cat .tmp$seed.stderr 2> /dev/null`);
			print "Stdout:\n$out\nStderr:\n$err\n";
			exit 1;
		}
		return 0;
	}
	my $err = trim(`cat .tmp$seed.stderr 2> /dev/null`);

	# No output?
	if($out eq "" && $requireResult) {
		print "Expected results but got \"No Results\"\n";
		if($exitOnFail) {
			print "Stdout:\n$out\nStderr:\n$err\n";
			exit 1;
		}
		return 0;
	}
	
	# Parse output to see if any of it is bad
	my @outlines = split('\n', $out);
	my %outhash = ();
	my %readcount = ();
	my %readStratum = (); # for unpaired reads
	my %readStratum1 = (); # for mate 1
	my %readStratum2 = (); # for mate 2
	my $lastread = "";
	
	for(my $i = 0; $i <= $#outlines; $i++) {
		# Get the alignment for the first mate (or the unpaired read)
		my $l = $outlines[$i];
		next if $l =~ /^Reported/; # skip the summary line
		chomp($l);
		checkRefConcise($l, @tts);
		my $key = "$l";
		my $l2 = "";
		if($pe) {
			# Get the alignment for the second mate
			$l2 = $outlines[++$i];
			defined($l2) || die "Odd number of output lines";
			chomp($l2);
			checkRefConcise($l2, @tts);
			$key .= ", $l2";
		}
		print "$key\n";
		# No two results should be the same
		if(!$pe) {
			!defined($outhash{$key}) || die "Result $key appears in output twice";
		}
		$outhash{$key} = 1;
		# Result should look like "4+:<4,231,0>"
		my $wellFormed = 0;
		$wellFormed = ($l =~ m/^([01-9]+)([+-])[:]<[01-9]+,[01-9]+,([01-9]+)>$/) unless $pe;
		$wellFormed = ($l =~ m/^([01-9]+)\/([12])([+-])[:]<[01-9]+,[01-9]+,([01-9]+)>$/) if $pe;
		unless($wellFormed) {
			print "Results malformed\n";
			print "$out\n";
			if($exitOnFail) {
				print "Stdout:\n$out\nStderr:\n$err\n";
				exit 1;
			}
			return 0;
		}
		# Parse out the read id
		my $read = $1;
		my $mate = 0;
		my $fw = $2;
		my $stratum = $3;
		if($pe) {
			$mate = $2;
			$fw = $3;
			$stratum = $4;
		}
		if($strand =~ /nofw/) {
			if($pe) {
				"$mate$fw" eq "2+" || die "Saw a forward alignment when --nofw was specified";
			} else {
				$fw eq "-" || die "Saw a forward alignment when --nofw was specified";
			}
		} elsif($strand =~ /norc/) {
			if($pe) {
				"$mate$fw" eq "1+" || die "Saw a forward alignment when --norc was specified";
			} else {
				$fw eq "+" || die "Saw a rev-comp alignment when --norc was specified";
			}
		}
		if(($read ne $lastread) && ($phased eq "")) {
			die "Read $read appears multiple times non-consecutively" if defined($readcount{$read});
		}
		if(!$pe && $policy =~ /--strata /) {
			!defined($readStratum{$read}) ||
				$readStratum{$read} == $stratum ||
				die "Incompatible strata: old: $readStratum{$read}, cur: $stratum";
			$readStratum{$read} = $stratum;
		} elsif($policy =~ /--strata /) {
			$l  =~ m/^([01-9]+\/[12])[+-]?[:]<[01-9]+,[01-9]+,([01-9]+)>$/;
			my $mate1 = $1;
			my $stratum1 = $2;
			$l2 ne "" || die;
			$l2 =~ m/^([01-9]+\/[12])[+-]?[:]<[01-9]+,[01-9]+,([01-9]+)>$/;
			my $mate2 = $1;
			my $stratum2 = $2;
			if($mate2 =~ /\/1$/) {
				# Swticheroo
				my $tmp = $mate1;
				$mate1 = $mate2;
				$mate2 = $tmp;
				$tmp = $stratum1;
				$stratum1 = $stratum2;
				$stratum2 = $tmp;
			}
			if(defined($readStratum1{$mate1})) {
				# One or ther other mate has to have the same stratum as we recorded previously
				defined($readStratum2{$mate2}) || die "$mate1 was seen before, but not $mate2";
				$readStratum1{$mate1} == $stratum1 ||
				$readStratum2{$mate2} == $stratum2 ||
				$readStratum1{$mate1} == $stratum2 ||
				$readStratum2{$mate2} == $stratum1 ||
					die "Incompatible strata: old: <$readStratum1{$mate1}, $readStratum2{$mate2}> cur: <$stratum1, $stratum2>";
			}
			$readStratum1{$mate1} = $stratum1;
			$readStratum2{$mate2} = $stratum2;
		}
		$lastread = $read;
		$readcount{$read}++ if defined($readcount{$read});
		$readcount{$read} = 1 unless defined($readcount{$read});
		if($mhits > 0) {
			$readcount{$read} <= $mhits || die "Read $read matched more than $mhits times";
		}
	}

	{
		$cmd = "./bowtie $policy $strand $unalignArg $khits $outformat $isaArg $offRateStr --orig \"$t\" $phased $oneHit --sanity $patarg .tmp$seed $patstr $outfile $maptool_cmd";
		print "$cmd\n";
		my $out2 = trim(`$cmd 2>.tmp$seed.stderr`);
		$out2 eq $out || die "Normal bowtie output did not match debug bowtie output";

		$cmd = "./bowtie $policy $strand $unalignArg $khits $outformat --orig \"$t\" $phased $oneHit --sanity $patarg .tmp$seed $patstr $outfile $maptool_cmd";
		print "$cmd\n";
		my $out3 = trim(`$cmd 2>.tmp$seed.stderr`);

		$cmd = "./bowtie --mm $policy $strand $unalignArg $khits $outformat --orig \"$t\" $phased $oneHit --sanity $patarg .tmp$seed $patstr $outfile $maptool_cmd";
		print "$cmd\n";
		my $out4 = trim(`$cmd 2>.tmp$seed.stderr`);
		$out3 eq $out4 || die "Normal bowtie output did not match memory-mapped bowtie output";
	}
	
	# Now do another run with verbose output so that we can check the
	# mismatch strings
	$cmd = "./bowtie-debug $policy $strand $unalignArg $khits $isaArg $offRateStr --orig \"$t\" $phased $oneHit --sanity $patarg .tmp$seed $patstr";
	print "$cmd\n";
	$out = trim(`$cmd 2>.tmp$seed.stderr`);
	# Parse output to see if any of it is bad
	@outlines = split('\n', $out);
	for my $l (@outlines) {
		checkRefVerbose($l, \@tts);
	}
	
	if($pe) {
		# If search was for paired-end alignments, then verify the
		# outcome using pe_verify.pl
		my $pol = $policy;
		$pol =~ s/--.*//;
		my $patstr2 = $patstr;
		$patstr2 =~ s/-1//;
		$patstr2 =~ s/-2//;
		$cmd = "perl scripts/pe_verify.pl -d $pol .tmp$seed $patstr2";
		print "$cmd\n";
		$out = trim(`$cmd 2>.tmp$seed.pe_verify.stderr`);
		
		# Bad exitlevel?
		if($? != 0) {
			print "scripts/pe_verify.pl exitlevel: $?\n";
			if($exitOnFail) {
				my $err = trim(`cat .tmp$seed.pe_verify.stderr 2> /dev/null`);
				print "Stdout:\n$out\nStderr:\n$err\n";
				exit 1;
			}
			return 0;
		}
	}
	
	if(!$pe && $policy =~ /--best|--better/) {
		$cmd = "perl scripts/best_verify.pl -d $policy .tmp$seed $patstr";
		print "$cmd\n";
		$out = trim(`$cmd 2>.tmp$seed.best_verify.stderr`);
		
		# Bad exitlevel?
		if($? != 0) {
			print "scripts/best_verify.pl exitlevel: $?\n";
			if($exitOnFail) {
				my $err = trim(`cat .tmp$seed.best_verify.stderr 2> /dev/null`);
				print "Stdout:\n$out\nStderr:\n$err\n";
				exit 1;
			}
			return 0;
		}
	}
		
	# Run the reconciler to ensure that --un, --max, and --al had
	# sane output
	# .tmp$seed.verbose.out
	if($format >= 0 && $format <= 2 && $unalignReconArg ne "") {
		deleteReadParts();
		my $cmd = "./bowtie $policy $strand $unalignArg $khits $isaArg $offRateStr --orig \"$t\" $phased $oneHit --sanity $patarg .tmp$seed $patstr .tmp$seed.verbose.out";
		print "$cmd\n";
		system($cmd) == 0 || die;
		$khits =~ s/--strata --best//;
		$patarg =~ s/--integer-quals//;
		$unalignReconArg =~ /\.tmp\.un/ || die;
		if($pe) {
			$patstr =~ s/-1//;
			$patstr =~ s/-2//;
			$cmd = "perl scripts/reconcile_alignments_pe.pl $patarg $khits $patstr .tmp$seed.verbose.out $unalignReconArg";
			print "$cmd\n";
			system($cmd) == 0 || die;
		} else {
			$cmd = "perl scripts/reconcile_alignments.pl $patarg $khits $patstr .tmp$seed.verbose.out $unalignReconArg";
			print "$cmd\n";
			system($cmd) == 0 || die;
		}
	}
	
	# Success
	return 1;
}

my $pass = 0;
my $tests = 0;
my $fail = 0;

for(; $outer > 0; $outer--) {

	# Generate random parameters
	my $lineRate = 4 + int(rand(7));     # Must be >= 4
	my $linesPerSide = 1 + int(rand(3));
	my $offRate = int(rand(16));         # Can be anything
	my $isaRate = int(rand(8))-1;        # Can be anything
	my $ftabChars = 1 + int(rand(8));    # Must be >= 1
	my $big = int(rand(2));
	my $endian = '';
	if($big) {
		$endian = "--big";
	} else {
		$endian = "--little";
	}

	# Generate random text(s)
	my $nt = int(rand(10)) + 1;
	my $t = '';
	my @tts;
	# For each reference sequence...
	for(my $i = 0; $i < $nt; $i++) {
		my $tlen = $tbase + int(rand($trand));
		my $tt = randDna($tlen);             # add text meat
		push(@tts, $tt);
		$t .= (randGap() . $tt . randGap()); # add random padding
		if($i < $nt-1) { $t .= ","; }        # add comma separator
	}
	
	# Run the command to build the Ebwt from the random text
	$pass += build($t, $lineRate, $linesPerSide, $offRate, $isaRate, $ftabChars, $endian);
	last if(++$tests > $limit);

	my $in = $inner;
	for(; $in >= 0; $in--) {
		# Paired-end?
		my $pe = (int(rand(2)) == 0);
		$pe = 1 if defined($options{e});
		# Generate random pattern(s) based on text
		my $pfinal1 = '';
		my $pfinal2 = '';
		# Decide number of patterns to generate
		my $np = int(rand(30)) + 1;
		for(my $i = 0; $i < $np; $i++) {
			# Pick a text
			my $tt = $tts[int(rand($#tts))];
			# Pick a length
			my $pl;
			my $plen;
			my $pr;
			my $p1;
			my $p2;
			if($pe) {
				# Pick a left-hand offset for the insert
				my $il = int(rand(length($tt))) - 10;
				# Pick a length for the insert
				my $ilen = int(rand($irand)) + $ibase;
				$il = min($il, length($tt)-$ilen);
				my $ir = min($il + $ilen, length($tt));
				my $is = substr $tt, $il, $ir - $il;
				# Pick a length for the #1 mate
				my $plen1 = int(rand($prand)) + $pbase;
				# Pick a length for the #2 mate
				my $plen2 = int(rand($prand)) + $pbase;
				$p1 = substr $is, 0, $plen1;
				$is = reverse $is;
				$p2 = substr $is, 0, $plen2;
				$p2 = reverse $p2;
				$p2 = reverseComp($p2);
			} else {
				# Pick a length for the read
				$plen = int(rand($prand)) + $pbase;
				$pl = int(rand(length($tt))) - 10;
				$pl = max($pl, 4);
				$pl = min($pl, length($tt));
				$pr = min($pl + $plen, length($tt));
				$p1 = substr $tt, $pl, $pr - $pl;
			}
			# Check for empty pattern or pattern that spans a comma
			if(length($p1) < 4 || index($p1, ",") != -1) {
				$i--; next;
			}
			if($pe && (length($p2) < 4 || index($p2, ",") != -1)) {
				$i--; next;
			}
			# Optionally add mutations to pattern (but not the first)
			if($i > 0) {
				my $nummms = int(rand(4));
				for(my $j = 0; $j < $nummms; $j++) {
					substr($p1, int(rand(length($p1))), 1, randDna(1));
				}
				if($pe) {
					$nummms = int(rand(4));
					for(my $j = 0; $j < $nummms; $j++) {
						substr($p2, int(rand(length($p2))), 1, randDna(1));
					}
				}
			}
			# Possibly reverse complement it
			if((int(rand(2)) == 0)) {
				$p1 = reverseComp($p1);
				if($pe) {
					$p2 = reverseComp($p2);
					my $ptmp = $p1;
					$p1 = $p2;
					$p2 = $ptmp;
				}
			}
			$p1 =~ tr/MRWSYKVHDBX/N/;
			# Add valid random quality values
			$p1 = addQual($p1);
			$pfinal1 .= $p1;
			$pfinal1 .= "," if($i < $np-1);
			if($pe) {
				$p2 =~ tr/MRWSYKVHDBX/N/;
				$p2 = addQual($p2);
				$pfinal2 .= $p2;
				$pfinal2 .= "," if($i < $np-1);
			}
		}
		
		# Run the command to search for the pattern from the Ebwt
		my $oneHit = (int(rand(3)) == 0);
		my $policy = pickPolicy($pe);
		my $expectResult = 1;
		if(!$pe) {
			for(my $i = 0; $i < length($pfinal1); $i++) {
				my $c = substr($pfinal1, $i, 1);
				last if($c eq ',');
				if($c ne 'A' && $c ne 'C' && $c ne 'G' && $c ne 'T') {
					$expectResult = 0;
					last;
				}
			}
		} else {
			$expectResult = 0;
		}
		$pass += search($t, $pe, $pfinal1, $pfinal2, $policy, $oneHit, $expectResult, $offRate, $isaRate); # require 1 or more results
		last if(++$tests > $limit);
	}

	$in = $inner;
	for(; $in >= 0; $in--) {
		my $pe = (int(rand(2)) == 0);
		# Generate random pattern *not* based on text
		my $pfinal1 = '';
		my $pfinal2 = '';
		my $np = int(rand(10)) + 1;
		for(my $i = 0; $i < $np; $i++) {
			my $plen = int(rand($prand)) + $pbase;
			my $p1 = randDna($plen);
			$plen = int(rand($prand)) + $pbase;
			my $p2 = randDna($plen);
			$p1 =~ tr/MRWSYKVHDBX/N/;
			$p1 = addQual($p1);
			$p2 =~ tr/MRWSYKVHDBX/N/ if $pe;
			$p2 = addQual($p2) if $pe;
			$pfinal1 .= $p1;
			$pfinal2 .= $p2 if $pe;
			if($i < $np-1) {
				$pfinal1 .= ",";
				$pfinal2 .= "," if $pe;
			}
		}

		# Run the command to search for the pattern from the Ebwt
		my $oneHit = (int(rand(3)) == 0);
		my $policy = pickPolicy($pe);
		$pass += search($t, $pe, $pfinal1, $pfinal2, $policy, $oneHit, 0, $offRate, $isaRate); # do not require any results
		last if(++$tests > $limit);
	}
}

print "$pass tests passed, $fail failed\n";
exit 1 if $fail > 0;
