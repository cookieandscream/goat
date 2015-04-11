#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <stdarg.h>

#include "src/goat.h"
#include "src/message.h"

static void _ptr_in_range(const void *ptr, const void *start, size_t len) {
    CU_ASSERT_PTR_NOT_NULL(ptr);
    CU_ASSERT_PTR_NOT_NULL_FATAL(start);
    CU_ASSERT(len > 0);

    CU_ASSERT(ptr >= start);
    CU_ASSERT(ptr < start + len);
}

static void _assert_message_params(const goat_message_t *msg, ...) {
    va_list ap;
    const char *s;
    size_t i = 0;

    va_start(ap, msg);
    s = va_arg(ap, const char *);
    while (s && i < 16) {
        CU_ASSERT_PTR_NOT_NULL(msg->m_params[i]);
        _ptr_in_range(msg->m_params[i], msg->m_bytes, msg->m_len);
        CU_ASSERT_STRING_EQUAL(msg->m_params[i], s);
        s = va_arg(ap, const char *);
        i++;
    }
    va_end(ap);

    for ( ; i < 16; i++) {
        CU_ASSERT_PTR_NULL(msg->m_params[i]);
    }
}

void test_goat__message__new___with_prefix(void) {
    const char *prefix = "prefix";
    const char *command = "command";

    goat_message_t *message = goat_message_new(prefix, command, NULL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message->m_prefix);
    CU_ASSERT_STRING_EQUAL(message->m_prefix, prefix);

    _ptr_in_range(message->m_prefix, message->m_bytes, message->m_len);

    goat_message_delete(message);
}

void test_goat__message__new___without_prefix(void) {
    const char *command = "command";

    goat_message_t *message = goat_message_new(NULL, command, NULL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);
    CU_ASSERT_PTR_NULL(message->m_prefix);

    goat_message_delete(message);
}

void test_goat__message__new___with_unrecognised_command(void) {
    const char *command = "command";

    goat_message_t *message = goat_message_new(NULL, command, NULL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    CU_ASSERT_FALSE(message->m_have_recognised_command);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message->m_command_string);
    CU_ASSERT_STRING_EQUAL(message->m_command_string, command);

    _ptr_in_range(message->m_command_string, message->m_bytes, message->m_len);

    goat_message_delete(message);
}

void test_goat__message__new___with_recognised_command(void) {
    const char *command = "PRIVMSG";

    goat_message_t *message = goat_message_new(NULL, command, NULL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    CU_ASSERT_TRUE(message->m_have_recognised_command);
    CU_ASSERT_EQUAL(message->m_command, GOAT_IRC_PRIVMSG);

    CU_ASSERT_PTR_EQUAL(message->m_command_string, goat_command_string(GOAT_IRC_PRIVMSG));
    CU_ASSERT_STRING_EQUAL(message->m_command_string, command);

    goat_message_delete(message);
}

void test_goat__message__new___without_params(void) {
    const char *command = "command";

    goat_message_t *message = goat_message_new(NULL, command, NULL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    _assert_message_params(message, NULL);

    goat_message_delete(message);
}

void test_goat__message__new___with_params(void) {
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param3", NULL };

    goat_message_t *message = goat_message_new(NULL, command, params);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    _assert_message_params(message, "param1", "param2", "param3", NULL);

    goat_message_delete(message);
}

void test_goat__message__new___with_space_param_last(void) {
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param 3 with spaces", NULL };

    goat_message_t *message = goat_message_new(NULL, command, params);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    _assert_message_params(message, "param1", "param2", "param 3 with spaces", NULL);

    goat_message_delete(message);
}

void test_goat__message__new___with_space_param_not_last(void) {
    const char *command = "command";
    const char *params[] = { "param1", "param 2 with spaces", "param3", NULL };

    goat_message_t *message = goat_message_new(NULL, command, params);

    CU_ASSERT_PTR_NULL(message);
}

void test_goat__message__new___with_the_works(void) {
    const char *prefix = "prefix";
    const char *command = "PRIVMSG";
    const char *params[] = { "#goat", "hello there", NULL };

    goat_message_t *message = goat_message_new(prefix, command, params);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message->m_prefix);
    CU_ASSERT_STRING_EQUAL(message->m_prefix, prefix);
    _ptr_in_range(message->m_prefix, message->m_bytes, message->m_len);

    CU_ASSERT_TRUE(message->m_have_recognised_command);
    CU_ASSERT_EQUAL(message->m_command, GOAT_IRC_PRIVMSG);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message->m_command_string);
    CU_ASSERT_STRING_EQUAL(message->m_command_string, command);
    CU_ASSERT_PTR_EQUAL(message->m_command_string, goat_command_string(GOAT_IRC_PRIVMSG));

    _assert_message_params(message, "#goat", "hello there", NULL);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___without_prefix(void) {
    const char *str = "command";

    goat_message_t *message = goat_message_new_from_string(str, strlen(str));

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    CU_ASSERT_PTR_NULL(message->m_prefix);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_prefix(void) {
    const char *str = ":prefix command";

    goat_message_t *message = goat_message_new_from_string(str, strlen(str));

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message->m_prefix);
    CU_ASSERT_STRING_EQUAL(message->m_prefix, "prefix");
    CU_ASSERT_PTR_EQUAL(message->m_prefix, &message->m_bytes[1]);
    _ptr_in_range(message->m_prefix, message->m_bytes, message->m_len);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_unrecognised_command(void) {
    const char *str = "command";

    goat_message_t *message = goat_message_new_from_string(str, strlen(str));

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    CU_ASSERT_FALSE(message->m_have_recognised_command);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message->m_command_string);
    CU_ASSERT_STRING_EQUAL(message->m_command_string, "command");
    _ptr_in_range(message->m_command_string, message->m_bytes, message->m_len);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_recognised_command(void) {
    const char *str = "PRIVMSG";

    goat_message_t *message = goat_message_new_from_string(str, strlen(str));

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    CU_ASSERT_TRUE(message->m_have_recognised_command);

    CU_ASSERT_EQUAL(message->m_command, GOAT_IRC_PRIVMSG);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message->m_command_string);
    CU_ASSERT_STRING_EQUAL(message->m_command_string, "PRIVMSG");
    CU_ASSERT_PTR_EQUAL(message->m_command_string, goat_command_string(GOAT_IRC_PRIVMSG));

    goat_message_delete(message);
}

void test_goat__message__new__from__string___without_params(void) {
    const char *str = "command";

    goat_message_t *message = goat_message_new_from_string(str, strlen(str));

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    _assert_message_params(message, NULL);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_params(void) {
    const char *str = "command param1 param2 param3";

    goat_message_t *message = goat_message_new_from_string(str, strlen(str));

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    _assert_message_params(message, "param1", "param2", "param3", NULL);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_colon_param(void) {
    const char *str = ":pfx cmd p1 p2 :p3 a b c d";

    goat_message_t *message = goat_message_new_from_string(str, strlen(str));

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    _assert_message_params(message, "p1", "p2", "p3 a b c d", NULL);

    goat_message_delete(message);
}

void test_goat__message__clone___without_prefix(void) {
    const char *command = "command";

    goat_message_t *msg1 = goat_message_new(NULL, command, NULL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);
    CU_ASSERT_PTR_NULL(msg1->m_prefix);

    goat_message_t *msg2 = goat_message_clone(msg1);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2);

    CU_ASSERT_PTR_NULL(msg2->m_prefix);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___with_prefix(void) {
    const char *prefix = "prefix";
    const char *command = "command";

    goat_message_t *msg1 = goat_message_new(prefix, command, NULL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1->m_prefix);

    goat_message_t *msg2 = goat_message_clone(msg1);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2->m_prefix);
    CU_ASSERT_STRING_EQUAL(msg2->m_prefix, msg1->m_prefix);
    _ptr_in_range(msg2->m_prefix, msg2->m_bytes, msg2->m_len);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___with_unrecognised_command(void) {
    const char *command = "command";

    goat_message_t *msg1 = goat_message_new(NULL, command, NULL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);
    CU_ASSERT_FALSE_FATAL(msg1->m_have_recognised_command);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1->m_command_string);

    goat_message_t *msg2 = goat_message_clone(msg1);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2);

    CU_ASSERT_FALSE(msg2->m_have_recognised_command);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2->m_command_string);
    CU_ASSERT_STRING_EQUAL(msg2->m_command_string, command);
    _ptr_in_range(msg2->m_command_string, msg2->m_bytes, msg2->m_len);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___with_recognised_command(void) {
    const char *command = "PRIVMSG";

    goat_message_t *msg1 = goat_message_new(NULL, command, NULL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);
    CU_ASSERT_TRUE_FATAL(msg1->m_have_recognised_command);
    CU_ASSERT_EQUAL_FATAL(msg1->m_command, GOAT_IRC_PRIVMSG);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1->m_command_string);

    goat_message_t *msg2 = goat_message_clone(msg1);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2);

    CU_ASSERT_TRUE(msg2->m_have_recognised_command);
    CU_ASSERT_EQUAL(msg2->m_command, msg1->m_command);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2->m_command_string);
    CU_ASSERT_PTR_EQUAL(msg2->m_command_string, goat_command_string(GOAT_IRC_PRIVMSG));

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___without_params(void) {
    const char *command = "command";

    goat_message_t *msg1 = goat_message_new(NULL, command, NULL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);

    goat_message_t *msg2 = goat_message_clone(msg1);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2);

    _assert_message_params(msg2, NULL);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___with_params(void) {
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param3", NULL };

    goat_message_t *msg1 = goat_message_new(NULL, command, params);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);

    goat_message_t *msg2 = goat_message_clone(msg1);

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2);

    _assert_message_params(msg2, "param1", "param2", "param3", NULL);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___with_the_works(void) {
    const char *prefix = "prefix";
    const char *command = "PRIVMSG";
    const char *params[] = { "#goat", "hello there", NULL };
    goat_message_t *msg1, *msg2;

    msg1 = goat_message_new(prefix, command, params);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);
    CU_ASSERT_TRUE_FATAL(msg1->m_have_recognised_command);
    CU_ASSERT_EQUAL_FATAL(msg1->m_command, GOAT_IRC_PRIVMSG);

    msg2 = goat_message_clone(msg1);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2);

    // FIXME tags

    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2->m_prefix);
    _ptr_in_range(msg2->m_prefix, msg2->m_bytes, msg2->m_len);
    CU_ASSERT_STRING_EQUAL(msg2->m_prefix, prefix);

    CU_ASSERT_EQUAL(msg2->m_have_recognised_command, msg1->m_have_recognised_command);
    CU_ASSERT_EQUAL(msg2->m_command, msg1->m_command);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2->m_command_string);
    CU_ASSERT_PTR_EQUAL(msg2->m_command_string, msg1->m_command_string);
    CU_ASSERT_PTR_EQUAL(msg2->m_command_string, goat_command_string(GOAT_IRC_PRIVMSG));
    CU_ASSERT_STRING_EQUAL(msg2->m_command_string, command);

    _assert_message_params(msg2, "#goat", "hello there", NULL);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}
