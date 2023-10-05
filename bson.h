#ifndef _BSON_H_
#define _BSON_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif 

typedef enum {
   BSON_SUCCESS = 0,
   BSON_FILE_PATH,
   BSON_MEMORY,
   BSON_NULL_PTR,
   BSON_SYNTAX,
   BSON_INVALID_VALUE,
   BSON_NOT_FOUND,
   BSON_CONTINUE,
   BSON_INT,
   BSON_DBL,
   BSON_STR,
   BSON_MAX
} bsonenum;

typedef void *(*bson_pfn_malloc)(uint64_t, void *);
typedef void *(*bson_pfn_calloc)(uint64_t, uint64_t, void *);
typedef void *(*bson_pfn_realloc)(void *, uint64_t, void *);
typedef char *(*bson_pfn_strdup)(const char *, void *ud);
typedef void  (*bson_pfn_free)(void *, void *);

typedef struct _s_bsoncmem {
    bson_pfn_malloc  malloc;
    bson_pfn_calloc  calloc;
    bson_pfn_realloc realloc;
    bson_pfn_strdup  strdup;
    bson_pfn_free    free;
    void          *userdata;
} bsonmem;
bsonenum bson_set_allocator(const bsonmem *a);

typedef struct _s_BSON BSON;

BSON          *bson_open(const char *filepath, bsonenum *result);
void         bson_free(BSON **bson, bsonenum *result);

long long   *bson_int(BSON *bson, const char *name);
double      *bson_dbl(BSON *bson, const char *name);
char       **bson_str(BSON *bson, const char *name);
void        *bson_dat(BSON *bson, const char *name, bsonenum *result);
size_t       bson_len(void *ptr);
void         bson_debug_print(const BSON *bson);

bsonenum       bson_res(const BSON *bson);
const char  *bson_res_str(bsonenum res);

#ifdef __cplusplus
}
#endif

#endif
