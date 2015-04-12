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

void test_goat__message__get__tag__value(void) {
    CU_FAIL("not implemented");
}

void test_goat__message__set__tag___when_no_tags(void) {
    if (msg->m_tags) free(msg->m_tags);
    msg->m_tags = NULL;

    CU_ASSERT_EQUAL_FATAL(goat_message_set_tag(msg, "key", "value"), 0);

    CU_ASSERT_STRING_EQUAL(msg->m_tags->m_bytes, "key=value");
}

void test_goat__message__set__tag___when_tags_empty(void) {
    _set_tags(msg, NULL);

    CU_ASSERT_EQUAL_FATAL(goat_message_set_tag(msg, "key", "value"), 0);

    CU_ASSERT_STRING_EQUAL(msg->m_tags->m_bytes, "key=value");
}

void test_goat__message__set__tag___when_already_a_tag(void) {
    _set_tags(msg, "b=bat");

    CU_ASSERT_EQUAL_FATAL(goat_message_set_tag(msg, "c", "cat"), 0);

    CU_ASSERT_STRING_EQUAL(msg->m_tags->m_bytes, "b=bat;c=cat");

    _set_tags(msg, "b=bat");

    CU_ASSERT_EQUAL_FATAL(goat_message_set_tag(msg, "a", "ant"), 0);

    CU_ASSERT_STRING_EQUAL(msg->m_tags->m_bytes, "a=ant;b=bat");
}

void test_goat__message__set__tag___when_already_many_tags(void) {
    _set_tags(msg, "a=ant;c=cat;e=egg");

    CU_ASSERT_EQUAL_FATAL(goat_message_set_tag(msg, "b", "bat"), 0);
    CU_ASSERT_EQUAL_FATAL(goat_message_set_tag(msg, "d", "dog"), 0);
    CU_ASSERT_EQUAL_FATAL(goat_message_set_tag(msg, "f", "fig"), 0);

    CU_ASSERT_STRING_EQUAL(msg->m_tags->m_bytes, "a=ant;b=bat;c=cat;d=dog;e=egg;f=fig");
}

void test_goat__message__set__tag___when_tag_already_exists(void) {
    _set_tags(msg, "a=ant;b=bat;c=cat");

    CU_ASSERT_EQUAL_FATAL(goat_message_set_tag(msg, "b", "bog"), 0);

    CU_ASSERT_STRING_EQUAL(msg->m_tags->m_bytes, "a=ant;b=bog;c=cat");
}

void test_goat__message__set__tag___no_value(void) {
    _set_tags(msg, "a=ant;c=cat");

    CU_ASSERT_EQUAL_FATAL(goat_message_set_tag(msg, "b", NULL), 0);

    CU_ASSERT_STRING_EQUAL(msg->m_tags->m_bytes, "a=ant;b;c=cat");
}

void test_goat__message__set__tag___replace_with_no_value(void) {
    _set_tags(msg, "a=ant;b=bat;c=cat");

    CU_ASSERT_EQUAL_FATAL(goat_message_set_tag(msg, "b", NULL), 0);

    CU_ASSERT_STRING_EQUAL(msg->m_tags->m_bytes, "a=ant;b;c=cat");
}

void test_goat__message__set__tag___replace_no_value_with_value(void) {
    _set_tags(msg, "a=ant;b;c=cat");

    CU_ASSERT_EQUAL_FATAL(goat_message_set_tag(msg, "b", "bat"), 0);

    CU_ASSERT_STRING_EQUAL(msg->m_tags->m_bytes, "a=ant;b=bat;c=cat");
}

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

