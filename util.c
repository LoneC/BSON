#include "bson.h"
#include "allocator.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>

static uint32_t murmur32_scramble(uint32_t k) {
    k *= 0xCC9E2D51;
    k = (k << 15) | (k >> 17);
    k *= 0x1B873593;
    return k;
}

static uint32_t murmur32(const uint8_t *key, uint64_t len, uint32_t seed) {
    uint32_t h = seed;
    uint32_t k;
    uint64_t i;
    for(i = len >> 2; i; i--) {
	memcpy(&k, key, sizeof(uint32_t));
	key += sizeof(uint32_t);
	h ^= murmur32_scramble(k);
	h = (h << 13) | (h >> 19);
	h = h * 5 + 0xE6546B64;
    }
    k = 0;
    for(i = len & 3; i; i--) {
	k <<= 8;
	k |= key[i - 1];
    }
    h ^= murmur32_scramble(k);
    h ^= len;
    h ^= h >> 16;
    h *= 0x85EBCA6B;
    h ^= h >> 13;
    h *= 0xC2B2AE35;
    h ^= h >> 16;
    return h;
}


uint64_t bson_hash(const char *str) {
/*
    uint64_t res = 0;
    uint8_t c;
    while((c = *str++))
	res = ((res << 5) + res) + c;
    return res;
*/
    uint64_t res = (uint64_t)(murmur32((const uint8_t *)str, strlen(str), 199933));
    return res;
}

int bson_is_whitespace(char c) {
    return (
	c ==  ' ' || c == '\n' ||
	c == '\t' || c == '\r' ||
	c == '\b'

    );
}

void bson_trim_string(char *dst, const char *src) {
    uint64_t start = 0, 
	     end = strlen(src) - 1, 
	     i;
    while(bson_is_whitespace(src[start]))         start++;
    while(bson_is_whitespace(src[end])) /* hi :) */ end--;
    for(i = 0; i < (end - start + 1); i++)
	dst[i] = src[start + i];
    dst[i] = '\0';
}

void bson_free_inner_strings(void *data) {
    char **strs = (char **)((size_t *)(data) + 1);
    uint64_t len = *((size_t *)(data));

    uint64_t i;
    for(i = 0; i < len; i++) {
	if(strs[i] != NULL)
	    bsonfree(strs[i]);
    }

}






/* End */

















