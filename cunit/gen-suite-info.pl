#!/usr/bin/env perl

use warnings;
use strict;

use Data::Dumper;

# generates suite-info.c

use File::Basename;
my $dirname = dirname __FILE__;

# find the suite directories
opendir my $dh, $dirname or die "opendir $dirname: $!";
my @suite_dirs = map { "$dirname/$_" } grep { -d "$dirname/$_" && m/^[^\.]/ } readdir $dh;
closedir $dh;

# find information about the suites
my @suites;
foreach my $dir (@suite_dirs) {
    my $suite_name = basename $dir;

    my @test_funcs = map {
        chomp;
        s/^\s*void\s*//;
        s/\(\s*void\s*\)(?:\s*\{)?$//;
        $_;
    } qx{ grep -hE '^void test_[A-Za-z0-9_]* *\\( *void *\\)' $dir/*.c };

    my @tests = map {
        my $func = $_;
        my ($name) = m/^test_(.*)$/;
        { name => $name, func => $func };
    } @test_funcs;

    my $suite_init = "${suite_name}_suite_init";
    my $suite_cleanup = "${suite_name}_suite_cleanup";

    push @suites, {
        name => $suite_name,
        init => $suite_init,
        cleanup => $suite_cleanup,
        tests => \@tests,
    };
}

# generate c file
print "#include <CUnit/CUnit.h>\n";
print '#include <CUnit/TestDB.h>', "\n\n";

# pre-declare the init/cleanup/test functions
foreach my $suite (@suites) {
    print "int $suite->{init}(void);\n";
    print "int $suite->{cleanup}(void);\n";
    foreach my $test (@{$suite->{tests}}) {
        print "void $test->{func}(void);\n";
    }
    print "\n";
}

# the tests
foreach my $suite (@suites) {
    print "CU_TestInfo $suite->{name}_tests[] = {\n";

    foreach my $test(@{$suite->{tests}}) {
        print "\t{ ";
        print qq{"$test->{name}", };
        print qq{$test->{func} };
        print "},\n";
    }
    print "\tCU_TEST_INFO_NULL,\n";
    print "};\n\n";
}

# the suites themselves
print 'CU_SuiteInfo cunit_suite_info[] = {', "\n";

foreach my $suite (@suites) {
    next if not scalar @{$suite->{tests}};

    print "\t{ ";
    print qq{"$suite->{name}", };
    print qq{$suite->{init}, };
    print qq{$suite->{cleanup}, };
    print qq{NULL, NULL, };
    print qq{$suite->{name}_tests, };
    print "},\n";
}

print "\tCU_SUITE_INFO_NULL,\n";
print '};', "\n";
