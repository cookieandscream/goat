#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "src/goat.h"
#include "src/message.h"

goat_message_t *msg;
goat_message_t *msg_w_prefix;
goat_message_t *msg_w_params;
goat_message_t *msg_w_prefix_params;

int message_accessor_suite_init(void) {
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

int message_accessor_suite_cleanup(void) {
    goat_message_delete(msg);
    goat_message_delete(msg_w_prefix);
    goat_message_delete(msg_w_params);
    goat_message_delete(msg_w_prefix_params);
    return 0;
}

void test_goat__message__get__prefix___without_message(void) {
    const char *str = goat_message_get_prefix(NULL);

    CU_ASSERT_PTR_NULL(str);
}

void test_goat__message__get__prefix___without_prefix(void) {
    const char *str = goat_message_get_prefix(msg);

    CU_ASSERT_PTR_NULL(str);
}

void test_goat__message__get__prefix___with_prefix(void) {
    const char *str = goat_message_get_prefix(msg_w_prefix);

    CU_ASSERT_PTR_NOT_NULL_FATAL(str);
    CU_ASSERT_STRING_EQUAL(str, "prefix");
}

void test_goat__message__get__command__string___without_message(void) {
    const char *str = goat_message_get_command_string(NULL);

    CU_ASSERT_PTR_NULL(str);
}

void test_goat__message__get__command__string___with_unrecognised_command(void) {
    const char *str = goat_message_get_command_string(msg);

    CU_ASSERT_PTR_NOT_NULL_FATAL(str);
    CU_ASSERT_STRING_EQUAL(str, "command");
}

void test_goat__message__get__command__string___with_recognised_command(void) {
    const char *command = "PRIVMSG";

    goat_message_t *message = goat_message_new(NULL, command, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    const char *str = goat_message_get_command_string(message);

    CU_ASSERT_PTR_NOT_NULL_FATAL(str);
    CU_ASSERT_STRING_EQUAL(str, "PRIVMSG");
    CU_ASSERT_PTR_EQUAL(str, goat_command_string(GOAT_IRC_PRIVMSG));

    goat_message_delete(message);
}

void test_goat__message__get__param___without_message(void) {
    const char *str = goat_message_get_param(NULL, 0);

    CU_ASSERT_PTR_NULL(str);
}

void test_goat__message__get__param___without_params(void) {
    const char *str = goat_message_get_param(msg, 0);

    CU_ASSERT_PTR_NULL(str);
}

void test_goat__message__get__param___with_excessive_size(void) {
    const char *str = goat_message_get_param(msg_w_params, 16);

    CU_ASSERT_PTR_NULL(str);
}

void test_goat__message__get__param___with_params(void) {
    const char *expect_params[] = { "param1", "param2", "param3", NULL };
    size_t expect_nparams = 3;

    for (size_t i = 0; i < 16; i++) {
        const char *str = goat_message_get_param(msg_w_params, i);

        if (i < expect_nparams) {
            CU_ASSERT_PTR_NOT_NULL_FATAL(str);
            CU_ASSERT_STRING_EQUAL(str, expect_params[i]);
        }
        else {
            CU_ASSERT_PTR_NULL(str);
        }
    }
}

void test_goat__message__get__nparams___without_message(void) {
    size_t n = goat_message_get_nparams(NULL);

    CU_ASSERT_EQUAL(n, 0);
}

void test_goat__message__get__nparams___with_params(void) {
    size_t n = goat_message_get_nparams(msg_w_params);

    CU_ASSERT_EQUAL(n, 3);
}

void test_goat__message__get__command___without_message(void) {
    int r = goat_message_get_command(NULL, NULL);

    CU_ASSERT_EQUAL(r, -1);
}

void test_goat__message__get__command___without_command(void) {
    int r = goat_message_get_command(msg, NULL);

    CU_ASSERT_EQUAL(r, -1);
}

void test_goat__message__get__command___with_unrecognised_command(void) {
    goat_command_t cmd = GOAT_IRC_LAST;

    int r = goat_message_get_command(msg, &cmd);

    CU_ASSERT_EQUAL(r, -1);
    CU_ASSERT_EQUAL(cmd, GOAT_IRC_LAST);
}

void test_goat__message__get__command___with_recognised_command(void) {
    goat_message_t *message = goat_message_new(NULL, "PRIVMSG", NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    goat_command_t cmd = GOAT_IRC_LAST;
    int r = goat_message_get_command(message, &cmd);

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_EQUAL(cmd, GOAT_IRC_PRIVMSG);

    goat_message_delete(message);
}
