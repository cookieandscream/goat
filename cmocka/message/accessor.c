#include <errno.h>
#include <stdlib.h>

#include "cmocka/run.h"

#include "src/goat.h"
#include "src/message.h"
#include "src/util.h"

struct objs {
    GoatMessage *msg;
    GoatMessage *msg_w_prefix;
    GoatMessage *msg_w_params;
    GoatMessage *msg_w_prefix_params;
};

int message_accessor_group_setup(void **state) {
    const char *prefix = "prefix";
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param3", NULL };

    struct objs *objs = calloc(1, sizeof(*objs));
    if (NULL == objs) return -1;

    objs->msg = goat_message_new(NULL, command, NULL);
    if (NULL == objs->msg) goto cleanup;

    objs->msg_w_prefix = goat_message_new(prefix, command, NULL);
    if (NULL == objs->msg) goto cleanup;

    objs->msg_w_params = goat_message_new(NULL, command, params);
    if (NULL == objs->msg) goto cleanup;

    objs->msg_w_prefix_params = goat_message_new(prefix, command, params);
    if (NULL == objs->msg) goto cleanup;

    *state = objs;
    return 0;

cleanup:
    if (objs->msg) goat_message_delete(objs->msg);
    if (objs->msg_w_prefix) goat_message_delete(objs->msg_w_prefix);
    if (objs->msg_w_params) goat_message_delete(objs->msg_w_params);
    if (objs->msg_w_prefix_params) goat_message_delete(objs->msg_w_prefix_params);
    free(objs);
    return -1;
}

int message_accessor_group_teardown(void **state) {
    struct objs *objs = * (struct objs **) state;
    *state = NULL;
    goat_message_delete(objs->msg);
    goat_message_delete(objs->msg_w_prefix);
    goat_message_delete(objs->msg_w_params);
    goat_message_delete(objs->msg_w_prefix_params);
    free(objs);
    return 0;
}

void test_goat__message__get__prefix___without_message(void **state) {
    ARG_UNUSED(state);
    const char *str = goat_message_get_prefix(NULL);

    assert_null(str);
}

void test_goat__message__get__prefix___without_prefix(void **state) {
    struct objs *objs = * (struct objs **) state;
    const char *str = goat_message_get_prefix(objs->msg);

    assert_null(str);
}

void test_goat__message__get__prefix___with_prefix(void **state) {
    struct objs *objs = * (struct objs **) state;
    const char *str = goat_message_get_prefix(objs->msg_w_prefix);

    assert_non_null(str);
    assert_string_equal(str, "prefix");
}

void test_goat__message__get__command__string___without_message(void **state) {
    ARG_UNUSED(state);
    const char *str = goat_message_get_command_string(NULL);

    assert_null(str);
}

void test_goat__message__get__command__string___with_unrecognised_command(void **state) {
    struct objs *objs = * (struct objs **) state;
    const char *str = goat_message_get_command_string(objs->msg);

    assert_non_null(str);
    assert_string_equal(str, "command");
}

void test_goat__message__get__command__string___with_recognised_command(void **state) {
    ARG_UNUSED(state);
    const char *command = "PRIVMSG";

    GoatMessage *message = goat_message_new(NULL, command, NULL);
    assert_non_null(message);

    const char *str = goat_message_get_command_string(message);

    assert_non_null(str);
    assert_string_equal(str, "PRIVMSG");
    assert_int_equal(str, goat_command_string(GOAT_IRC_PRIVMSG));

    goat_message_delete(message);
}

void test_goat__message__get__param___without_message(void **state) {
    ARG_UNUSED(state);
    const char *str = goat_message_get_param(NULL, 0);

    assert_null(str);
}

void test_goat__message__get__param___without_params(void **state) {
    struct objs *objs = * (struct objs **) state;
    const char *str = goat_message_get_param(objs->msg, 0);

    assert_null(str);
}

void test_goat__message__get__param___with_excessive_size(void **state) {
    struct objs *objs = * (struct objs **) state;
    const char *str = goat_message_get_param(objs->msg_w_params, 16);

    assert_null(str);
}

void test_goat__message__get__param___with_params(void **state) {
    struct objs *objs = * (struct objs **) state;
    const char *expect_params[] = { "param1", "param2", "param3", NULL };
    size_t expect_nparams = 3;

    for (size_t i = 0; i < 16; i++) {
        const char *str = goat_message_get_param(objs->msg_w_params, i);

        if (i < expect_nparams) {
            assert_non_null(str);
            assert_string_equal(str, expect_params[i]);
        }
        else {
            assert_null(str);
        }
    }
}

void test_goat__message__get__nparams___without_message(void **state) {
    ARG_UNUSED(state);
    size_t n = goat_message_get_nparams(NULL);

    assert_int_equal(n, 0);
}

void test_goat__message__get__nparams___with_params(void **state) {
    struct objs *objs = * (struct objs **) state;
    size_t n = goat_message_get_nparams(objs->msg_w_params);

    assert_int_equal(n, 3);
}

void test_goat__message__get__command___without_message(void **state) {
    ARG_UNUSED(state);
    GoatError r = goat_message_get_command(NULL, NULL);

    assert_int_equal(r, EINVAL);
}

void test_goat__message__get__command___without_command(void **state) {
    struct objs *objs = * (struct objs **) state;
    GoatError r = goat_message_get_command(objs->msg, NULL);

    assert_int_equal(r, EINVAL);
}

void test_goat__message__get__command___with_unrecognised_command(void **state) {
    struct objs *objs = * (struct objs **) state;
    GoatCommand cmd = GOAT_IRC_LAST;

    GoatError r = goat_message_get_command(objs->msg, &cmd);

    assert_int_equal(r, GOAT_E_UNREC);
    assert_int_equal(cmd, GOAT_IRC_LAST);
}

void test_goat__message__get__command___with_recognised_command(void **state) {
    ARG_UNUSED(state);
    GoatMessage *message = goat_message_new(NULL, "PRIVMSG", NULL);
    assert_non_null(message);

    GoatCommand cmd = GOAT_IRC_LAST;
    GoatError r = goat_message_get_command(message, &cmd);

    assert_int_equal(r, GOAT_E_NONE);
    assert_int_equal(cmd, GOAT_IRC_PRIVMSG);

    goat_message_delete(message);
}
