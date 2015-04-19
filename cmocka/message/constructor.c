#include <stdarg.h>
#include <string.h>

#include "cmocka/run.h"

#include "src/goat.h"
#include "src/message.h"

static void _ptr_in_range(const void *ptr, const void *start, size_t len) {
    assert_non_null(ptr);
    assert_non_null(start);
    assert_true(len > 0);

    assert_true(ptr >= start);
    assert_true(ptr < start + len);
}

static void _assert_message_params(const GoatMessage *msg, ...) {
    va_list ap;
    const char *s;
    size_t i = 0;

    va_start(ap, msg);
    s = va_arg(ap, const char *);
    while (s && i < 16) {
        assert_non_null(msg->m_params[i]);
        _ptr_in_range(msg->m_params[i], msg->m_bytes, msg->m_len);
        assert_string_equal(msg->m_params[i], s);
        s = va_arg(ap, const char *);
        i++;
    }
    va_end(ap);

    for ( ; i < 16; i++) {
        assert_null(msg->m_params[i]);
    }
}

void test_goat__message__new___with_prefix(void **state UNUSED) {
    const char *prefix = "prefix";
    const char *command = "command";

    GoatMessage *message = goat_message_new(prefix, command, NULL);

    assert_non_null(message);
    assert_non_null(message->m_prefix);
    assert_string_equal(message->m_prefix, prefix);

    _ptr_in_range(message->m_prefix, message->m_bytes, message->m_len);

    goat_message_delete(message);
}

void test_goat__message__new___with_invalid_prefix_space(void **state UNUSED) {
    const char *prefix = "invalid prefix";
    const char *command = "command";

    GoatMessage *message = goat_message_new(prefix, command, NULL);

    assert_null(message);
}

void test_goat__message__new___with_invalid_prefix_cr(void **state UNUSED) {
    const char *prefix = "invalid\x0dprefix";
    const char *command = "command";

    GoatMessage *message = goat_message_new(prefix, command, NULL);

    assert_null(message);
}

void test_goat__message__new___with_invalid_prefix_lf(void **state UNUSED) {
    const char *prefix = "invalid\x0aprefix";
    const char *command = "command";

    GoatMessage *message = goat_message_new(prefix, command, NULL);

    assert_null(message);
}

void test_goat__message__new___without_prefix(void **state UNUSED) {
    const char *command = "command";

    GoatMessage *message = goat_message_new(NULL, command, NULL);

    assert_non_null(message);
    assert_null(message->m_prefix);

    goat_message_delete(message);
}

void test_goat__message__new___with_unrecognised_command(void **state UNUSED) {
    const char *command = "command";

    GoatMessage *message = goat_message_new(NULL, command, NULL);

    assert_non_null(message);

    assert_false(message->m_have_recognised_command);

    assert_non_null(message->m_command_string);
    assert_string_equal(message->m_command_string, command);

    _ptr_in_range(message->m_command_string, message->m_bytes, message->m_len);

    goat_message_delete(message);
}

void test_goat__message__new___with_recognised_command(void **state UNUSED) {
    const char *command = "PRIVMSG";

    GoatMessage *message = goat_message_new(NULL, command, NULL);

    assert_non_null(message);

    assert_true(message->m_have_recognised_command);
    assert_int_equal(message->m_command, GOAT_IRC_PRIVMSG);

    assert_int_equal(message->m_command_string, goat_command_string(GOAT_IRC_PRIVMSG));
    assert_string_equal(message->m_command_string, command);

    goat_message_delete(message);
}

void test_goat__message__new___with_invalid_command_space(void **state UNUSED) {
    const char *command = "invalid command";

    GoatMessage *message = goat_message_new(NULL, command, NULL);

    assert_null(message);
}

void test_goat__message__new___with_invalid_command_cr(void **state UNUSED) {
    const char *command = "invalid" "\x0d" "command";

    GoatMessage *message = goat_message_new(NULL, command, NULL);

    assert_null(message);
}

void test_goat__message__new___with_invalid_command_lf(void **state UNUSED) {
    const char *command = "invalid" "\x0a" "command";

    GoatMessage *message = goat_message_new(NULL, command, NULL);

    assert_null(message);
}

void test_goat__message__new___without_params(void **state UNUSED) {
    const char *command = "command";

    GoatMessage *message = goat_message_new(NULL, command, NULL);

    assert_non_null(message);

    _assert_message_params(message, NULL);

    goat_message_delete(message);
}

void test_goat__message__new___with_params(void **state UNUSED) {
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param3", NULL };

    GoatMessage *message = goat_message_new(NULL, command, params);

    assert_non_null(message);

    _assert_message_params(message, "param1", "param2", "param3", NULL);

    goat_message_delete(message);
}

void test_goat__message__new___with_space_param_last(void **state UNUSED) {
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param 3 with spaces", NULL };

    GoatMessage *message = goat_message_new(NULL, command, params);

    assert_non_null(message);

    _assert_message_params(message, "param1", "param2", "param 3 with spaces", NULL);

    goat_message_delete(message);
}

void test_goat__message__new___with_space_param_2nd_last(void **state UNUSED) {
    const char *command = "command";
    const char *params[] = { "param1", "param 2 with spaces", "param3", NULL };

    GoatMessage *message = goat_message_new(NULL, command, params);

    assert_null(message);
}

void test_goat__message__new___with_space_param_3nd_last(void **state UNUSED) {
    const char *command = "command";
    const char *params[] = { "param1", "param 2 with spaces", "param3", "param4", NULL };

    GoatMessage *message = goat_message_new(NULL, command, params);

    assert_null(message);
}

void test_goat__message__new___with_space_param_not_last(void **state UNUSED) {
    const char *command = "command";
    const char *params[] = { "p1", "p 2 with spaces", "p3", "p4", "p5", NULL };

    GoatMessage *message = goat_message_new(NULL, command, params);

    assert_null(message);
}

void test_goat__message__new___with_the_works(void **state UNUSED) {
    const char *prefix = "prefix";
    const char *command = "PRIVMSG";
    const char *params[] = { "#goat", "hello there", NULL };

    GoatMessage *message = goat_message_new(prefix, command, params);

    assert_non_null(message);

    assert_non_null(message->m_prefix);
    assert_string_equal(message->m_prefix, prefix);
    _ptr_in_range(message->m_prefix, message->m_bytes, message->m_len);

    assert_true(message->m_have_recognised_command);
    assert_int_equal(message->m_command, GOAT_IRC_PRIVMSG);
    assert_non_null(message->m_command_string);
    assert_string_equal(message->m_command_string, command);
    assert_int_equal(message->m_command_string, goat_command_string(GOAT_IRC_PRIVMSG));

    _assert_message_params(message, "#goat", "hello there", NULL);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___without_prefix(void **state UNUSED) {
    const char *str = "command";

    GoatMessage *message = goat_message_new_from_string(str, strlen(str));

    assert_non_null(message);

    assert_null(message->m_prefix);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_prefix(void **state UNUSED) {
    const char *str = ":prefix command";

    GoatMessage *message = goat_message_new_from_string(str, strlen(str));

    assert_non_null(message);

    assert_non_null(message->m_prefix);
    assert_string_equal(message->m_prefix, "prefix");
    assert_int_equal(message->m_prefix, &message->m_bytes[1]);
    _ptr_in_range(message->m_prefix, message->m_bytes, message->m_len);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_unrecognised_command(void **state UNUSED) {
    const char *str = "command";

    GoatMessage *message = goat_message_new_from_string(str, strlen(str));

    assert_non_null(message);

    assert_false(message->m_have_recognised_command);

    assert_non_null(message->m_command_string);
    assert_string_equal(message->m_command_string, "command");
    _ptr_in_range(message->m_command_string, message->m_bytes, message->m_len);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_recognised_command(void **state UNUSED) {
    const char *str = "PRIVMSG";

    GoatMessage *message = goat_message_new_from_string(str, strlen(str));

    assert_non_null(message);

    assert_true(message->m_have_recognised_command);

    assert_int_equal(message->m_command, GOAT_IRC_PRIVMSG);

    assert_non_null(message->m_command_string);
    assert_string_equal(message->m_command_string, "PRIVMSG");
    assert_int_equal(message->m_command_string, goat_command_string(GOAT_IRC_PRIVMSG));

    goat_message_delete(message);
}

void test_goat__message__new__from__string___without_params(void **state UNUSED) {
    const char *str = "command";

    GoatMessage *message = goat_message_new_from_string(str, strlen(str));

    assert_non_null(message);

    _assert_message_params(message, NULL);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_params(void **state UNUSED) {
    const char *str = "command param1 param2 param3";

    GoatMessage *message = goat_message_new_from_string(str, strlen(str));

    assert_non_null(message);

    _assert_message_params(message, "param1", "param2", "param3", NULL);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_colon_param(void **state UNUSED) {
    const char *str = "command param1 param2 :param3";

    GoatMessage *message = goat_message_new_from_string(str, strlen(str));

    assert_non_null(message);

    _assert_message_params(message, "param1", "param2", "param3", NULL);

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_crlf(void **state UNUSED) {
    const char *str = "command\x0d\x0a";

    GoatMessage *message = goat_message_new_from_string(str, strlen(str));

    assert_non_null(message);
    assert_non_null(message->m_command_string);
    assert_string_equal(message->m_command_string, "command");

    goat_message_delete(message);
}

void test_goat__message__new__from__string___with_the_works(void **state UNUSED) {
    const char *str = ":anne PRIVMSG #goat :hello there\x0d\x0a";

    GoatMessage *message = goat_message_new_from_string(str, strlen(str));

    assert_non_null(message);

    assert_non_null(message->m_prefix);
    assert_string_equal(message->m_prefix, "anne");

    assert_true(message->m_have_recognised_command);
    assert_int_equal(message->m_command, GOAT_IRC_PRIVMSG);
    assert_non_null(message->m_command_string);
    assert_int_equal(message->m_command_string, goat_command_string(GOAT_IRC_PRIVMSG));

    _assert_message_params(message, "#goat", "hello there", NULL);

    goat_message_delete(message);
}

void test_goat__message__clone___without_prefix(void **state UNUSED) {
    const char *command = "command";

    GoatMessage *msg1 = goat_message_new(NULL, command, NULL);

    assert_non_null(msg1);
    assert_null(msg1->m_prefix);

    GoatMessage *msg2 = goat_message_clone(msg1);

    assert_non_null(msg2);

    assert_null(msg2->m_prefix);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___with_prefix(void **state UNUSED) {
    const char *prefix = "prefix";
    const char *command = "command";

    GoatMessage *msg1 = goat_message_new(prefix, command, NULL);

    assert_non_null(msg1);
    assert_non_null(msg1->m_prefix);

    GoatMessage *msg2 = goat_message_clone(msg1);

    assert_non_null(msg2);
    assert_non_null(msg2->m_prefix);
    assert_string_equal(msg2->m_prefix, msg1->m_prefix);
    _ptr_in_range(msg2->m_prefix, msg2->m_bytes, msg2->m_len);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___with_unrecognised_command(void **state UNUSED) {
    const char *command = "command";

    GoatMessage *msg1 = goat_message_new(NULL, command, NULL);

    assert_non_null(msg1);
    assert_false(msg1->m_have_recognised_command);
    assert_non_null(msg1->m_command_string);

    GoatMessage *msg2 = goat_message_clone(msg1);

    assert_non_null(msg2);

    assert_false(msg2->m_have_recognised_command);
    assert_non_null(msg2->m_command_string);
    assert_string_equal(msg2->m_command_string, command);
    _ptr_in_range(msg2->m_command_string, msg2->m_bytes, msg2->m_len);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___with_recognised_command(void **state UNUSED) {
    const char *command = "PRIVMSG";

    GoatMessage *msg1 = goat_message_new(NULL, command, NULL);

    assert_non_null(msg1);
    assert_true(msg1->m_have_recognised_command);
    assert_int_equal(msg1->m_command, GOAT_IRC_PRIVMSG);
    assert_non_null(msg1->m_command_string);

    GoatMessage *msg2 = goat_message_clone(msg1);

    assert_non_null(msg2);

    assert_true(msg2->m_have_recognised_command);
    assert_int_equal(msg2->m_command, msg1->m_command);
    assert_non_null(msg2->m_command_string);
    assert_int_equal(msg2->m_command_string, goat_command_string(GOAT_IRC_PRIVMSG));

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___without_params(void **state UNUSED) {
    const char *command = "command";

    GoatMessage *msg1 = goat_message_new(NULL, command, NULL);

    assert_non_null(msg1);

    GoatMessage *msg2 = goat_message_clone(msg1);

    assert_non_null(msg2);

    _assert_message_params(msg2, NULL);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___with_params(void **state UNUSED) {
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param3", NULL };

    GoatMessage *msg1 = goat_message_new(NULL, command, params);

    assert_non_null(msg1);

    GoatMessage *msg2 = goat_message_clone(msg1);

    assert_non_null(msg2);

    _assert_message_params(msg2, "param1", "param2", "param3", NULL);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}

void test_goat__message__clone___with_the_works(void **state UNUSED) {
    const char *prefix = "prefix";
    const char *command = "PRIVMSG";
    const char *params[] = { "#goat", "hello there", NULL };
    GoatMessage *msg1, *msg2;

    msg1 = goat_message_new(prefix, command, params);
    assert_non_null(msg1);
    assert_true(msg1->m_have_recognised_command);
    assert_int_equal(msg1->m_command, GOAT_IRC_PRIVMSG);

    msg2 = goat_message_clone(msg1);
    assert_non_null(msg2);

    // FIXME tags

    assert_non_null(msg2->m_prefix);
    _ptr_in_range(msg2->m_prefix, msg2->m_bytes, msg2->m_len);
    assert_string_equal(msg2->m_prefix, prefix);

    assert_int_equal(msg2->m_have_recognised_command, msg1->m_have_recognised_command);
    assert_int_equal(msg2->m_command, msg1->m_command);
    assert_non_null(msg2->m_command_string);
    assert_int_equal(msg2->m_command_string, msg1->m_command_string);
    assert_int_equal(msg2->m_command_string, goat_command_string(GOAT_IRC_PRIVMSG));
    assert_string_equal(msg2->m_command_string, command);

    _assert_message_params(msg2, "#goat", "hello there", NULL);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}
