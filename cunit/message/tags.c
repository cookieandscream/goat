#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <assert.h>
#include <stdlib.h>

#include "src/goat.h"
#include "src/message.h"

goat_message_t *msg;

static void _set_tags(goat_message_t *message, const char *raw_tags);

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
    _set_tags(msg, NULL);
    CU_ASSERT_EQUAL(goat_message_has_tags(msg), 0);
}

void test_goat__message__has__tags___with_tag(void) {
    _set_tags(msg, "somekey");

    CU_ASSERT_EQUAL(goat_message_has_tags(msg), 1);

    _set_tags(msg, "somekey;");

    CU_ASSERT_EQUAL(goat_message_has_tags(msg), 1);

    _set_tags(msg, "somekey=somevalue");

    CU_ASSERT_EQUAL(goat_message_has_tags(msg), 1);

    _set_tags(msg, "somekey=somevalue;");

    CU_ASSERT_EQUAL(goat_message_has_tags(msg), 1);
}

void test_goat__message__has__tags___with_tags(void) {
    _set_tags(msg, "key1=val1;key2;key3=val3;key4;key5;");

    CU_ASSERT_EQUAL(goat_message_has_tags(msg), 5);
}

void test_goat__message__has__tags___with_special_value(void) {
    _set_tags(msg, "plain=text;encoded=text\\:with\\:separators;one\\smore");

    CU_ASSERT_EQUAL(goat_message_has_tags(msg), 3);
}

void test_goat__message__has__tag___when_it_does_not(void) {
    _set_tags(msg, "key1=val1;key2=val2;key3=val3");

    CU_ASSERT_FALSE(goat_message_has_tag(msg, "key4"));
}

void test_goat__message__has__tag___when_it_does(void) {
    _set_tags(msg, "key1=val1;key2=val2;key3;key4;key5=val5");

    CU_ASSERT_TRUE(goat_message_has_tag(msg, "key1"));
    CU_ASSERT_TRUE(goat_message_has_tag(msg, "key2"));
    CU_ASSERT_TRUE(goat_message_has_tag(msg, "key3"));
    CU_ASSERT_TRUE(goat_message_has_tag(msg, "key4"));
    CU_ASSERT_TRUE(goat_message_has_tag(msg, "key5"));
}

//int goat_message_has_tag(const goat_message_t *message, const char *key);
//int goat_message_get_tag_value(const goat_message_t *message, const char *key, char *value, size_t *size);
//int goat_message_set_tag(goat_message_t *message, const char *key, const char *value);
//int goat_message_unset_tag(goat_message_t *message, const char *key);

static void _set_tags(goat_message_t *message, const char *raw_tags) {
    assert(NULL != message);

    if (NULL == message->m_tags) {
        message->m_tags = calloc(1, sizeof(*message->m_tags));
        CU_ASSERT_PTR_NOT_NULL_FATAL(message->m_tags);
    }

    memset(message->m_tags, 0, sizeof(*message->m_tags));

    if (raw_tags) {
        message->m_tags->m_len = strlen(raw_tags);
        strcpy(message->m_tags->m_bytes, raw_tags);
    }
}

