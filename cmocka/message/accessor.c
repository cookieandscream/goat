#include "cmocka/run.h"

#include "src/goat.h"
#include "src/message.h"

goat_message_t *msg;
goat_message_t *msg_w_prefix;
goat_message_t *msg_w_params;
goat_message_t *msg_w_prefix_params;

int message_accessor_group_init(void **state) {
    const char *prefix = "prefix";
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param3", NULL };

    msg = goat_message_new(NULL, command, NULL);
    msg_w_prefix = goat_message_new(prefix, command, NULL);
    msg_w_params = goat_message_new(NULL, command, params);
    msg_w_prefix_params = goat_message_new(prefix, command, params);

    if (msg && msg_w_prefix && msg_w_params && msg_w_prefix_params) return 0;

    if (msg) goat_message_delete(msg);
    if (msg_w_prefix) goat_message_delete(msg_w_prefix);
    if (msg_w_params) goat_message_delete(msg_w_params);
    if (msg_w_prefix_params) goat_message_delete(msg_w_prefix_params);
    return -1;
}

int message_accessor_group_cleanup(void **state) {
    goat_message_delete(msg);
    goat_message_delete(msg_w_prefix);
    goat_message_delete(msg_w_params);
    goat_message_delete(msg_w_prefix_params);
    return 0;
}

void test_goat__message__get__prefix___without_message(void **state) {
    const char *str = goat_message_get_prefix(NULL);

    assert_null(str);
}

void test_goat__message__get__prefix___without_prefix(void **state) {
    const char *str = goat_message_get_prefix(msg);

    assert_null(str);
}

void test_goat__message__get__prefix___with_prefix(void **state) {
    const char *str = goat_message_get_prefix(msg_w_prefix);

    assert_non_null(str);
    assert_string_equal(str, "prefix");
}

void test_goat__message__get__command__string___without_message(void **state) {
    const char *str = goat_message_get_command_string(NULL);

    assert_null(str);
}

void test_goat__message__get__command__string___with_unrecognised_command(void **state) {
    const char *str = goat_message_get_command_string(msg);

    assert_non_null(str);
    assert_string_equal(str, "command");
}

void test_goat__message__get__command__string___with_recognised_command(void **state) {
    const char *command = "PRIVMSG";

    goat_message_t *message = goat_message_new(NULL, command, NULL);
    assert_non_null(message);

    const char *str = goat_message_get_command_string(message);

    assert_non_null(str);
    assert_string_equal(str, "PRIVMSG");
    assert_int_equal(str, goat_command_string(GOAT_IRC_PRIVMSG));

    goat_message_delete(message);
}

void test_goat__message__get__param___without_message(void **state) {
    const char *str = goat_message_get_param(NULL, 0);

    assert_null(str);
}

void test_goat__message__get__param___without_params(void **state) {
    const char *str = goat_message_get_param(msg, 0);

    assert_null(str);
}

void test_goat__message__get__param___with_excessive_size(void **state) {
    const char *str = goat_message_get_param(msg_w_params, 16);

    assert_null(str);
}

void test_goat__message__get__param___with_params(void **state) {
    const char *expect_params[] = { "param1", "param2", "param3", NULL };
    size_t expect_nparams = 3;

    for (size_t i = 0; i < 16; i++) {
        const char *str = goat_message_get_param(msg_w_params, i);

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
    size_t n = goat_message_get_nparams(NULL);

    assert_int_equal(n, 0);
}

void test_goat__message__get__nparams___with_params(void **state) {
    size_t n = goat_message_get_nparams(msg_w_params);

    assert_int_equal(n, 3);
}

void test_goat__message__get__command___without_message(void **state) {
    int r = goat_message_get_command(NULL, NULL);

    assert_int_equal(r, -1);
}

void test_goat__message__get__command___without_command(void **state) {
    int r = goat_message_get_command(msg, NULL);

    assert_int_equal(r, -1);
}

void test_goat__message__get__command___with_unrecognised_command(void **state) {
    goat_command_t cmd = GOAT_IRC_LAST;

    int r = goat_message_get_command(msg, &cmd);

    assert_int_equal(r, -1);
    assert_int_equal(cmd, GOAT_IRC_LAST);
}

void test_goat__message__get__command___with_recognised_command(void **state) {
    goat_message_t *message = goat_message_new(NULL, "PRIVMSG", NULL);
    assert_non_null(message);

    goat_command_t cmd = GOAT_IRC_LAST;
    int r = goat_message_get_command(message, &cmd);

    assert_int_equal(r, 0);
    assert_int_equal(cmd, GOAT_IRC_PRIVMSG);

    goat_message_delete(message);
}
