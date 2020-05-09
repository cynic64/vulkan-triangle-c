#!/usr/bin/perl -w

use strict;

my $M_RUN = 0;
my $M_TEST = 1;

my $usage = "Usage: comp.pl <run|test> [file]";
die $usage if ($#ARGV < 0 || $#ARGV > 1);

my $mode;
if ($ARGV[0] eq "run") {
    $mode = $M_RUN;
    die $usage unless $#ARGV == 1;
} elsif ($ARGV[0] eq "test") {
    $mode = $M_TEST;
    die $usage unless $#ARGV == 0;
} else {
    die $usage;
}

my @pkgs = ("vulkan", "glfw3", "cglm", "check");
my $flags = "";
for (@pkgs) {
    my $pflags = qx(pkg-config --cflags --libs $_);
    chomp $pflags;
    $flags .= " $pflags";
}
# link math.h
$flags .= " -lm";

my @files;
push @files, <src/*.c>;
push @files, <tests-src/*.c> if $mode == $M_TEST;

my $str_files = join " ", @files;

my $main;
if ($mode == $M_TEST) {
    $main = "test-runner/runner.c"
} elsif ($mode == $M_RUN) {
    $main = "demo/$ARGV[1].c"
} else {
    die;
}

my $out;
if ($main =~ m|.*/(.*)\.c|) {
    $out = "bin/$1";
} else {
    die;
}

my $dump = "out-$ARGV[0].txt";

my $cmd = "gcc -g -o $out $flags $str_files $main > $dump 2>&1";

print "$cmd\n";
system $cmd;

open my $f, "<", $dump;
my $line_count = 0;
while (<$f>) {
    print "$_";
    last if ($line_count++ > 40);
}

system "$out" if $line_count == 0;
