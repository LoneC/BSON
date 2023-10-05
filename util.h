#ifndef _BSON_HELP_H_
#define _BSON_HELP_H_

#include <stdint.h>

uint64_t  bson_hash(const char *key);
int       bson_is_whitespace(char c);
void      bson_trim_string(char *dst, const char *src);
void      bson_free_inner_strings(void *data);

#endif
