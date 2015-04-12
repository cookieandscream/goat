#ifndef GOAT_TAGS_H
#define GOAT_TAGS_H

size_t tags_parse(const char *str, goat_message_tags_t **tagsp);

int tags_init(goat_message_tags_t **tagsp, const char *key, const char *val);

#endif
