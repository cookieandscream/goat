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

    my %setup_funcs = map {
        ( demangle($_), funcname($_, "int") );
    } qx{ grep -hE '^void setup_[A-Za-z0-9_]* *\\( *void *\\*\\*state\\)' $file };

    my %teardown_funcs = map {
        ( demangle($_), funcname($_, "int") );
    } qx{ grep -hE '^void teardown_[A-Za-z0-9_]* *\\( *void *\\*\\*state\\)' $file };

    my @tests = map {
        { name => demangle($_), func => $_ };
    } @test_funcs;

    my $group_pretty = "${group_base} ${group_file}";
    my $group_prefix = "${group_base}_${group_file}";

    my ($group_setup) = map {
        funcname $_, "int";
    } qx{ grep -hE '^int ${group_prefix}_group_setup *\\( *void *\\*\\*state\\)' $file };

    my ($group_teardown) = map {
        funcname $_, "int";
    } qx{ grep -hE '^int ${group_prefix}_group_teardown *\\( *void *\\*\\*state\\)' $file };

    my ($test_setup) = map {
        funcname $_, "int";
    } qx{ grep -hE '^int ${group_prefix}_test_setup *\\( *void *\\*\\*state\\)' $file };

    my ($test_teardown) = map {
        funcname $_, "int";
    } qx{ grep -hE '^int ${group_prefix}_test_teardown *\\( *void *\\*\\*state\\)' $file };

    push @groups, {
        name => $group_pretty,
        prefix => $group_prefix,
        group_setup => $group_setup,
        group_teardown => $group_teardown,
        test_setup => $test_setup,
        test_teardown => $test_teardown,
        setup_funcs => \%setup_funcs,
        teardown_funcs => \%teardown_funcs,
        tests => \@tests,
    };
}

# generate c file
print '#include "run.h"', "\n\n";

# pre-declare the init/cleanup/test functions
foreach my $group (@groups) {
    next if not scalar @{$group->{tests}};

    print "int $group->{group_setup}(void **state);\n" if defined $group->{group_setup};
    print "int $group->{group_teardown}(void **state);\n" if defined $group->{group_teardown};
    print "int $group->{test_setup}(void **state);\n" if defined $group->{test_setup};
    print "int $group->{test_teardown}(void **state);\n" if defined $group->{test_teardown};
    foreach my $test (@{$group->{tests}}) {
        print "void $test->{func}(void **state);\n";
    }
    foreach my $fixture (values %{$group->{setup_funcs}}, values %{$group->{teardown_funcs}}) {
        print "int $fixture(void **state);\n";
    }
    print "\n";
}

# the tests
foreach my $group (@groups) {
    next if not scalar @{$group->{tests}};

    print "const goat_test_group_t $group->{prefix}_group = {\n";
    print qq{\t"$group->{name}",\n};
    print $group->{group_setup} ? qq{\t$group->{group_setup},\n} : qq{\tNULL,\n};
    print $group->{group_teardown} ? qq{\t$group->{group_teardown},\n} : qq{\tNULL,\n};
    print qq{\t} . scalar @{$group->{tests}} . qq{,\n};
    print "\t{\n";

    foreach my $test(@{$group->{tests}}) {
        print "\t\t{ ";
        print qq{"$test->{name}", };
        print qq{$test->{func}, };

        if ($group->{setup_funcs}->{$group->{name}}) {
            print qq{$group->{setup_funcs}->{$group->{name}}, };
        }
        elsif ($group->{test_setup}) {
            print qq{$group->{test_setup}, };
        }
        else {
            print q{NULL, };
        }

        if ($group->{teardown_funcs}->{$group->{name}}) {
            print qq{$group->{teardown_funcs}->{$group->{name}}, };
        }
        elsif ($group->{test_teardown}) {
            print qq{$group->{test_teardown}, };
        }
        else {
            print q{NULL, };
        }

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

