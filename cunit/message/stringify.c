#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <stdlib.h>

#include "src/goat.h"
#include "src/message.h"

void test_goat__message__cstring___without_message(void) {
    char *str = goat_message_cstring(NULL, NULL, NULL);

    CU_ASSERT_PTR_NULL(str);
}

void test_goat__message__cstring___without_buf(void) {
    const char *prefix = "prefix";
    const char *command = "command";
    const char *params[] = { "p1", "p2", "p3", NULL };

    goat_message_t *message = goat_message_new(prefix, command, params);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    char *str = goat_message_cstring(message, NULL, NULL);
    CU_ASSERT_PTR_NULL(str);

    goat_message_delete(message);
}

void test_goat__message__cstring___without_size(void) {
    const char *prefix = "prefix";
    const char *command = "command";
    const char *params[] = { "p1", "p2", "p3", NULL };

    goat_message_t *message = goat_message_new(prefix, command, params);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    char *str = goat_message_cstring(message, buf, NULL);
    CU_ASSERT_PTR_NULL(str);

    goat_message_delete(message);
}

void test_goat__message__cstring___without_prefix(void) {
    const char *command = "command";

    goat_message_t *message = goat_message_new(NULL, command, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_PTR_EQUAL(str, buf);
    CU_ASSERT_STRING_EQUAL(str, "command");
    CU_ASSERT_EQUAL(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__cstring___with_prefix(void) {
    const char *prefix = "prefix";
    const char *command = "command";

    goat_message_t *message = goat_message_new(prefix, command, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_PTR_EQUAL(str, buf);
    CU_ASSERT_STRING_EQUAL(str, ":prefix command");
    CU_ASSERT_EQUAL(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__cstring___without_params(void) {
    const char *command = "command";

    goat_message_t *message = goat_message_new(NULL, command, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_PTR_EQUAL(str, buf);
    CU_ASSERT_STRING_EQUAL(str, "command");
    CU_ASSERT_EQUAL(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__cstring___with_params(void) {
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param3", NULL };

    goat_message_t *message = goat_message_new(NULL, command, params);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_PTR_EQUAL(str, buf);
    CU_ASSERT_STRING_EQUAL(str, "command param1 param2 :param3");
    CU_ASSERT_EQUAL(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__cstring___with_space_param(void) {
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param 3 with spaces", NULL };

    goat_message_t *message = goat_message_new(NULL, command, params);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_PTR_EQUAL(str, buf);
    CU_ASSERT_STRING_EQUAL(str, "command param1 param2 :param 3 with spaces");
    CU_ASSERT_EQUAL(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__cstring___with_the_works(void) {
    const char *prefix = "anne";
    const char *command = "PRIVMSG";
    const char *params[] = { "#goat", "hello there", NULL };

    goat_message_t *message = goat_message_new(prefix, command, params);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    char buf[GOAT_MESSAGE_BUF_SZ];
    size_t sz = sizeof(buf);
    char *str = goat_message_cstring(message, buf, &sz);

    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_PTR_EQUAL(str, buf);
    CU_ASSERT_STRING_EQUAL(str, ":anne PRIVMSG #goat :hello there");
    CU_ASSERT_EQUAL(sz, strlen(str));

    goat_message_delete(message);
}

void test_goat__message__strdup___without_message(void) {
    char *str = goat_message_strdup(NULL);

    CU_ASSERT_PTR_NULL(str);
}

void test_goat__message__strdup___with_the_works(void) {
    const char *prefix = "anne";
    const char *command = "PRIVMSG";
    const char *params[] = { "#goat", "hello there", NULL };

    goat_message_t *message = goat_message_new(prefix, command, params);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    char *str = goat_message_strdup(message);

    CU_ASSERT_PTR_NOT_NULL_FATAL(str);
    CU_ASSERT_STRING_EQUAL(str, ":anne PRIVMSG #goat :hello there");

    free(str);
}
