#include "vmobj.h"

// 1MB
#define MIN_RECYCLE_SIZE (1 << 20)

struct ListElement *GenerationListStart[2], *GenerationListEnd[2], *CrossListStart, *CrossListEnd;
// The free list
struct ListElement *FreeListStart, *FreeListEnd;

int CurrentMemorySize;

uint64_t GenerationMaxSize[2], GenerationSize[2];

uint64_t LastGCTime = 0, GCTimeInterval;

void VM_InitGC(uint64_t _generation0_max_size, uint64_t _generation1_max_size, uint64_t _gc_time_interval) {
    LastGCTime = clock();
    GCTimeInterval = _gc_time_interval;

    GenerationMaxSize[0] = _generation0_max_size;
    GenerationMaxSize[1] = _generation1_max_size;

    GenerationSize[0] = GenerationSize[1] = 0;

    for (int i = 0; i < 2; i++) {
        GenerationListStart[i] = malloc(sizeof(struct Object));
        GenerationListEnd[i] = malloc(sizeof(struct Object));
        List_Init(GenerationListStart[i], GenerationListEnd[i]);

    }
    CrossListStart = malloc(sizeof(struct Object));
    CrossListEnd = malloc(sizeof(struct Object));
    List_Init(CrossListStart, CrossListEnd);

    FreeListStart = malloc(sizeof(struct Object));
    FreeListEnd = malloc(sizeof(struct Object));
    List_Init(FreeListStart, FreeListEnd);
}

static inline uint64_t flag_size(uint64_t data_size) { return ((data_size >> 3) + 63) >> 6;}

inline void VM_AddCrossReference(struct Object *_object, int _data) {
    if (_object->CrossReferenceCount == 0) List_Insert(_object->CrossBelong, CrossListEnd);
    _object->CrossReferenceCount += _data;
}
inline void VM_ReduceCrossReference(struct Object *_object, int _data) {
    _object->CrossReferenceCount -= _data;
    if (_object->CrossReferenceCount == 0) List_Delete(_object->CrossBelong);
}

struct Object *get_object(uint64_t _object_size, size_t _struct_size) {
    struct Object *_object = NULL;
    uint64_t _flag_size = flag_size(_object_size);
    // try to use the object of free list
    if (FreeListStart->Next != FreeListEnd) {
        _object = (struct Object *)FreeListStart->Next->Content,
        _object = realloc(_object, _struct_size);
        if (_object->Size >= MIN_RECYCLE_SIZE) 
            _object->Data = malloc(sizeof(uint8_t) * _object_size),
            _object->Flag = malloc(sizeof(uint64_t) * _flag_size);
        else 
            _object->Data = realloc(_object->Data, sizeof(uint8_t) * _object_size), 
            _object->Flag = realloc(_object->Flag, sizeof(uint64_t) * _flag_size);
        List_Delete(_object->Belong);
    } else {
        // or alloc a new object and its list element
        _object = malloc(_struct_size);
        // set the list elements
        _object->Belong = malloc(sizeof(struct ListElement));
        _object->CrossBelong = malloc(sizeof(struct ListElement));
        _object->CrossBelong->Content = _object->Belong->Content = _object;

        _object->Data = malloc(sizeof(uint8_t) * _object_size);
        _object->Flag = malloc(sizeof(uint64_t) * _flag_size);

        memset(_object->Data, 0, sizeof(uint8_t) * _object_size);
        memset(_object->Flag, 0, sizeof(uint64_t) * _flag_size);
    }

    _object->Size = _object_size;
    _object->FlagSize = _flag_size;

    

    return _object;
}

struct Object *VM_CreateObject(uint64_t _object_size) {
    struct Object *_object = get_object(_object_size, sizeof(struct Object));

    _object->ReferenceCount = _object->RootReferenceCount = _object->CrossReferenceCount = 0;
    _object->State = 1;

    _object->Type = ObjectType_User;

    // intial generation of _object is generation 0
    _object->Generation = 0;
    List_Insert(_object->Belong, GenerationListEnd[0]);
    GenerationSize[0] += _object_size;

    return _object;
}

const size_t ObjectSize[] = {sizeof(struct Object), sizeof(struct BigNumber)};

struct Object *VM_CreateBuiltinObject(uint64_t _object_size, int _object_type) {
    struct Object *_object = get_object(_object_size, ObjectSize[_object_type]);

    _object->ReferenceCount = _object->RootReferenceCount = _object->CrossReferenceCount = 0;
    _object->State = 1;

    _object->Type = _object_type;

    // intial generation of _object is generation 0
    _object->Generation = 0;
    List_Insert(_object->Belong, GenerationListEnd[0]);
    GenerationSize[0] += _object_size;

    return _object;
}


void free_object(struct Object *_object) {
    _object->State = 0;
    if (_object->Type == ObjectType_BigNumber) VM_BigNumber_Free((struct BigNumber *)_object);
    List_Delete(_object->Belong), List_Insert(_object->Belong, FreeListEnd);
    if (_object->CrossReferenceCount) List_Delete(_object->CrossBelong);
    if (_object->Size > MIN_RECYCLE_SIZE) free(_object->Flag), free(_object->Data);
}

void VM_ReferenceGC(struct Object *_object) {
    if (_object->ReferenceCount > 0 || _object->State == 0) return ;
    _object->State = 0;

    // scan the reference
    for (uint64_t i = 0; i < _object->FlagSize; i++) if (_object->Flag[i])
        for (uint64_t j = 0; j < 64 && (((i << 6) + j) << 3) < _object->Size; j++)
            if (_object->Flag[i] & (1ull << j)) {
                struct Object *_ref = (struct Object *)*(uint64_t *)&_object->Data[((i << 6) + j) << 3];
                if (_ref == NULL) continue;
                // modify the cross reference count
                if (_object->Generation > _ref->Generation) VM_ReduceCrossReference(_ref, 1);
                // modity the reference count of _ref and check if it is removable
                if (_ref->ReferenceCount-- == 0) VM_ReferenceGC(_ref);
            }
    free_object(_object);
}

uint64_t gc_tick; 

inline int VM_Check() {
    return (GenerationSize[0] > GenerationMaxSize[0] || GenerationSize[1] > GenerationMaxSize[1]) && ++gc_tick > GCTimeInterval;
}

void scan_object(struct Object *_object, int _max_generation) {
    if (_object->State == 1) return ;
    _object->State = 1;
     for (uint64_t i = 0; i < _object->FlagSize; i++) if (_object->Flag[i])
        for (uint64_t j = 0; j < 64 && (((i << 6) + j) << 3) < _object->Size; j++)
            if (_object->Flag[i] & (1ull << j)) {
                struct Object *_ref = (struct Object *)*(uint64_t *)&_object->Data[(((i << 6) + j) << 3)];
                if (_ref == NULL || _ref->Generation > _max_generation) continue;
                scan_object(_ref, _max_generation);
            }
}

// the generational GC process
void VM_GenerationalGC() {
    // printf("Generation GC... gc_tick = %llu ", gc_tick);
    
    // scan the generation 0    
    for (struct ListElement *_element = GenerationListStart[0]->Next; _element != GenerationListEnd[0]; _element = _element->Next)
        ((struct Object *)_element->Content)->State = 2;
    for (struct ListElement *_element = GenerationListStart[0]->Next; _element != GenerationListEnd[0]; _element = _element->Next)
        if (((struct Object *)_element->Content)->RootReferenceCount > 0) scan_object((struct Object *)_element->Content, 0);
    for (struct ListElement *_element = CrossListStart->Next; _element != CrossListEnd; _element = _element->Next)
        scan_object((struct Object *)_element->Content, 0);
    // find if the element need to be free
    // the _element->Next may be change after free_object process.
    for (struct ListElement *_element = GenerationListStart[0]->Next, *_next; _element != GenerationListEnd[0]; _element = _next) {
        _next = _element->Next;
        if (((struct Object *)_element->Content)->State == 2)
            free_object((struct Object *)_element->Content);
    }
    // move all the remaining objects into generation 1
    // clean the cross list and cross reference
    for (struct ListElement *_element = CrossListStart->Next; _element != CrossListEnd; _element = _element->Next) {
        struct Object *_object = (struct Object *)_element->Content;
        _object->CrossReferenceCount = 0;
    }
    List_Clean(CrossListStart, CrossListEnd);
    for (struct ListElement *_element = GenerationListStart[0]->Next, *_next; _element != GenerationListEnd[0]; _element = _next) {
        _next = _element->Next;
        List_Delete(_element), List_Insert(_element, GenerationListEnd[1]);
        struct Object *_object = (struct Object *)_element->Content;
        GenerationSize[1] += _object->Size;
        _object->Generation = 1;
    }
    GenerationSize[0] = 0;
    
    // check if it is necessary to scan generation 1
    if (GenerationSize[1] > GenerationMaxSize[1] && gc_tick > GCTimeInterval * (GenerationMaxSize[1] / GenerationMaxSize[0])) {
        // scan all the object 
        for (struct ListElement *_element = GenerationListStart[1]->Next; _element != GenerationListEnd[1]; _element = _element->Next)
            ((struct Object *)_element->Content)->State = 2;
        for (struct ListElement *_element = GenerationListStart[1]->Next; _element != GenerationListEnd[1]; _element = _element->Next)
            if (((struct Object *)_element->Content)->RootReferenceCount > 0)
                scan_object((struct Object *)_element->Content, 1);
        for (struct ListElement *_element = GenerationListStart[1]->Next, *_next; _element != GenerationListEnd[1]; _element = _next) {
            _next = _element->Next;
            if (((struct Object *)_element->Content)->State == 2)
                free_object((struct Object *)_element->Content);
        }
        gc_tick = 0;
    }
    // printf("end...\n");
}