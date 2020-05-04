#!/usr/bin/perl -w

use strict;

my @pkgs = ("vulkan", "glfw3", "cglm", "check");
my $flags = "";
for (@pkgs) {
    my $pflags = qx(pkg-config --cflags --libs $_);
    chomp $pflags;
    $flags .= " $pflags";
}

my @files;
push @files, <src/*.c>;
push @files, <tests-src/*.c>;

my $str_files = join " ", @files;

my $cmd = "gcc -g -o bin/runner $flags $str_files test-runner/runner.c > out-run.txt 2>&1";

print "$cmd\n";
system $cmd;

open my $f, "<", "out-run.txt";
my $line_count = 0;
while (<$f>) {
    print "$_";
    last if ($line_count++ > 40);
}

system "bin/runner" if $line_count == 0;
