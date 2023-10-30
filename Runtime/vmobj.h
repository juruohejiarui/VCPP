#pragma once
#include "gloconst.h"

enum ThreadState {
    ThreadState_Waiting, ThreadState_Running, ThreadState_Pause, ThreadState_Exit
};
extern volatile enum ThreadState VMThreadState, GCThreadState;
struct VM_ObjectInfo {
    //1: is using 0: is removed 2: waiting for check
    int State;
    //The number of quote
    int QuoteCount, VarQuoteCount;
    //1: the size of this object
    ullong Size, FlagSize;
    //The data
    byte *Data;
    //The flag, the i-th bit demonstrates whethter Data[i...i+8] store an address of an object
    ullong *Flag;
    
   struct VM_ObjectInfo *Previous, *Next;
};
void VM_InitGC();
struct VM_ObjectInfo* VM_CreateObjectInfo(ullong size);
void VM_GC(struct VM_ObjectInfo* obj);
void *VM_GCThread(void *arg);

static inline ullong VM_ObjectInfo_FlagAddr(ullong addr) { return addr >> 9; }
static inline ullong VM_ObjectInfo_FlagBit(ullong addr) { return 1llu << ((addr >> 3) - ((addr >> 9) << 6)); }