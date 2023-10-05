#ifndef _BSON_ALLOCATOR_H_
#define _BSON_ALLOCATOR_H_

#include <stddef.h>

void *bsonmalloc(size_t size);
void *bsoncalloc(size_t nummemb, size_t size);
void *bsonrealloc(void *ptr, size_t size);
void *bsonstrdup(const char *str);
void  bsonfree(void *ptr);

#endif
