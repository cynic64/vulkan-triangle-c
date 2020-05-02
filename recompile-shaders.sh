#!/usr/bin/perl -w

use strict;
use File::Find::Rule;

my @files = File::Find::Rule->file()
                            ->name(qr/.*\.(vert|frag)$/)
                            ->in(".");

for (@files) {
    my $cmd = "glslc -o $_.spv $_";
    print "$cmd\n";
    system $cmd;
}
