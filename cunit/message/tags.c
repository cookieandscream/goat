#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "src/goat.h"
#include "src/message.h"

goat_message_t *msg;

static void _clear_message_tags(goat_message_t *message);

int message_tags_suite_init(void) {
    msg = goat_message_new(NULL, "PRIVMSG", NULL);
    if (NULL == msg) return -1;

    return 0;
}

int message_tags_suite_cleanup(void) {
    if (msg) goat_message_delete(msg);
    return 0;
}

void test_goat__message__has__tags___without_message(void) {
    CU_FAIL("not implemented yet");
}

void test_goat__message__has__tags___without_tags(void) {
    _clear_message_tags(msg);
    CU_ASSERT_EQUAL(goat_message_has_tags(msg), 0);
}

void test_goat__message__has__tags___with_tag(void) {
    _clear_message_tags(msg);

    int ret = goat_message_set_tag(msg, "somekey", "somevalue");
    CU_ASSERT_EQUAL_FATAL(ret, 0);

    CU_ASSERT_EQUAL(goat_message_has_tags(msg), 1);
    _clear_message_tags(msg);
}

void test_goat__message__has__tags___with_tags(void) {
    _clear_message_tags(msg);

    const size_t n_tags = 5;
    for (size_t i = 0; i < n_tags; i++) {
        char key[5], val[5];
        snprintf(key, sizeof(key), "%s%zu", "key", i);
        snprintf(val, sizeof(val), "%s%zu", "val", i);
        int ret = goat_message_set_tag(msg, key, val);
        CU_ASSERT_EQUAL_FATAL(ret, 0);
    }

    CU_ASSERT_EQUAL(goat_message_has_tags(msg), n_tags);
    _clear_message_tags(msg);
}

//size_t goat_message_has_tags(const goat_message_t *message);
//int goat_message_has_tag(const goat_message_t *message, const char *key);
//int goat_message_get_tag_value(const goat_message_t *message, const char *key, char *value, size_t *size);
//int goat_message_set_tag(goat_message_t *message, const char *key, const char *value);
//int goat_message_unset_tag(goat_message_t *message, const char *key);

static void _clear_message_tags(goat_message_t *message) {
    if (message && message->m_tags) {
        memset(message->m_tags, 0, sizeof(*message->m_tags));
    }
}

