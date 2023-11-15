#pragma once
#include "list.h"

enum ObjectType { UserObject, BigNumberObject };

struct Object {
    //1: is using 0: is removed 2: waiting for check
    int State;

    // The generation of this object
    int Generation;
    // The type of this object (ObjectType)
    int Type;
    //The number of quote
    int ReferenceCount, RootReferenceCount, CrossReferenceCount;
    //the size of this object
    uint64_t Size, FlagSize;
    //The data
    uint8_t *Data;
    //The flag, the i-th bit demonstrates whethter Data[i...i+8] store an address of an object
    uint64_t *Flag;

    struct ListElement *Belong, *CrossBelong;
};

struct BigNumber {
    struct Object *Base;
    unsigned int Length;
    unsigned int Data[0];
};

// the default max size of generation 0 is 64 MB
#define DEFAULT_GENERATION0_MAX_SIZE (6 << 20)
// the default max size of generation 1 is 1 GB
#define DEFAULT_GENERATION1_MAX_SIZE (1 << 30)
// the default gc time interval is 1*10^6 cmd
#define DEFAULT_GC_TIME_INTERVAL 100000

void VM_InitGC(uint64_t _generation0_max_size, uint64_t _generation1_max_size, uint64_t _gc_time_interval);
struct Object *VM_CreateObject(uint64_t _object_size);
struct Object *VM_CreateBuiltinObject(uint64_t _object_size, size_t _struct_size);
struct BigNumber *VM_BigNumber_Create();

// Check if it is necessary to run generational GC process.
int VM_Check();
// referencetype GC
void VM_ReferenceGC(struct Object* _object);
// the generational GC process
void VM_GenerationalGC();

static inline void VM_AddReference(struct Object *_object, int _data) { _object->ReferenceCount += _data; }


static inline void VM_AddRootReference(struct Object *_object, int _data) {
    _object->RootReferenceCount += _data;
}

static inline void VM_ReduceReference(struct Object *_object, int _data) {
    _object->ReferenceCount -= _data;
    if (_object->ReferenceCount == 0) VM_ReferenceGC(_object);
}

static inline void VM_ReduceRootReference(struct Object *_object, int _data) {
    _object->RootReferenceCount -= _data;
}
void VM_AddCrossReference(struct Object *_object, int _data);
void VM_ReduceCrossReference(struct Object *_object, int _data);



static inline uint64_t VM_FlagAddr(uint64_t _address) { return _address >> 9; }
static inline uint64_t VM_FlagBit(uint64_t _address) { return 1llu << ((_address >> 3) - ((_address >> 9) << 6)); }