#include "vmobj.h" 

struct BigNumber *VM_BigNumber_Create() {
    struct BigNumber *_object = VM_CreateBuiltinObject(0, sizeof(struct BigNumber) + sizeof(unsigned ));
}

struct BigNumber *VM_BigNumber_Add(struct BigNumber *_n1, struct BigNumber *_n2) {

}