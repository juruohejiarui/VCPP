#include "vmobj.h"

volatile enum ThreadState VMThreadState, GCThreadState;
struct VM_ObjectInfo *ObjectListStart, *ObjectListEnd, *FreeObjectListStart, *FreeObjectListEnd;

int CurrentMemorySize;

void InitList(struct VM_ObjectInfo *start, struct VM_ObjectInfo *end) {
    start->Previous = end->Next = NULL;
    start->Next = end;
    end->Previous = start;
}
void InsertElement(struct VM_ObjectInfo *element, struct VM_ObjectInfo *p) {
    element->Previous = p->Previous;
    element->Next = p;
    p->Previous->Next = element;
    p->Previous = element;
}
void DeleteElement(struct VM_ObjectInfo *element) {
    element->Previous->Next = element->Next;
    element->Next->Previous = element->Previous;
    element->Previous = element->Next = NULL;
}


void VM_InitGC() {
    ObjectListStart = malloc(sizeof(struct VM_ObjectInfo));
    ObjectListEnd = malloc(sizeof(struct VM_ObjectInfo));
    FreeObjectListStart = malloc(sizeof(struct VM_ObjectInfo));
    FreeObjectListEnd = malloc(sizeof(struct VM_ObjectInfo));

    InitList(ObjectListStart, ObjectListEnd);
    InitList(FreeObjectListStart, FreeObjectListEnd);
}

static inline uulong FlagSize(uulong data_size) { return ((data_size >> 3) + 64) >> 6;}

struct VM_ObjectInfo *VM_CreateObjectInfo(uulong size) {
    // printf("Create an object size = %llu\n", size);
    struct VM_ObjectInfo *element;
    // need to malloc a new object
    if (FreeObjectListStart->Next == FreeObjectListEnd) {
        element = malloc(sizeof(struct VM_ObjectInfo));
    } else element = FreeObjectListEnd->Previous, DeleteElement(element);
    InsertElement(element, ObjectListEnd);
    element->State = 1;
    element->QuoteCount = element->VarQuoteCount = 0;
    element->Size = size;
    element->FlagSize = FlagSize(size);
    element->Flag = malloc(sizeof(uulong) * element->FlagSize);
    element->Data = malloc(size);
    memset(element->Data, 0, sizeof(vbyte) * size);
    memset(element->Flag, 0, sizeof(uulong) * element->FlagSize);

    CurrentMemorySize += size;
    return element;
}

void GC(struct VM_ObjectInfo *obj) {
    if (obj->QuoteCount) return ;
    DeleteElement(obj);
    InsertElement(obj, FreeObjectListEnd);
    obj->State = 0;
    CurrentMemorySize -= obj->Size;
    //Scan [i, i + 63]
    // printf("GC... %llx\n", (uulong)obj);
    for (uulong i = 0, shift = 0; i < obj->FlagSize; i++, shift += 64)
        if (*(obj->Flag + i))
            for (uulong j = 0; j < 64 && ((shift + j + 1) << 3) <= obj->Size; j++) if (*(obj->Flag + i) & (1llu << j)) {
                if (*(uulong *)(obj->Data + ((shift + j) << 3)) != NULL) {
                    struct VM_ObjectInfo *mem = *(uulong *)(obj->Data + ((shift + j) << 3));
                    mem->QuoteCount--;
                    if (!mem->QuoteCount) GC(mem);
                }
            } 
    free(obj->Data);
    free(obj->Flag);
}
void VM_GC(struct VM_ObjectInfo *obj) {
    if (obj == NULL || obj->QuoteCount > 0) return ;
    GC(obj);
}

const int GCCheckLoopTime = 5 * CLOCKS_PER_SEC;

void ScanObject(struct VM_ObjectInfo* obj) {
    if (obj->State == 1) return ;
    obj->State = 1;
    for (uulong i = 0, shift = 0; i < obj->FlagSize; i++, shift += 64) 
        if (*(obj->Flag + i))
            for (uulong j = 0; j < 64 && ((shift + j + 1) << 3) <= obj->Size; j++) if (*(obj->Flag + i) & (1llu << j))
                if (*(uulong *)(obj->Data + ((shift + j) << 3)) != NULL) 
                    ScanObject((struct VM_ObjectInfo *)*(uulong *)(obj->Data + ((shift + j) << 3)));
}

void *VM_GCThread(void *arg) {
    // printf("GCThread Launched.\n");
    clock_t st = clock();
    int delay_count = 0;
    while (1) {
        while (delay_count <= GCCheckLoopTime) delay_count++;
        sleep(4);
        // if (clock() - st <= GCCheckLoopTime * max(1, (CurrentMemorySize >> 25))) continue;
        GCThreadState = ThreadState_Waiting;
        // wait until the VMThread pause
        while (VMThreadState == ThreadState_Running) ;
        GCThreadState = ThreadState_Running;
        uulong pos = 0;
        // printf("Scan...\n");
        for (struct VM_ObjectInfo *obj = ObjectListStart->Next; obj != ObjectListEnd; obj = obj->Next) 
            obj->State = 2;
        for (struct VM_ObjectInfo *obj = ObjectListStart->Next; obj != ObjectListEnd; obj = obj->Next)
            if (obj->VarQuoteCount > 0 && obj->State == 2) ScanObject(obj);
        for (struct VM_ObjectInfo *obj = ObjectListStart->Next; obj != ObjectListEnd; obj = obj->Next)
            if (obj->State == 2) {
                CurrentMemorySize -= obj->Size;
                DeleteElement(obj);
                obj->State = 0, free(obj->Data), free(obj->Flag);
                InsertElement(obj, FreeObjectListEnd);
            }
        GCThreadState = ThreadState_Pause;
    }
    GCThreadState = ThreadState_Exit;
    return NULL;
}
