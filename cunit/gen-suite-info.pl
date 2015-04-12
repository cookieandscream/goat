#!/usr/bin/env perl

use warnings;
use strict;

use Data::Dumper;

# generates suite-info.c

use File::Basename;

sub demangle;
sub funcname;

# scan input files for test suite info
my @suites;
my @test_files = @ARGV;

die "no test files" if not scalar @test_files;

foreach my $file (@test_files) {
    my ($suite_file, $suite_dir) = fileparse $file, qr/\.[^\.]*/;
    my $suite_base = basename $suite_dir;

    my @test_funcs = map {
        funcname $_, "void";
    } qx{ grep -hE '^void test_[A-Za-z0-9_]* *\\( *void *\\)' $file };

    my @tests = map {
        { name => demangle($_), func => $_ };
    } @test_funcs;

    my $suite_pretty = "${suite_base} ${suite_file}";
    my $suite_prefix = "${suite_base}_${suite_file}";

    my ($suite_init) = map {
        funcname $_, "int";
    } qx{ grep -hE '^int ${suite_prefix}_suite_init *\\( *void *\\)' $file };

    my ($suite_cleanup) = map {
        funcname $_, "int";
    } qx{ grep -hE '^int ${suite_prefix}_suite_cleanup *\\( *void *\\)' $file };

    my ($suite_setup) = map {
        funcname $_, "void";
    } qx{ grep -hE '^void ${suite_prefix}_test_setup *\\( *void *\\)' $file };

    my ($suite_teardown) = map {
        funcname $_, "void";
    } qx{ grep -hE '^void ${suite_prefix}_test_teardown *\\( *void *\\)' $file };

    push @suites, {
        name => $suite_pretty,
        prefix => $suite_prefix,
        init => $suite_init,
        cleanup => $suite_cleanup,
        setup => $suite_setup,
        teardown => $suite_teardown,
        tests => \@tests,
    };
}

# generate c file
print "#include <CUnit/CUnit.h>\n";
print '#include <CUnit/TestDB.h>', "\n\n";

# pre-declare the init/cleanup/test functions
foreach my $suite (@suites) {
    print "int $suite->{init}(void);\n" if defined $suite->{init};
    print "int $suite->{cleanup}(void);\n" if defined $suite->{init};
    print "void $suite->{setup}(void);\n" if defined $suite->{setup};
    print "void $suite->{teardown}(void);\n" if defined $suite->{teardown};
    foreach my $test (@{$suite->{tests}}) {
        print "void $test->{func}(void);\n";
    }
    print "\n";
}

# the tests
foreach my $suite (@suites) {
    print "CU_TestInfo $suite->{prefix}_tests[] = {\n";

    foreach my $test(@{$suite->{tests}}) {
        print "\t{ ";
        print qq{"$test->{name}", };
        print qq{$test->{func} };
        print "},\n";
    }
    print "\tCU_TEST_INFO_NULL,\n";
    print "};\n\n";
}

# the suites
print 'CU_SuiteInfo cunit_suite_info[] = {', "\n";

foreach my $suite (@suites) {
    next if not scalar @{$suite->{tests}};

    print "\t{ ";
    print qq{"$suite->{name}", };
    print $suite->{init} ? qq{$suite->{init}, } : q{NULL, };
    print $suite->{cleanup} ? qq{$suite->{cleanup}, } : q{NULL, };
    print $suite->{setup} ? qq{$suite->{setup}, } : q{NULL, };
    print $suite->{teardown} ? qq{$suite->{teardown}, } : q{NULL, };
    print qq{$suite->{prefix}_tests, };
    print "},\n";
}

print "\tCU_SUITE_INFO_NULL,\n";
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
