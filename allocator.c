#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "bson.h"

void *defmalloc(size_t size, void *ud)                 { return malloc(size);           }
void *defcalloc(size_t nummemb, size_t size, void *ud) { return calloc(nummemb, size);  }
void *defrealloc(void *ptr, size_t size, void *ud)     { return realloc(ptr, size);     }
char *defstrdup(const char *str, void *ud)             { return strdup(str);            }
void  deffree(void *ptr, void *ud)                     {        free(ptr);              }

static bsonmem mem = {
    .malloc   = defmalloc,
    .calloc   = defcalloc,
    .realloc  = defrealloc,
    .strdup   = defstrdup,
    .free     = deffree,
    .userdata = NULL
};

bsonenum bson_set_allocator(const bsonmem *m) {
    if(
	m == NULL          ||
	m->malloc == NULL  ||
	m->calloc == NULL  ||
	m->realloc == NULL ||
	m->strdup == NULL  ||
	m->free
    ) return BSON_NULL_PTR;
    mem = *m;
    return BSON_SUCCESS;
}

void *bsonmalloc(size_t size)                 { return mem.malloc(size, mem.userdata);          }
void *bsoncalloc(size_t nummemb, size_t size) { return mem.calloc(nummemb, size, mem.userdata); }
void *bsonrealloc(void *ptr, size_t size)     { return mem.realloc(ptr, size, mem.userdata);    }
char *bsonstrdup(const char *str)             { return mem.strdup(str, mem.userdata);           }
void  bsonfree(void *ptr)                     {        mem.free(ptr, mem.userdata);             }
