#pragma once
#include "gloconst.h"
#define HASHMAP_DEFAULT_RANGE 3109

struct HashMapElement {
    void *Value;
    char *Key; 
    struct HashMapElement *Previous, *Next;
};
struct HashMap {
    struct HashMapElement **ElementStart, **ElementEnd;
    uint64_t HashRange;
};

struct HashMap *HashMap_CreateMap(uint64_t range);
void HashMap_Insert(struct HashMap* map, char* key, void* value);
void HashMap_Erase(struct HashMap* map, char* key, int free_value);
int HashMap_Count(struct HashMap* map, char *key);
void* HashMap_Get(struct HashMap* map, char* key);