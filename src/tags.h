#ifndef GOAT_TAGS_H
#define GOAT_TAGS_H

size_t tags_parse(const char *str, MessageTags **tagsp);

int tags_init(MessageTags **tagsp, const char *key, const char *val);

#endif
