#include <stdlib.h>
#include <string.h>

#include "cmocka/run.h"

#include "src/goat.h"
#include "src/message.h"
#include "src/util.h"

void test_goat__message__cstring___without_message(void **state) {
    ARG_UNUSED(state);
    char *str = goat_message_cstring(NULL, NULL, NULL);

    assert_null(str);
}

void test_goat__message__cstring___without_buf(void **state) {
    ARG_UNUSED(state);
    const char *prefix = "prefix";
    const char *command = "command";
    const char *params[] = { "p1", "p2", "p3", NULL };

    GoatMessage *message = goat_message_new(prefix, command, params);
    assert_non_null(message);

    char *str = goat_message_cstring(message, NULL, NULL);
    assert_null(str);

    goat_message_delete(message);
}

void test_goat__message__cstring___without_size(void **state) {
    ARG_UNUSED(state);
    const char *prefix = "prefix";
    const char *command = "command";
    const char *params[] = { "p1", "p2", "p3", NULL };

    GoatMessage *message = goat_message_new(prefix, command, params);
    assert_non_null(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    char *str = goat_message_cstring(message, buf, NULL);
    assert_null(str);

    goat_message_delete(message);
}

void test_goat__message__cstring___without_prefix(void **state) {
    ARG_UNUSED(state);
    const char *command = "command";

    GoatMessage *message = goat_message_new(NULL, command, NULL);
    assert_non_null(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    assert_non_null(str);
    assert_int_equal(str, buf);
    assert_string_equal(str, "command");
    assert_int_equal(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__cstring___with_prefix(void **state) {
    ARG_UNUSED(state);
    const char *prefix = "prefix";
    const char *command = "command";

    GoatMessage *message = goat_message_new(prefix, command, NULL);
    assert_non_null(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    assert_non_null(str);
    assert_int_equal(str, buf);
    assert_string_equal(str, ":prefix command");
    assert_int_equal(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__cstring___without_params(void **state) {
    ARG_UNUSED(state);
    const char *command = "command";

    GoatMessage *message = goat_message_new(NULL, command, NULL);
    assert_non_null(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    assert_non_null(str);
    assert_int_equal(str, buf);
    assert_string_equal(str, "command");
    assert_int_equal(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__cstring___with_params(void **state) {
    ARG_UNUSED(state);
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param3", NULL };

    GoatMessage *message = goat_message_new(NULL, command, params);
    assert_non_null(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    assert_non_null(str);
    assert_int_equal(str, buf);
    assert_string_equal(str, "command param1 param2 :param3");
    assert_int_equal(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__cstring___with_space_param(void **state) {
    ARG_UNUSED(state);
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param 3 with spaces", NULL };

    GoatMessage *message = goat_message_new(NULL, command, params);
    assert_non_null(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    assert_non_null(str);
    assert_int_equal(str, buf);
    assert_string_equal(str, "command param1 param2 :param 3 with spaces");
    assert_int_equal(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__cstring___with_tags(void **state) {
    ARG_UNUSED(state);
    skip(); // FIXME not implemented yet
}

void test_goat__message__cstring___with_the_works(void **state) {
    ARG_UNUSED(state);
    const char *prefix = "anne";
    const char *command = "PRIVMSG";
    const char *params[] = { "#goat", "hello there", NULL };

    GoatMessage *message = goat_message_new(prefix, command, params);
    assert_non_null(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    assert_non_null(str);
    assert_int_equal(str, buf);
    assert_string_equal(str, ":anne PRIVMSG #goat :hello there");
    assert_int_equal(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__strdup___without_message(void **state) {
    ARG_UNUSED(state);
    char *str = goat_message_strdup(NULL);

    assert_null(str);
}

void test_goat__message__strdup___with_the_works(void **state) {
    ARG_UNUSED(state);
    const char *prefix = "anne";
    const char *command = "PRIVMSG";
    const char *params[] = { "#goat", "hello there", NULL };

    GoatMessage *message = goat_message_new(prefix, command, params);
    assert_non_null(message);

    char *str = goat_message_strdup(message);

    assert_non_null(str);
    assert_string_equal(str, ":anne PRIVMSG #goat :hello there");

    free(str);
}
