#!/usr/bin/env perl

use warnings;
use strict;

use Data::Dumper;

# generates test-db.c

use File::Basename;

sub demangle;
sub funcname;

# scan input files for test group info
my @groups;
my @test_files = @ARGV;

die "no test files" if not scalar @test_files;

foreach my $file (@test_files) {
    my ($group_file, $group_dir) = fileparse $file, qr/\.[^\.]*/;
    my $group_base = basename $group_dir;

    my @test_funcs = map {
        funcname $_, "void";
    } qx{ grep -hE '^void test_[A-Za-z0-9_]* *\\( *void *\\*\\*state\\)' $file };

    my @tests = map {
        { name => demangle($_), func => $_ };
    } @test_funcs;

    my $group_pretty = "${group_base} ${group_file}";
    my $group_prefix = "${group_base}_${group_file}";

    my ($group_init) = map {
        funcname $_, "int";
    } qx{ grep -hE '^int ${group_prefix}_group_init *\\( *void *\\*\\*state\\)' $file };

    my ($group_cleanup) = map {
        funcname $_, "int";
    } qx{ grep -hE '^int ${group_prefix}_group_cleanup *\\( *void *\\*\\*state\\)' $file };

    my ($group_setup) = map {
        funcname $_, "void";
    } qx{ grep -hE '^int ${group_prefix}_test_setup *\\( *void *\\*\\*state\\)' $file };

    my ($group_teardown) = map {
        funcname $_, "void";
    } qx{ grep -hE '^int ${group_prefix}_test_teardown *\\( *void *\\*\\*state\\)' $file };

    push @groups, {
        name => $group_pretty,
        prefix => $group_prefix,
        init => $group_init,
        cleanup => $group_cleanup,
        setup => $group_setup,
        teardown => $group_teardown,
        tests => \@tests,
    };
}

# generate c file
print '#include "run.h"', "\n\n";

# pre-declare the init/cleanup/test functions
foreach my $group (@groups) {
    next if not scalar @{$group->{tests}};

    print "int $group->{init}(void **state);\n" if defined $group->{init};
    print "int $group->{cleanup}(void **state);\n" if defined $group->{init};
    print "void $group->{setup}(void **state);\n" if defined $group->{setup};
    print "void $group->{teardown}(void **state);\n" if defined $group->{teardown};
    foreach my $test (@{$group->{tests}}) {
        print "void $test->{func}(void **state);\n";
    }
    print "\n";
}

# the tests
foreach my $group (@groups) {
    next if not scalar @{$group->{tests}};

    print "const goat_test_group_t $group->{prefix}_group = {\n";
    print qq{\t"$group->{name}",\n};
    print $group->{init} ? qq{\t$group->{init},\n} : qq{\tNULL,\n};
    print $group->{cleanup} ? qq{\t$group->{cleanup},\n} : qq{\tNULL,\n};
    print qq{\t} . scalar @{$group->{tests}} . qq{,\n};
    print "\t{\n";

    foreach my $test(@{$group->{tests}}) {
        print "\t\t{ ";
        print qq{"$test->{name}", };
        print qq{$test->{func}, };
        print $group->{setup} ? qq{$group->{setup}, } : q{NULL, };
        print $group->{teardown} ? qq{$group->{teardown}, } : q{NULL, };
        print "},\n";
    }
    print "\t}\n";
    print "};\n\n";
}

# the groups
#const goat_test_group_t *test_groups[] = {
#    &foo,
#    NULL
#};
print 'const goat_test_group_t *test_groups[] = {', "\n";

foreach my $group (@groups) {
    next if not scalar @{$group->{tests}};

    print "\t&$group->{prefix}_group,\n";
}

print "\tNULL,\n";
print '};', "\n";

##############

sub demangle {
    my ($str) = @_;

    # throw away 'test_' prefix
    $str =~ s/^test_//;

    # replace single underscores with spaces
    $str =~ s/(?<!_)_(?!_)/ /g;

    # replace double underscores with singles
    $str =~ s/(?<!_)__(?!_)/_/g;

    # replace triple underscores with spaced hyphen
    $str =~ s/(?<!_)___(?!_)/ - /g;

    return $str;
}

sub funcname {
    my ($str, $rtype) = @_;

    chomp $str;
    $str =~ s/^\s*\Q$rtype\E\s*//;
    $str =~ s/\s*\([^\)]*\)(?:\s*\{\s*)?$//;

    return $str;
}

