#include "vmobj.h"

struct ListElement *GenerationListStart[2], *GenerationListEnd[2], *RootListStart[2], *RootListEnd[2];
// The free list
struct ListElement *FreeListStart, *FreeListEnd;

int CurrentMemorySize;

ullong GenerationMaxSize[2], GenerationSize[2];

clock_t LastLGCTime = 0, GCTimeInterval;

void VM_InitGC(ullong _generation0_max_size, ullong _generation1_max_size, clock_t _gc_time_interval) {
    LastLGCTime = clock();
    GCTimeInterval = _gc_time_interval;

    GenerationMaxSize[0] = _generation0_max_size;
    GenerationMaxSize[1] = _generation1_max_size;

    GenerationSize[0] = GenerationSize[1] = 0;

    for (int i = 0; i < 2; i++) {
        GenerationListStart[i] = malloc(sizeof(struct Object));
        GenerationListEnd[i] = malloc(sizeof(struct Object));
        List_init(GenerationListStart[i], GenerationListEnd[i]);

        RootListStart[i] = malloc(sizeof(struct Object));
        RootListEnd[i] = malloc(sizeof(struct Object));
        List_Init(RootListStart[i], RootListEnd[i]);
    }

    FreeListStart = malloc(sizeof(struct Object));
    FreeListEnd = malloc(sizeof(struct Object));
    List_Init(FreeListStart, FreeListEnd);
}

static inline ullong flag_size(ullong data_size) { return ((data_size >> 3) + 63) >> 6;}

void VM_AddReference(struct Object *_object) { _object->ReferenceCount++; }

void VM_AddRootReference(struct Object *_object) {
    _object->ReferenceCount++, _object->RootReferenceCount++;
    // if it becomes a root, then add it into the root list of the right generation
    if (_object->RootReferenceCount == 1) List_Insert(_object->RootBelong, RootListEnd[_object->Generation]);
}

void VM_ReduceReference(struct Object *_object) {
    _object->ReferenceCount--;
    if (_object->ReferenceCount == 0) VM_GC(_object);
}

void VM_ReduceRootReference(struct Object *_object) {
    _object->ReferenceCount--, _object->RootReferenceCount--;
    if (_object->ReferenceCount == 0) {
        // remove it from the root list
        List_Delete(_object->RootBelong);
        VM_GC(_object);
    }
}

struct Object *VM_CreateObject(ullong _object_size) {
    struct Object *_object = NULL;
    struct ListElement *_list_ele = NULL;
    // try to use the object of free list
    if (FreeListStart->Next != FreeListEnd)
        _object = (struct Object *)FreeListStart->Next->Content, 
        _list_ele = FreeListStart->Next,
        List_Delete(_list_ele);
    else {
        // or alloc a new object and its list element
        _object = malloc(sizeof(struct Object));
        _list_ele = malloc(sizeof(struct ListElement));
        _list_ele->Content = _object;
        _object->Belong = _list_ele;
        _object->RootBelong = malloc(sizeof(struct ListElement));
    }

    ullong _flag_size = flag_size(_object_size);
    _object->Size = _object_size;
    _object->FlagSize = _flag_size;

    _object->Data = malloc(sizeof(byte) * _object_size);
    _object->Flag = malloc(sizeof(ullong) * _flag_size);

    memset(_object->Data, 0, sizeof(byte) * _object_size);
    memset(_object->Flag, 0, sizeof(ullong) * _flag_size);

    _object->ReferenceCount = _object->RootReferenceCount = 0;
    _object->State = 1;

    // intial generation of _object is generation 0
    _object->Generation = 0;
    InsertElement(_list_ele, GenerationListEnd[0]);
    GenerationSize[0] += _object_size;

    return _object;
}

void VM_GC(struct Object *_object) {
    if (_object->ReferenceCount > 0 || _object->State == 0) return ;
    _object->State = 0;
    // remove it from the generation list and insert it into the free object list
    List_Delete(_object->Belong), List_Insert(_object->Belong, FreeListEnd);

    // scan the reference
    for (ullong i = 0; i < _object->FlagSize; i++) if (_object->Flag[i])
        for (ullong j = 0; j < 64 && (((i << 6) + j) << 3) < _object->Size; j++)
            if (_object->Flag[i] & (1ull << j)) {
                struct Object *_ref = (struct Object *)*(ullong *)_object->Data[(((i << 6) + j) << 3)];
                if (_ref == NULL) continue;
                // modify the cross reference count
                if (_object->Generation > _ref->Generation) {
                    _ref->RootReferenceCount--;
                    if (_ref->RootReferenceCount == 0) List_Delete(_ref->RootBelong);
                }
                // modity the reference count of _ref and check if it is removable
                if (_ref->ReferenceCount-- == 0) VM_GC(_ref);
            }
    free(_object->Flag), _object->Flag = NULL;
    free(_object->Data), _object->Data = NULL;
    _object->RootReferenceCount = _object->ReferenceCount = 0;
}

// move the object from generation 0 to generation 1
void move_to_generation1(struct Object *_object) {
    // modify the generation size
    GenerationSize[0] -= _object->Size, GenerationSize[1] += _object->Size;
    // modify the list element
    _object->Generation = 1;
    List_Delete(_object->Belong), List_Insert(_object->Belong, GenerationListEnd[1]);
    if (_object->RootReferenceCount) List_Delete(_object->RootBelong), List_Insert(_object->RootBelong, RootListEnd[1]);
}

inline int VM_Check() {
    return (GenerationSize[0] > GenerationMaxSize[0] || GenerationSize[1] > GenerationMaxSize[1]) && clock() - LastLGCTime > GCTimeInterval;
}
