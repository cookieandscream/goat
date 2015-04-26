#ifndef GOAT_CMOCKA_MAIN_C
#define GOAT_CMOCKA_MAIN_C

#ifndef group_name
#define group_name "unnamed test group"
#endif

int main (int argc __attribute__((unused)), char **argv __attribute__((unused))) {
    cmocka_set_message_output(CM_OUTPUT_TAP);

    const TestGroup *group = &test_group;

    printf("1..1\n");
    int r = _cmocka_run_group_tests(
        group->name,
        group->tests,
        group->n_tests,
        group->setup,
        group->teardown
    );

    if (r < 0) {
        fprintf(stderr, "ERROR: _cmocka_run_group_tests (%s) failed: %i\n", group->name, r);
    }

    return r;
}

#endif
