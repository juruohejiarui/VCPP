#include "vmobj.h"

struct Object *GenerationListStart[2], *GenerationListEnd[2], *RootListStart[2], *RootListEnd[2], *CrossListStart, *CrossListEnd;
// The free list
struct Object *FreeObjectListStart, *FreeObjectListEnd;

int CurrentMemorySize;

void InitList(struct Object *_start, struct Object *_end) {
    _start->Previous = _end->Next = NULL;
    _start->Next = _end;
    _end->Previous = _start;
}

// Insert the _element in front of _position
void InsertElement(struct Object *_element, struct Object *_position) {
    _element->Previous = _position->Previous;
    _element->Next = _position;
    _position->Previous->Next = _element;
    _position->Previous = _element;
}

// Delete the link of _element from the list
void DeleteElement(struct Object *_element) {
    _element->Previous->Next = _element->Next;
    _element->Next->Previous = _element->Previous;
    _element->Previous = _element->Next = NULL;
}

ullong GenerationMaxSize[2], GenerationSize[2];

void VM_InitGC(ullong _generation0_max_size, ullong _generation1_max_size) {
    GenerationMaxSize[0] = _generation0_max_size;
    GenerationMaxSize[1] = _generation1_max_size;

    GenerationSize[0] = GenerationSize[1] = 0;

    for (int i = 0; i < 2; i++) {
        GenerationListStart[i] = malloc(sizeof(struct Object));
        GenerationListEnd[i] = malloc(sizeof(struct Object));
        InitList(GenerationListStart[i], GenerationListEnd[i]);

        RootListStart[i] = malloc(sizeof(struct Object));
        RootListEnd[i] = malloc(sizeof(struct Object));
        InitList(RootListStart[i], RootListEnd[i]);
    }
    CrossListStart = malloc(sizeof(struct Object));
    CrossListEnd = malloc(sizeof(struct Object));
    InitList(CrossListStart, CrossListEnd);

    FreeObjectListStart = malloc(sizeof(struct Object));
    FreeObjectListEnd = malloc(sizeof(struct Object));
    InitList(FreeObjectListStart, FreeObjectListEnd);
}

static inline ullong FlagSize(ullong data_size) { return ((data_size >> 3) + 64) >> 6;}

void VM_AddReference(struct Object *_object) { _object->ReferenceCount++; }

void VM_AddRootReference(struct Object *_object) {
    _object->ReferenceCount++, _object->RootReferenceCount++;
    // if it becomes a root, then add it into the root list of the right generation
    if (_object->RootReferenceCount == 1) InsertElement(_object, RootListEnd[_object->Generation]);
}

void VM_ReduceReference(struct Object *_object) {
    _object->ReferenceCount--;
    if (_object->ReferenceCount == 0) VM_GC(_object);
}

void VM_ReduceRootReference(struct Object *_object) {
    _object->ReferenceCount--, _object->RootReferenceCount--;
    if (_object->ReferenceCount == 0) {
        // remove it from the root list
        DeleteElement(_object);
        VM_GC(_object);
    }
}

struct Object *VM_CreateObject(ullong size) {
    struct Object *_object = malloc(sizeof(struct Object));
    
    return _object;
}
