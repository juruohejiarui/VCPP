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
    ullong Size, FlagSize;
    //The data
    byte *Data;
    //The flag, the i-th bit demonstrates whethter Data[i...i+8] store an address of an object
    ullong *Flag;

    struct ListElement *Belong, *RootBelong;
};

struct BigNumber {
    struct Object Base;
    unsigned int Length;
    unsigned int Data[0];
};

// the default max size of generation 0 is 64 MB
#define DEFAULT_GENERATION0_MAX_SIZE (6 << 20)
// the default max size of generation 1 is 1 GB
#define DEFAULT_GENERATION1_MAX_SIZE (1 << 30)
// the default gc time interval is 500(ms)
#define DEFAULT_GC_TIME_INTERVAL (500000)

void VM_InitGC(ullong _generation0_max_size, ullong _generation1_max_size, clock_t _gc_time_interval);
struct Object* VM_CreateObject(ullong _object_size);
// struct Object* VM_CreateBigNumber();

// referencetype GC
void VM_GC(struct Object* _object);

// Check if it is necessary to run generational GC process.
int VM_Check();
// the generational GC process
void VM_GenerationalGC();

void VM_AddReference(struct Object *_object);
void VM_AddRootReference(struct Object *_object);
void VM_ReduceReference(struct Object *_object);
void VM_ReduceRootReference(struct Object *_object);

static inline ullong VM_FlagAddr(ullong _address) { return _address >> 9; }
static inline ullong VM_FlagBit(ullong _address) { return 1llu << ((_address >> 3) - ((_address >> 9) << 6)); }