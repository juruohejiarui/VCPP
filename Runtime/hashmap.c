#include "hashmap.h"

uint64_t CalcHashKey(char *str, uint64_t range) {
    uint64_t res = 0;
    while (*str != '\0') {
        res = (res << 8) + *str; str++;
        if (res > range) res %= range;
    }
    return res;
}

char *CopyString(char *str) {
    int len = strlen(str) + 1;
    char *dst = (char *)malloc(sizeof(char) * len);
    memcpy(dst, str, sizeof(char) * len);
    return dst;
}
int StringEqual(char *s1, char *s2) {
    for (; *s1 != '\0' && *s2 != '\0'; s1++, s2++) if (*s1 != *s2) return 0;
    if (*s1 != '\0' || *s2 != '\0') return 0;
    return 1;
}
struct HashMap *HashMap_CreateMap(uint64_t range) {
    struct HashMap *map = (struct HashMap *) malloc(sizeof(struct HashMap));
    map->ElementStart = (struct HashMapElement **) malloc(sizeof(struct HashMapElement*) * range);
    map->ElementEnd = (struct HashMapElement **) malloc(sizeof(struct HashMapElement*) * range);
    map->HashRange = range;

    for (int i = 0; i < range; i++) {
        map->ElementStart[i] = (struct HashMapElement *) malloc(sizeof(struct HashMapElement));
        map->ElementEnd[i] = (struct HashMapElement *) malloc(sizeof(struct HashMapElement));
        map->ElementStart[i]->Next = map->ElementEnd[i];
        map->ElementEnd[i]->Previous = map->ElementStart[i];
        map->ElementStart[i]->Previous = map->ElementEnd[i]->Next = NULL;
    }
    return map;
}

void HashMap_Insert(struct HashMap *map, char *key, void *value) {
    uint64_t hash = CalcHashKey(key, map->HashRange);
    for (struct HashMapElement *ele = map->ElementStart[hash]->Next; ele != map->ElementEnd[hash]; ele = ele->Next)
        // find a existing one
        if (StringEqual(ele->Key, key)) { ele->Value = value; return ; }
    // create a new element
    struct HashMapElement *new_ele = (struct HashMapElement *) malloc(sizeof(struct HashMapElement));
    new_ele->Previous = map->ElementEnd[hash]->Previous;
    new_ele->Next = map->ElementEnd[hash];
    map->ElementEnd[hash]->Previous->Next = new_ele;
    map->ElementEnd[hash]->Previous = new_ele;

    new_ele->Key = CopyString(key);
    new_ele->Value = value;
}

void HashMap_Erase(struct HashMap *map, char *key, int free_value) {
    uint64_t hash = CalcHashKey(key, map->HashRange);
    for (struct HashMapElement *ele = map->ElementStart[hash]->Next; ele != map->ElementEnd[hash]; ele = ele->Next)
        // find a existing one
        if (StringEqual(ele->Key, key)) {
            ele->Previous->Next = ele->Next;
            ele->Next->Previous = ele->Previous;
            free(ele->Key);
            if (free_value) free(ele->Value);
            free(ele);
            return ;
        }
}

int HashMap_Count(struct HashMap *map, char *key) {
    uint64_t hash = CalcHashKey(key, map->HashRange);
    for (struct HashMapElement *ele = map->ElementStart[hash]->Next; ele != map->ElementEnd[hash]; ele = ele->Next)
        // find a existing one
        if (StringEqual(ele->Key, key)) return 1;
    return 0;
}

void *HashMap_Get(struct HashMap *map, char *key) {
    uint64_t hash = CalcHashKey(key, map->HashRange);
    for (struct HashMapElement *ele = map->ElementStart[hash]->Next; ele != map->ElementEnd[hash]; ele = ele->Next)
        // find a existing one
        if (StringEqual(ele->Key, key)) return ele->Value;
    return NULL;
}
