#pragma once
#include "list.h"

enum ObjectType { ObjectType_User, ObjectType_BigNumber };

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
    int8_t Sign;
    // the length of this big number
    int32_t Length;
    // the data of this big number
    uint16_t *Data;
};

// the default max size of generation 0 is 64 MB
#define DEFAULT_GENERATION0_MAX_SIZE (6 << 20)
// the default max size of generation 1 is 1 GB
#define DEFAULT_GENERATION1_MAX_SIZE (1 << 30)
// the default gc time interval is 1*10^6 cmd
#define DEFAULT_GC_TIME_INTERVAL 100000

void VM_InitGC(uint64_t _generation0_max_size, uint64_t _generation1_max_size, uint64_t _gc_time_interval);
struct Object *VM_CreateObject(uint64_t _object_size);
struct Object *VM_CreateBuiltinObject(uint64_t _object_size, int _object_type);

#pragma region Operation of big number
struct BigNumber *VM_BigNumber_Create(int64_t _init_val);
// the function called before freeing this object
void VM_BigNumber_Free(struct BigNumber * _object);
void VM_BigNumber_Assign(struct BigNumber *_object, struct BigNumber *_a);
struct BigNumber *VM_BigNumber_Add(struct BigNumber *_a, struct BigNumber *_b);
struct BigNumber *VM_BigNumber_Minus(struct BigNumber *_a, struct BigNumber *_b);
struct BigNumber *VM_BigNumber_Mul(struct BigNumber *_a, struct BigNumber *_b);
struct BigNumber *VM_BigNumber_Div(struct BigNumber *_a, struct BigNumber *_b);
struct BigNumber *VM_BigNumber_Mod(struct BigNumber *_a, struct BigNumber *_b);
struct BigNumber *VM_BigNumber_BitAnd(struct BigNumber *_a, struct BigNumber *_b);
struct BigNumber *VM_BigNumber_BitOr(struct BigNumber *_a, struct BigNumber *_b);
struct BigNumber *VM_BigNumber_BitNot(struct BigNumber *_a, struct BigNumber *_b);
// return -1 if _a < _b, 0 if _a == _b, 1 if _a > _b
int VM_BigNumber_Compare(struct BigNumber *_a, struct BigNumber *_b);
#pragma endregion

// Check if it is necessary to run generational GC process.
int VM_Check();
// referencetype GC
void VM_ReferenceGC(struct Object* _object);
// the generational GC process
void VM_GenerationalGC();

void VM_AddCrossReference(struct Object *_object, int _data);
void VM_ReduceCrossReference(struct Object *_object, int _data);

static inline uint64_t VM_FlagAddr(uint64_t _address) { return _address >> 9; }
static inline uint64_t VM_FlagBit(uint64_t _address) { return 1llu << ((_address >> 3) - ((_address >> 9) << 6)); }