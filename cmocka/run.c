#include <stdio.h>

#include "run.h"

extern const TestGroup *test_groups[];

int main (int argc __attribute__((unused)), char **argv __attribute__((unused))) {
    int total_errors = 0;
    int num_groups = 0;

    const TestGroup **p = &test_groups[0];
    while (*(p++)) num_groups++;

    cmocka_set_message_output(CM_OUTPUT_TAP);

    printf("1..%i\n", num_groups);
    for (int i = 0; i < num_groups; i++) {
        const TestGroup *group = test_groups[i];

        int ret = _cmocka_run_group_tests(
            group->name,
            group->tests,
            group->n_tests,
            group->setup,
            group->teardown
        );

        if (ret >= 0) {
            total_errors += ret;
        }
        else {
            fprintf(stderr, "ERROR: _cmocka_run_group_tests (%s) failed: %i\n", group->name, ret);
        }
    }

    return total_errors;
}
