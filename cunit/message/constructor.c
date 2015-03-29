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

void test_goat_message_new(void) {
    const char *prefix = "prefix";
    const char *command = "command";
    const char *privmsg = "PRIVMSG";
    const char *privmsg_static = goat_message_static_command("PRIVMSG");
    const char *params[] = { "these", "are", "some", "params", NULL };
    const size_t nparams = sizeof(params) / sizeof(params[0]);

    goat_message_t *message = goat_message_new(prefix, command, params);

    CU_ASSERT_PTR_NOT_NULL(message);
    CU_ASSERT_STRING_EQUAL(message->m_prefix, prefix);
    CU_ASSERT_STRING_EQUAL(message->m_command, command);

    // check that m_prefix points inside m_bytes
    _ptr_in_range(message->m_prefix, message->m_bytes, message->m_len);

    // check that m_prefix points inside m_bytes
    // (n.b. "command" is not a valid irc command, so it won't be staticked)
    _ptr_in_range(message->m_command, message->m_bytes, message->m_len);

    // check the params we passed
    for (size_t i = 0; i < nparams; i++) {
        if (params[i] != NULL) {
            CU_ASSERT_STRING_EQUAL(message->m_params[i], params[i]);
            _ptr_in_range(message->m_params[i], message->m_bytes, message->m_len);
        }
        else {
            CU_ASSERT_PTR_NULL(message->m_params[i]);
        }
    }

    // check the params we didn't pass
    for (size_t i = nparams; i < 16; i++) {
        CU_ASSERT_PTR_NULL(message->m_params[i]);
    }

    goat_message_delete(message);

    message = goat_message_new(NULL, privmsg, NULL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(message);
    CU_ASSERT_PTR_NULL(message->m_prefix);
    CU_ASSERT_PTR_EQUAL(message->m_command, privmsg_static);
    _assert_message_params(message, NULL);

    goat_message_delete(message);
}

void test_goat_message_new_from_string(void) {
    const char *without_prefix = "command param1 param2";
    const char *with_prefix = ":prefix command param1 param2";
    const char *with_colon_param = ":pfx cmd p1 p2 :p3 a b c d";

    goat_message_t *message = goat_message_new_from_string(without_prefix,
        strlen(without_prefix));

    CU_ASSERT_PTR_NOT_NULL(message);
    CU_ASSERT_PTR_NULL(message->m_prefix);
    CU_ASSERT_PTR_NOT_NULL(message->m_command);
    CU_ASSERT_STRING_EQUAL(message->m_command, "command");
    _assert_message_params(message, "param1", "param2", NULL);

    goat_message_delete(message);

    goat_message_new_from_string(with_prefix, strlen(with_prefix));

    CU_ASSERT_PTR_NOT_NULL(message);
    CU_ASSERT_PTR_NOT_NULL(message->m_prefix);
    CU_ASSERT_STRING_EQUAL(message->m_prefix, "prefix");
    CU_ASSERT_PTR_NOT_NULL(message->m_command);
    CU_ASSERT_STRING_EQUAL(message->m_command, "command");
    _assert_message_params(message, "param1", "param2", NULL);

    goat_message_delete(message);

    message = goat_message_new_from_string(with_colon_param, strlen(with_colon_param));

    CU_ASSERT_PTR_NOT_NULL(message);
    CU_ASSERT_PTR_NOT_NULL(message->m_prefix);
    CU_ASSERT_STRING_EQUAL(message->m_prefix, "pfx");
    CU_ASSERT_PTR_NOT_NULL(message->m_command);
    CU_ASSERT_STRING_EQUAL(message->m_command, "cmd");
    _assert_message_params(message, "p1", "p2", "p3 a b c d", NULL);

    goat_message_delete(message);
}

void test_goat_message_clone(void) {
    const char *prefix = "prefix";
    const char *command = "command";
    const char *params[] = { "param1", "param2", "param3", NULL };
    const size_t nparams = sizeof(params) / sizeof(params[0]);
    goat_message_t *msg1, *msg2;

    msg1 = goat_message_new(prefix, command, params);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);

    msg2 = goat_message_clone(msg1);
    CU_ASSERT_PTR_NOT_NULL(msg2);

    // FIXME tags

    CU_ASSERT_PTR_NOT_NULL(msg2->m_prefix);
    CU_ASSERT_PTR_NOT_EQUAL(msg2->m_prefix, msg1->m_prefix);
    _ptr_in_range(msg2->m_prefix, msg2->m_bytes, msg2->m_len);
    CU_ASSERT_STRING_EQUAL(msg2->m_prefix, prefix);

    CU_ASSERT_PTR_NOT_NULL(msg2->m_command);
    CU_ASSERT_PTR_NOT_EQUAL(msg2->m_command, msg1->m_command);
    _ptr_in_range(msg2->m_command, msg2->m_bytes, msg2->m_len);
    CU_ASSERT_STRING_EQUAL(msg2->m_command, command);

    _assert_message_params(msg2, "param1", "param2", "param3", NULL);

    goat_message_delete(msg2);
    goat_message_delete(msg1);
}
