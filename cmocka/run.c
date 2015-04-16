#include "run.h"

extern const goat_test_group_t *test_groups[];

int main (int argc __attribute__((unused)), char **argv __attribute__((unused))) {
    int errors = 0;

    cmocka_set_message_output(CM_OUTPUT_TAP);

    for (size_t g = 0; test_groups[g] != NULL; g++) {
        const goat_test_group_t *group = test_groups[g];

        errors += _cmocka_run_group_tests(
            group->name,
            group->tests,
            group->n_tests,
            group->setup,
            group->teardown
        );
    }

    return errors;
}
