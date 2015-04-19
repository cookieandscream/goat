#ifndef GOAT_TAGS_H
#define GOAT_TAGS_H

size_t tags_parse(const char *str, GoatMessageTags **tagsp);

int tags_init(GoatMessageTags **tagsp, const char *key, const char *val);

#endif
