#include "bson.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "allocator.h"
#include "util.h"

#define MAX_ELEMENTS   32
#define MORE_STACK    256
#define MORE_LEFT      64
#define MORE_RIGHT    128
#define MORE_ARRAY     32

/*    ELEMENT   */

typedef struct _s_element_t {
    char                *name;
    void                *data;
    bsonenum               type;
    struct _s_element_t *next;
} element_t;

/*              */


/* HASHED CONFIG */

static bsonenum begin_read(BSON *bson);
struct _s_BSON {
    char        *filename;
    uint64_t     elementsmax;
    element_t  **elements;
};

BSON *bson_open(const char *filepath, bsonenum *result) {
    BSON *bson = bsoncalloc(1, sizeof(BSON));
    if(bson == NULL) {
	*result = BSON_MEMORY;
	return NULL;
    }
    bson->filename = bsonstrdup(filepath);
    bson->elementsmax = MAX_ELEMENTS;
    bson->elements = bsoncalloc(bson->elementsmax, sizeof(element_t *));
    if(bson->filename == NULL || bson->elements == NULL) {
	bson_free(&bson, NULL);
	*result = BSON_MEMORY;
	return NULL;
    }

    bsonenum ret = begin_read(bson);
    if(ret != BSON_SUCCESS) {
	if(result != NULL)
	    *result = ret;
	return NULL;
    }
    
    *result = BSON_SUCCESS;
    return bson;
}

void bson_free(BSON **bson, bsonenum *result) {
    if(bson == NULL || *bson == NULL) {
	if(result != NULL)
	    *result = BSON_NULL_PTR;
	return;
    }

    if((*bson)->filename != NULL) 
	bsonfree((*bson)->filename);
    
    char **start;
    uint64_t i, j;
    size_t len;
    if((*bson)->elements != NULL) {
	for(i = 0; i < (*bson)->elementsmax; i++) {
	    element_t *cur = (*bson)->elements[i];
	    element_t *tmp;
	    while(cur != NULL) {
		tmp = cur->next;
		if(cur->type == BSON_STR) {
		    len = *((size_t *)(cur->data));
		    start = (char **)((size_t *)(cur->data) + 1);
		    for(j = 0; j < len; j++)
			bsonfree(start[j]);
		}
		bsonfree(cur->name);
		bsonfree(cur->data);
		bsonfree(cur);
		cur = tmp;
	    }
	}
	bsonfree((*bson)->elements);
    }

    bsonfree(*bson);
    *bson = NULL;
    if(result != NULL)
	*result = BSON_SUCCESS;
}

long long *bson_int(BSON *bson, const char *name) {
    element_t *cur = bson->elements[bson_hash(name) % bson->elementsmax];
    while(cur != NULL) {
	if(strcmp(name, cur->name) == 0)
	    return (long long *)((size_t *)(cur->data) + 1);
	cur = cur->next;
    }
    return NULL;
}

double *bson_dbl(BSON *bson, const char *name) {
    element_t *cur = bson->elements[bson_hash(name) % bson->elementsmax];
    while(cur != NULL) {
	if(strcmp(name, cur->name) == 0)
	    return (double *)((size_t *)(cur->data) + 1);
	cur = cur->next;
    }
    return NULL;
}

char **bson_str(BSON *bson, const char *name) {
    element_t *cur = bson->elements[bson_hash(name) % bson->elementsmax];
    while(cur != NULL) {
	if(strcmp(name, cur->name) == 0)
	    return (char **)((size_t *)(cur->data) + 1);
	cur = cur->next;
    }
    return NULL;
}

void *bson_dat(BSON *bson, const char *name, bsonenum *result) {
    assert("unimplemented" && 0);
    return NULL;
}

size_t bson_len(void *ptr) {
    return *((size_t *)(ptr) - 1);
}

const char *bson_res_str(bsonenum res) {
    switch(res) {
	case BSON_SUCCESS:  	  return "Result[SUCCESS]";         break;
	case BSON_FILE_PATH: 	  return "Result[FILE PATH]";       break;
	case BSON_MEMORY:   	  return "Result[MEMORY]";          break;
	case BSON_NULL_PTR: 	  return "Result[NULL POINTER]";    break;
	case BSON_SYNTAX:     	  return "Result[SYNTAX ERROR]";    break;
	case BSON_INVALID_VALUE:  return "Result[INVALID VALUE]";   break;
	case BSON_NOT_FOUND:      return "Result[ENTRY NOT FOUND]"; break;
	case BSON_CONTINUE: 	  return "Result[CONTINUE]";        break;
	case BSON_INT:      	  return "Enum[INTEGER]";           break;
	case BSON_DBL:      	  return "Enum[DOUBLE]";            break;
	case BSON_STR:      	  return "Enum[STRING]";            break;
	case BSON_MAX:      	  return "Value[BSON_MAX]";         break;
    }
    return "<invalid-enum>";
}

/*               */


/* READ CONTEXT */

typedef struct {
    uint64_t   stackmax;
    char      *stack;
    uint64_t   leftmax;
    char      *left;
    uint64_t   rightmax;
    char      *right;
    char       middle[32]; /* Middle should be no more than one char */
    FILE      *file;
    uint64_t   lines;
} ReadContext;

static bsonenum read(BSON *bson, ReadContext *ctx);
static bsonenum begin_read(BSON *bson) {
    ReadContext ctx;
    memset(&ctx, 0, sizeof(ReadContext));
    
    ctx.file = fopen(bson->filename, "r");
    if(ctx.file == NULL)
    	return BSON_FILE_PATH;

    ctx.stackmax  = MORE_STACK;
    ctx.stack     = bsonmalloc(ctx.stackmax);
    ctx.leftmax   = MORE_LEFT;
    ctx.left      = bsonmalloc(ctx.leftmax);
    ctx.rightmax  = MORE_RIGHT;
    ctx.right     = bsonmalloc(ctx.rightmax);
    
    if(ctx.stack == NULL || ctx.left == NULL || ctx.right == NULL) {
	if(ctx.stack != NULL) bsonfree(ctx.stack);
	if(ctx.left  != NULL) bsonfree(ctx.left);
	if(ctx.right != NULL) bsonfree(ctx.right);
	fclose(ctx.file);
	return BSON_MEMORY;
    }

    ctx.stack[0] = '\0';
    bsonenum ret = read(bson, &ctx);
    bsonfree(ctx.stack);
    bsonfree(ctx.left);
    bsonfree(ctx.right);
    fclose(ctx.file);
    return ret;
}

static bsonenum ctx_push(ReadContext *ctx) {
    uint64_t stacklen = strlen(ctx->stack);
    uint64_t leftlen = strlen(ctx->left);
    if(stacklen + leftlen + 2 > ctx->stackmax) {
	void *tptr = bsonrealloc(ctx->stack, (ctx->stackmax += MORE_STACK));
	if(tptr == NULL)
	    return BSON_MEMORY;
	ctx->stack = tptr;
    }
    if(stacklen > 0) {
	ctx->stack[stacklen] = '.';
	ctx->stack[stacklen + 1] = '\0';
	strcat(ctx->stack, ctx->left);
    }
    else
	strcpy(ctx->stack, ctx->left);

    return BSON_SUCCESS;
}

static bsonenum ctx_pop(ReadContext *ctx) {
    uint64_t i = strlen(ctx->stack) - 1;
    while(ctx->stack[i] != '.' && i != 0)
	i--;
    ctx->stack[i] = '\0';

    return BSON_SUCCESS;
}

/*              */


/*    READING    */

#define eof     (feof(ctx->file))
static bsonenum skip_ignored(ReadContext *ctx);
static bsonenum check_pop(ReadContext *ctx);
static bsonenum read_left(ReadContext *ctx);
static bsonenum read_middle(ReadContext *ctx);
static bsonenum read_right(ReadContext *ctx);
static bsonenum save_right(BSON *bson, ReadContext *ctx);

#define RETCASE        				\
switch(ret) {                      		\
    case BSON_CONTINUE: continue;   break;	\
    case BSON_SUCCESS:              break;  	\
    default:            return ret; break;	\
}

static bsonenum read(BSON *bson, ReadContext *ctx) {
    bsonenum ret;
    while(!eof) {
	ret = skip_ignored(ctx); RETCASE
	ret = check_pop(ctx);    RETCASE
	ret = read_left(ctx);    RETCASE
	ret = skip_ignored(ctx); RETCASE
	ret = read_middle(ctx);  RETCASE
	ret = skip_ignored(ctx); RETCASE
	ret = read_right(ctx);   RETCASE
	if(strlen(ctx->left) == 0 || strlen(ctx->right) == 0)
	    continue;
	ret = save_right(bson, ctx);
	if(ret != BSON_SUCCESS)
	    return ret;
    }
    bson_debug_print(bson);
    return BSON_SUCCESS;
}

/*               */


/* READING FUNCS */

static bsonenum skip_ignored(ReadContext *ctx) {
    if(eof)
	return BSON_SUCCESS;
    uint64_t prev = ftell(ctx->file);
    char pc = 0, c = 0;
    do {
	prev = ftell(ctx->file);
	pc   = c;
	c    = fgetc(ctx->file);
	if(c == '/') {
	    prev = ftell(ctx->file);
	    pc   = c;
	    c    = fgetc(ctx->file);
	}
	else if(c == '\n')
	    ctx->lines++;
	if(c == '/' && pc == '/') {
	    do {
		prev = ftell(ctx->file);
		pc   = c;
		c    = fgetc(ctx->file);
	    } while(!eof && c != '\n');
	}
    } while(!eof && bson_is_whitespace(c));
    fseek(ctx->file, prev, SEEK_SET);
    return BSON_SUCCESS;
}

static bsonenum check_pop(ReadContext *ctx) {
    if(eof)
	return BSON_SUCCESS;
    uint64_t prev = ftell(ctx->file);
    int popped = 0;
    char c;
    do {
	c = fgetc(ctx->file);
	if(c == '}') {
	    popped = 1;
	    break;
	}
    } while(!eof && !bson_is_whitespace(c));
    if(!popped) {
	fseek(ctx->file, prev, SEEK_SET);
	return BSON_SUCCESS;
    }
    ctx_pop(ctx);
    return BSON_CONTINUE;
}

static bsonenum read_left(ReadContext *ctx) {
    if(eof)
	return BSON_SUCCESS;
    char c;
    uint64_t i = 0;
    do {
	c = fgetc(ctx->file);
	ctx->left[i++] = c;
	if(i >= ctx->leftmax) {
	    void *tptr = bsonrealloc(ctx->left, (ctx->leftmax + MORE_LEFT));
	    if(tptr == NULL)
		return BSON_MEMORY;
	    ctx->left = tptr;
	}
    } while(!eof && !bson_is_whitespace(c));
    ctx->left[i] = '\0';
    bson_trim_string(ctx->left, ctx->left);
    return BSON_SUCCESS;
}

static bsonenum read_middle(ReadContext *ctx) {
    if(eof)
	return BSON_SUCCESS;
    bsonenum ret;
    char c = fgetc(ctx->file);
    switch(c) {
	case '{':
	    ret = ctx_push(ctx);
	    if(ret != BSON_SUCCESS)
		return ret;
	    return BSON_CONTINUE;
	    break;
	case '=':
	    return BSON_SUCCESS;
	    break;
    }
    return BSON_SYNTAX;
}

static bsonenum read_right(ReadContext *ctx) {
    if(eof)
	return BSON_SUCCESS;
    char c;
    uint64_t i = 0;
    do {
	c = fgetc(ctx->file);
	ctx->right[i++] = c;
	if(i >= ctx->rightmax) {
	    ctx->right = bsonrealloc(ctx->right, (ctx->rightmax += MORE_RIGHT));
	    if(ctx->right == NULL)
		return BSON_MEMORY;
	}
    } while(!eof && c != '\n'); /* TODO: FIND A WAY FOR MULTI LINE DATA */
    ctx->right[i] = '\0';
    bson_trim_string(ctx->right, ctx->right);
    return BSON_SUCCESS;
}

static void *get_strings(const char *src, bsonenum *type);
static void *get_string(const char *src, bsonenum *type);
static void *get_numbers(const char *src, bsonenum *type);
static void *get_number(const char *src, bsonenum *type);
static bsonenum add_element_to_bson(BSON *bson, ReadContext *ctx, void *data, bsonenum type);

static bsonenum save_right(BSON *bson, ReadContext *ctx) {
    if(strlen(ctx->right) == 0)
	return BSON_SUCCESS;
    bsonenum   type;
    void    *data;
    switch(ctx->right[0]) {
	case '[': {
	    uint64_t i;
	    int quotes = 0;
	    for(i = 0; ctx->right[i]; i++) {
		if(ctx->right[i] == '"') {
		    quotes = 1;
		    break;
		}
	    }
	    data = quotes ?
		   get_strings(ctx->right, &type) :
		   get_numbers(ctx->right, &type);
	} break;
	case '"':
	    data = get_string(ctx->right, &type);
	    break;
	default: 
	    data = get_number(ctx->right, &type);
	    break;
    }
    if(data == NULL)
	return BSON_SYNTAX;
    return add_element_to_bson(bson, ctx, data, type);
}


static bsonenum add_element_to_bson(BSON *bson, ReadContext *ctx, void *data, bsonenum type) {
    char *name;
    if(strlen(ctx->stack) == 0) {
	name = bsonstrdup(ctx->left);
	if(name == NULL)
	    return BSON_MEMORY;
    }
    else {
	name = bsonmalloc(strlen(ctx->stack) + strlen(ctx->left) + 2);
	if(name == NULL)
	    return BSON_MEMORY;
	strcpy(name, ctx->stack);
	name[strlen(name)]     =  '.';
	name[strlen(name) + 1] = '\0';
	strcat(name, ctx->left);
    }

    element_t *e = bsoncalloc(1, sizeof(element_t));
    if(e == NULL) {
	bsonfree(name);
	return BSON_MEMORY;
    }
    
    e->name = name;
    e->data = data;
    e->type = type;

    uint64_t loc = bson_hash(e->name) % bson->elementsmax;
    // printf(" >> GOING TO LOCATION (%lu)\n", loc);
    if(bson->elements[loc] == NULL) {
	bson->elements[loc] = e;
	return BSON_SUCCESS;
    }
    
    element_t *cur = bson->elements[loc];
    while(cur != NULL) {
	if(strcmp(cur->name, e->name) == 0) {
	    bsonfree(cur->name);
	    bsonfree(cur->data);
	    cur->name = e->name;
	    cur->data = e->data;
	    cur->type = e->type;
	    bsonfree(e);
	    return BSON_SUCCESS;
	}
	if(cur->next == NULL) {
	    cur->next = e;
	    return BSON_SUCCESS;
	}
	cur = cur->next;
    }

    return BSON_SUCCESS;
}

/*               */


/*    PARSING    */


static void *get_strings(const char *src, bsonenum *type) {
    const char *first = src + 1;
    while(*first && bson_is_whitespace(*first))
	first++;
    if(*first != '"') {
	*type = BSON_SYNTAX;
	return NULL;
    }

    uint64_t i = strlen(first) - 1, last;
    int valid = 0;
    while(i != 0) {
	if(first[i] == ']') {
	    valid = 1;
	    break;
	}
	i--;
    }
    last = i;
    if(!valid) {
	*type = BSON_SYNTAX;
	return NULL;
    }
    
    first++; /* We know the first char is a quote */
    uint64_t datamax = sizeof(char *) * MORE_ARRAY + sizeof(size_t);
    uint64_t datalen = 0;
    void    *data = bsonmalloc(datamax);
    char   **strs = (char **)((size_t *)(data) + 1);
    uint64_t stringmax;
    char *string;
#define MORE_STRING 64
    while(*first) {
	i = 0;
	stringmax = MORE_STRING;
	string = bsoncalloc(stringmax, 1);
	valid = 0;
	while(*first && *first != '"') {
	    string[i] = *first;
	    i++;
	    if(i >= stringmax) {
		void *tptr = bsonrealloc(string, (stringmax += MORE_STRING));
		if(tptr == NULL) {
		    bsonfree(string);
		    bson_free_inner_strings(data);
		    bsonfree(data);
		    *type = BSON_MEMORY;
		    return NULL;
		}
		string = tptr;
	    }

	    first++;
	}
	/* Since calloc, we good */
	strs[datalen++] = string;
	if(datalen >= datamax) {
	    void *tptr = bsonrealloc(data, (datamax += MORE_ARRAY * sizeof(char *)));
	    if(tptr == NULL) {
		bson_free_inner_strings(data);
		bsonfree(data);
		*type = BSON_MEMORY;
		return NULL;
	    }
	    data = tptr;
	    strs = (char **)((size_t *)(data) + 1);
	}


	first++;
	while(bson_is_whitespace(*first))
	    first++;
	if(*first == ']')
	    break;
	if(*first != ',') {
	    bson_free_inner_strings(data);
	    *type = BSON_SYNTAX;
	    return NULL;
	}
	
	do first++; while(bson_is_whitespace(*first));
	if(*first != '"') {
	    bson_free_inner_strings(data);
	    bsonfree(data);
	    *type = BSON_SYNTAX;
	    return NULL;
	}
	first++;
    }
    
    *type = BSON_STR;
    *((size_t *)(data)) = datalen;
    return data;
}

static void *get_string(const char *src, bsonenum *type) {
    const char *first = src + 1;
    uint64_t i = strlen(src) - 1, last;
    int valid = 0;
    while(i != 0) {
	if(first[i] == '"') {
	    valid = 1;
	    last = i;
	    break;
	}
	i--;
    }
    last = i;

    if(!valid) {
	*type = BSON_SYNTAX;
	return NULL;
    }
    
    char **data = bsonmalloc(sizeof(char *) + sizeof(size_t));
    if(data == NULL) {
	*type = BSON_MEMORY;
	return NULL;
    }
   
    *((size_t *)(data)) = 1;
    char **start = (char **)((size_t *)(data) + 1);
    (*start) = bsonmalloc(last + 2);

    for(i = 0; i < last; i++)
	start[0][i] = first[i];
    start[0][i] = '\0';

    *type = BSON_STR;
    return (void *)data;
}
    
union number { long long d; double f; };
static void *get_numbers(const char *src, bsonenum *type) {
    const char *first = src + 1;
    while(*first && bson_is_whitespace(*first))
	first++;

    uint64_t i = strlen(first) - 1, last;
    int valid = 0;
    while(i != 0) {
	if(first[i] == ']') {
	    valid = 1;
	    break;
	}
	i--;
    }
    last = i;
    if(!valid) {
	*type = BSON_SYNTAX;
	return NULL;
    }

    uint64_t datamax = sizeof(long long) * MORE_ARRAY + sizeof(size_t);
    uint64_t datalen = 0;
    void *data = bsonmalloc(datamax);
    union number *start = (union number *)((size_t *)(data) + 1);
    
    char *end;
    *type = BSON_INT;
    start[datalen].d = strtoll(first, &end, 10);
    if(first == end) {
	bsonfree(data);
	*type = BSON_SYNTAX;
	return NULL;
    }
    if(*end == '.') {
	start[datalen].f = strtof(first, &end);
	*type = BSON_DBL;
	if(first == end) {
	    bsonfree(data);
	    *type = BSON_SYNTAX;
	    return NULL;
	}
    }
    datalen++;

    while(bson_is_whitespace(*end))
	end++;
    
    if(*end == ']')
	goto done; /* LMAO */
    if(*end != ',') {
	bsonfree(data);
	*type = BSON_SYNTAX;
	return NULL;
    }

    do end++; while(bson_is_whitespace(*end));

    while(*end) {
	first = end;
	if(*type == BSON_DBL)
	    start[datalen].f = strtod(first, &end);
	else
	    start[datalen].d = strtoll(first, &end, 10);
	datalen++;
	if(datalen >= datamax) {
	    void *tptr = realloc(data, (datamax += sizeof(long long) * MORE_ARRAY));
	    if(tptr == NULL) {
		bsonfree(data);
		*type = BSON_MEMORY;
		return NULL;
	    }
	    data = tptr;
	}

	if(first == end || *end == '.') {
	    bsonfree(data);
	    *type = BSON_SYNTAX;
	    return NULL;
	}
	
	while(bson_is_whitespace(*end))
	    end++;
	
	if(*end == ']')
	    break;

	if(*end != ',') {
	    bsonfree(data);
	    *type = BSON_SYNTAX;
	    return NULL;
	}
	
	do end++; while(bson_is_whitespace(*end));
    }

done:
    *((size_t *)(union intdbl *)(data)) = datalen;
    return data;
}

static void *get_number(const char *src, bsonenum *type) {
    union number intdbl;
	
    char *end;
    char *target;
    uint64_t i = strlen(src), last;
    while(bson_is_whitespace(src[i]) && i != 0)
	i--;
    last = i;
    
    target = bsonmalloc(last);
    for(i = 0; i < last; i++)
	target[i] = src[i];
    target[i] = '\0';
    
    
    intdbl.d = strtoll(target, &end, 10);
    *type = BSON_INT;
    if(target == end) {
	bsonfree(target);
	*type = BSON_SYNTAX;
	return NULL;
    }

    if(*end == '.') {
	intdbl.f = strtod(target, &end);
	*type = BSON_DBL;
	if(target == end) {
	    bsonfree(target);
	    *type = BSON_SYNTAX;
	    return NULL;
	}
    }
    bsonfree(target);
    
    void *data = bsonmalloc(sizeof(size_t) + sizeof(intdbl));
    *((size_t *)(data)) = 1;
    memcpy((size_t *)(data) + 1, &intdbl, sizeof(intdbl));

    return data;
}

/*               */


/* DEBUG, IGNORE */

static void dpristr(char **data) {
    uint64_t i, len;
    len = *((size_t *)(data));
    char **start = (char **)((size_t *)(data) + 1);
    printf("%lu str [ ", len);
    for(i = 0; i < len; i++) {
	printf("<%s>", start[i]);
	if(i != len - 1)
	    printf(", ");
	else printf(" ");
    }
    printf("]\n");
}

static void dpriint(long long *data) {
    uint64_t i, len;
    len = *((size_t *)(data));
    long long *start = (long long *)((size_t *)(data) + 1);
    printf("%lu int [ ", len);
    for(i = 0; i < len; i++) {
	printf("%lld", start[i]);
	if(i != len - 1)
	    printf(", ");
	else printf(" ");
    }
    printf("]\n");
}

static void dpridbl(double *data) {
    uint64_t i, len;
    len = *((size_t *)(data));
    double *start = (double *)((size_t *)(data) + 1);
    printf("%lu dbl [ ", len);
    for(i = 0; i < len; i++) {
	printf("%lf", start[i]);
	if(i != len - 1)
	    printf(", ");
	else printf(" ");
    }
    printf("]\n");
}

void bson_debug_print(const BSON *bson) {
    uint64_t i;
    for(i = 0; i < bson->elementsmax; i++) {
	printf("%3lu:", i);
	element_t *e = bson->elements[i];
	if(e == NULL) {
	    printf("\t<Null>\n");
	    continue;
	}
	do {
	    printf("\t\"%s\"\t\t\t= ", e->name);
	    switch(e->type) {
		case BSON_STR:
		    dpristr(e->data);
		    break;
		case BSON_INT:
		    dpriint(e->data);
		    break;
		case BSON_DBL:
		    dpridbl(e->data);
		    break;
		default:
		    printf("\t<Invalid>\n");
		    break;
	    }
	    e = e->next;
	} while(e != NULL);
    }
}

/*               */


/* END */
