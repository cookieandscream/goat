#include <assert.h>
#include <stdlib.h>

#include "cmocka/run.h"

#include "src/goat.h"
#include "src/message.h"

goat_message_t *msg;

static void _set_tags(goat_message_t *message, const char *raw_tags);

int message_tags_group_init(void **state) {
    msg = goat_message_new(NULL, "PRIVMSG", NULL);
    if (NULL == msg) return -1;

    return 0;
}

int message_tags_group_cleanup(void **state) {
    if (msg) goat_message_delete(msg);
    return 0;
}

void test_goat__message__has__tags___without_message(void **state) {
    skip(); // FIXME not implemented yet
}

void test_goat__message__has__tags___without_tags(void **state) {
    _set_tags(msg, NULL);
    assert_int_equal(goat_message_has_tags(msg), 0);
}

void test_goat__message__has__tags___with_tag(void **state) {
    _set_tags(msg, "somekey");

    assert_int_equal(goat_message_has_tags(msg), 1);

    _set_tags(msg, "somekey;");

    assert_int_equal(goat_message_has_tags(msg), 1);

    _set_tags(msg, "somekey=somevalue");

    assert_int_equal(goat_message_has_tags(msg), 1);

    _set_tags(msg, "somekey=somevalue;");

    assert_int_equal(goat_message_has_tags(msg), 1);
}

void test_goat__message__has__tags___with_tags(void **state) {
    _set_tags(msg, "key1=val1;key2;key3=val3;key4;key5;");

    assert_int_equal(goat_message_has_tags(msg), 5);
}

void test_goat__message__has__tags___with_special_value(void **state) {
    _set_tags(msg, "plain=text;encoded=text\\:with\\:separators;one\\smore");

    assert_int_equal(goat_message_has_tags(msg), 3);
}

void test_goat__message__has__tag___when_it_does_not(void **state) {
    _set_tags(msg, "key1=val1;key2=val2;key3=val3");

    assert_false(goat_message_has_tag(msg, "key4"));
}

void test_goat__message__has__tag___when_it_does(void **state) {
    _set_tags(msg, "key1=val1;key2=val2;key3;key4;key5=val5");

    assert_true(goat_message_has_tag(msg, "key1"));
    assert_true(goat_message_has_tag(msg, "key2"));
    assert_true(goat_message_has_tag(msg, "key3"));
    assert_true(goat_message_has_tag(msg, "key4"));
    assert_true(goat_message_has_tag(msg, "key5"));
}

void test_goat__message__get__tag__value(void **state) {
    skip(); // FIXME not implemented yet
}

void test_goat__message__set__tag___when_no_tags(void **state) {
    if (msg->m_tags) free(msg->m_tags);
    msg->m_tags = NULL;

    assert_int_equal(goat_message_set_tag(msg, "key", "value"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "key=value");
}

void test_goat__message__set__tag___when_tags_empty(void **state) {
    _set_tags(msg, NULL);

    assert_int_equal(goat_message_set_tag(msg, "key", "value"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "key=value");
}

void test_goat__message__set__tag___when_already_a_tag(void **state) {
    _set_tags(msg, "b=bat");

    assert_int_equal(goat_message_set_tag(msg, "c", "cat"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "b=bat;c=cat");

    _set_tags(msg, "b=bat");

    assert_int_equal(goat_message_set_tag(msg, "a", "ant"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "b=bat;a=ant");
}

void test_goat__message__set__tag___when_already_many_tags(void **state) {
    _set_tags(msg, "a=ant;c=cat;e=egg");

    assert_int_equal(goat_message_set_tag(msg, "b", "bat"), 0);
    assert_int_equal(goat_message_set_tag(msg, "d", "dog"), 0);
    assert_int_equal(goat_message_set_tag(msg, "f", "fig"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "a=ant;c=cat;e=egg;b=bat;d=dog;f=fig");
}

void test_goat__message__set__tag___when_tag_already_exists(void **state) {
    _set_tags(msg, "a=ant;b=bat;c=cat");

    assert_int_equal(goat_message_set_tag(msg, "b", "bog"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "a=ant;c=cat;b=bog");
}

void test_goat__message__set__tag___no_value(void **state) {
    _set_tags(msg, "a=ant;c=cat");

    assert_int_equal(goat_message_set_tag(msg, "b", NULL), 0);

    assert_string_equal(msg->m_tags->m_bytes, "a=ant;c=cat;b");
}

void test_goat__message__set__tag___replace_with_no_value(void **state) {
    _set_tags(msg, "a=ant;b=bat;c=cat");

    assert_int_equal(goat_message_set_tag(msg, "b", NULL), 0);

    assert_string_equal(msg->m_tags->m_bytes, "a=ant;c=cat;b");
}

void test_goat__message__set__tag___replace_no_value_with_value(void **state) {
    _set_tags(msg, "a=ant;b;c=cat");

    assert_int_equal(goat_message_set_tag(msg, "b", "bat"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "a=ant;c=cat;b=bat");
}

void test_goat__message__set__tag___escape_special_values(void **state) {
    _set_tags(msg, NULL);

    assert_int_equal(goat_message_set_tag(msg, "oh", ";| |\\|\r|\n"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "oh=\\:|\\s|\\\\|\\r|\\n");
}

void test_goat__message__unset__tag___no_tags(void **state) {
    if (msg->m_tags) free(msg->m_tags);
    msg->m_tags = NULL;

    assert_int_equal(goat_message_unset_tag(msg, "somekey"), 0);

    assert_null(msg->m_tags);
}

void test_goat__message__unset__tag___empty_tags(void **state) {
    _set_tags(msg, NULL);

    assert_int_equal(goat_message_unset_tag(msg, "somekey"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "");
}

void test_goat__message__unset__tag___only_tag(void **state) {
    _set_tags(msg, "a=ant");

    assert_int_equal(goat_message_unset_tag(msg, "a"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "");
}

void test_goat__message__unset__tag___first_tag(void **state) {
    _set_tags(msg, "a=ant;b=bat");

    assert_int_equal(goat_message_unset_tag(msg, "a"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "b=bat");
}

void test_goat__message__unset__tag___last_tag(void **state) {
    _set_tags(msg, "a=ant;b=bat");

    assert_int_equal(goat_message_unset_tag(msg, "b"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "a=ant");
}

void test_goat__message__unset__tag___middle_tag(void **state) {
    _set_tags(msg, "a=ant;b=bat;c=cat");

    assert_int_equal(goat_message_unset_tag(msg, "b"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "a=ant;c=cat");
}

void test_goat__message__unset__tag___first_tag_no_value(void **state) {
    _set_tags(msg, "a;b=bat");

    assert_int_equal(goat_message_unset_tag(msg, "a"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "b=bat");
}

void test_goat__message__unset__tag___last_tag_no_value(void **state) {
    _set_tags(msg, "a=ant;b");

    assert_int_equal(goat_message_unset_tag(msg, "b"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "a=ant");
}

void test_goat__message__unset__tag___middle_tag_no_value(void **state) {
    _set_tags(msg, "a=ant;b;c=cat");

    assert_int_equal(goat_message_unset_tag(msg, "b"), 0);

    assert_string_equal(msg->m_tags->m_bytes, "a=ant;c=cat");
}

static void _set_tags(goat_message_t *message, const char *raw_tags) {
    assert(NULL != message);

    if (NULL == message->m_tags) {
        message->m_tags = calloc(1, sizeof(*message->m_tags));
        assert_non_null(message->m_tags);
    }

    memset(message->m_tags, 0, sizeof(*message->m_tags));

    if (raw_tags) {
        message->m_tags->m_len = strlen(raw_tags);
        strcpy(message->m_tags->m_bytes, raw_tags);
    }
}

