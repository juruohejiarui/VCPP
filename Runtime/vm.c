#include "vm.h"
#include "vmobj.h"
#include "hashmap.h"
#include "syscall.h"

#define RUNTIME_BLOCK_SIZE 1024

char* CommandNameList[] = {COMMAND_NAME_LS};

struct RuntimeBlock {
    char *Path;
    FILE *FileInfo;

    uint8_t *Glomem, *ExecContent;
    uint64_t ExposeCount;
    struct HashMap* ExposeMap;
    uint64_t RelyCount;
    char **RelyList;
    uint64_t ExternCount;
    char **ExternList;

    uint64_t StringObjectCount;
    struct Object **StringObjectList;
    
    int IsLoaded;
} *runtime_block[RUNTIME_BLOCK_SIZE], *current_runtime_block;
int runtime_block_count;

void InitRuntimeBlock(struct RuntimeBlock *blk) {
    blk->Path = NULL;
    blk->Glomem = blk->ExecContent = NULL;
    blk->ExternList = blk->RelyList = NULL;
    blk->StringObjectList = NULL;
    blk->ExposeMap = HashMap_CreateMap(HASHMAP_DEFAULT_RANGE);
    blk->ExposeCount = blk->ExternCount = blk->RelyCount = blk->StringObjectCount = 0;
    blk->IsLoaded = 0;
}

// Extern Identifier Name -> Block ID
struct HashMap *ExternMap;
// Library Path -> Block ID
struct HashMap *LibraryMap;

struct CallFrame {
    uint64_t Address, VariableCount, UsedMaxVariable, *Variables;
    int BlockID;
} *call_stack, *call_stack_top;

uint64_t arg_data[16], tmp_data[16];
uint64_t *calculate_stack, *calculate_stack_top;

#pragma region File Operations
static inline FILE *OpenFile(char *path) { return fopen(path, "rb"); }
static inline uint64_t ReadUULong(FILE *file) {
    uint64_t res = 0;
    fread(&res, sizeof(uint64_t), 1, file);
    return res;
}
static inline uint8_t ReadVByte(FILE *file) { 
    uint8_t res = 0;
    fread(&res, sizeof(uint8_t), 1, file);
    return res; 
}
char temp_string[1 << 24];
//Read a string from the binary file, and alloc the memory for a copy
static inline char *ReadString(FILE *file) {
    int len = 0;
    char ch; fread(&ch, sizeof(char), 1, file);
    for (; ch != '\0'; fread(&ch, sizeof(char), 1, file)) {
        if (ch == '\\') {
            ch = fgetc(file);
            switch(ch) {
                case 't': temp_string[len++] = '\t'; break;
                case 'n': temp_string[len++] = '\n'; break;
                case 'r': temp_string[len++] = '\r'; break;
                case '0': temp_string[len++] = '\0'; break;
                case 'b': temp_string[len++] = '\b'; break;
                default: temp_string[len++] = ch; break;
            }
        } else temp_string[len++] = ch;
    }
    char *res = (char *)malloc(len + 1);
    memcpy(res, temp_string, len), res[len] = '\0';
    return res;
}
#pragma endregion

#pragma region Loading Process of VLib and Vexe
int PreloadVlib(char *path) {
    if (HashMap_Count(LibraryMap, path)) return 0;
    // create a runtime block
    int blkid = runtime_block_count++;
    struct RuntimeBlock *blk = (struct RuntimeBlock *) malloc(sizeof(struct RuntimeBlock));
    InitRuntimeBlock(blk);
    runtime_block[blkid] = blk;
    
    FILE *file_info = OpenFile(path);
    blk->FileInfo = file_info;
    
    //ignore the definition
    ReadString(file_info);


    uint64_t expose_count = blk->ExposeCount = ReadUULong(file_info);

    uint64_t *blkid_store = (uint64_t *)malloc(sizeof(uint64_t));
    *blkid_store = (uint64_t) blkid;

    for (int i = 0; i < expose_count; i++) {
        char *str = ReadString(file_info); uint64_t *addr = (uint64_t *)malloc(sizeof(uint64_t));
        *addr = ReadUULong(file_info);
        HashMap_Insert(blk->ExposeMap, str, addr);
        HashMap_Insert(ExternMap, str, blkid_store);
        free(str);
    }
    return 0;
}

void LoadStringRegion(FILE *file, struct RuntimeBlock *blk) {
    blk->StringObjectCount = ReadUULong(file);
    blk->StringObjectList = (struct Object **) malloc(sizeof(struct Object*) * blk->StringObjectCount);
    for (int i = 0; i < blk->StringObjectCount; i++) {
        char *str = ReadString(file);
        uint64_t len = strlen(str) + 1;
        struct Object *obj = VM_CreateObject(len + sizeof(uint64_t));
        VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);
        obj->FlagSize = 0;
        obj->Size = sizeof(uint64_t) + len;
        *(uint64_t*)(obj->Data) = 1;
        for (int j = 0; j < len; j++) obj->Data[j + sizeof(uint64_t)] = str[j];
        blk->StringObjectList[i] = obj;
    }
}

void LoadVlib(int blkid) {
    struct RuntimeBlock *blk = runtime_block[blkid];
    blk->IsLoaded = 1;
    uint64_t cnt = blk->RelyCount = ReadUULong(blk->FileInfo);
    for (int i = 0; i < cnt; i++) {
        char *path = ReadString(blk->FileInfo);
        PreloadVlib(path), free(path);
    }
    cnt = blk->ExternCount = ReadUULong(blk->FileInfo);
    blk->ExternList = (char **) malloc(sizeof(char*) * cnt);
    for (int i = 0; i < cnt; i++) blk->ExternList[i] = ReadString(blk->FileInfo);

    //load exec size
    blk->ExecContent = (uint8_t *) malloc(sizeof(uint8_t) * (cnt = ReadUULong(blk->FileInfo)));
    blk->Glomem = (uint8_t *) malloc(sizeof(uint8_t) * ReadUULong(blk->FileInfo));
    //load the string region
    LoadStringRegion(blk->FileInfo, blk);
    //load the exec content
    fread(blk->ExecContent, sizeof(uint8_t), cnt, blk->FileInfo);
    fclose(blk->FileInfo);
    return ;
}

//Load a vexe file, modify the current_runtime_block to it and return the main pointer
uint64_t LoadVexe(char *path) {
    // create a runtime block
    int blkid = runtime_block_count++;
    struct RuntimeBlock *blk = (struct RuntimeBlock *) malloc(sizeof(struct RuntimeBlock));
    InitRuntimeBlock(blk);
    runtime_block[blkid] = blk;
    //modify the current_runtime_block
    current_runtime_block = blk;

    uint64_t main_addr, execsize, cnt; uint8_t flag;
    int res = 0;
    FILE *file = OpenFile(path);
    blk->RelyCount = cnt = ReadUULong(file);
    blk->RelyList = (char **) malloc(sizeof(char *) * cnt);
    for (int i = 0; i < cnt; i++) 
        blk->RelyList[i] = ReadString(file),
        res |= PreloadVlib(blk->RelyList[i]);
    if (res == -1) { PrintError("Fail to preload vlib file\n", 0); return -1; }
    blk->ExternCount = cnt = ReadUULong(file);
    blk->ExternList = (char **) malloc(cnt * sizeof(char **));
    for (int i = 0; i < cnt; i++) blk->ExternList[i] = ReadString(file);

    execsize = ReadUULong(file), blk->ExecContent = (uint8_t *) malloc(sizeof(uint8_t) * execsize);
    cnt = ReadUULong(file), blk->Glomem = (uint8_t *) malloc(sizeof(uint8_t) * cnt);

    LoadStringRegion(file, blk);

    flag = ReadVByte(file), cnt = ReadUULong(file);
    if (!flag) main_addr = 0;
    else main_addr = cnt;

    fread(blk->ExecContent, sizeof(uint8_t), execsize, file);
    fclose(file);
    return main_addr;
}
#pragma endregion


#pragma region Function Operations
// blkid == -1 means that call a function in the current_block
void Function_Call(uint64_t addr, int arg_count, int blkid) {
    for (arg_count--; arg_count >= 0; arg_count--, calculate_stack_top--) 
        arg_data[arg_count] = *calculate_stack_top;
    if (blkid == -1) blkid = call_stack_top->BlockID;
    call_stack_top++;
    call_stack_top->Address = addr, call_stack_top->BlockID = blkid;
    current_runtime_block = runtime_block[blkid];
}

void Function_ECall(uint64_t id, int arg_count) {
    char *extiden = current_runtime_block->ExternList[id];
    uint64_t libid = *(uint64_t *) HashMap_Get(ExternMap, extiden), addr;
    if (!runtime_block[libid]->IsLoaded) LoadVlib(libid);
    addr = *(uint64_t *) HashMap_Get(runtime_block[libid]->ExposeMap, extiden);
    Function_Call(addr, arg_count, libid);
}

void Function_Return() {
    // printf("free %llx\n", call_stack_top->Variables);
    free(call_stack_top->Variables);
    --call_stack_top;
    current_runtime_block = runtime_block[call_stack_top->BlockID];
}
#pragma endregion

void *VM_VMThread(void *vexe_path) {
    // printf("VMThread Launched.\n");
    uint64_t main_addr = LoadVexe((char *)vexe_path);
    // printf("Loaded vexe file %s\n", vexe_path);
    Function_Call(main_addr, 0, 0);
    uint8_t cid; uint64_t arg1, arg2;
    while (call_stack_top != call_stack) {
        // use generational GC if it is necessary
        if (VM_Check()) VM_GenerationalGC();
        cid = current_runtime_block->ExecContent[call_stack_top->Address++];
        switch (cid) {
            #pragma region xxmov
            case vbmov:
                *(uint8_t *)(*(calculate_stack_top - 1)) = *((uint8_t *)calculate_stack_top);
                calculate_stack_top -= 2;
                break;
            case vi32mov:
                *(unsigned int *)(*(calculate_stack_top - 1)) = *((unsigned int *)calculate_stack_top);
                calculate_stack_top -= 2;
                break;
            case vi64mov:
                *((uint64_t *)(*(calculate_stack_top - 1))) = *((uint64_t *)calculate_stack_top);
                calculate_stack_top -= 2;
                break;
            case vfmov:
                *(double *)(*(calculate_stack_top - 1)) = *((double *)calculate_stack_top);
                calculate_stack_top -= 2;
                break;
            case vomov:
                {
                    struct Object *lst_obj = (struct Object*)*(uint64_t*)*(calculate_stack_top - 1);
                    if (lst_obj != NULL)
                        VM_ReduceRootReference(lst_obj, 2), VM_ReduceReference(lst_obj, 2);
                    *(uint64_t *)*(calculate_stack_top - 1) = *calculate_stack_top;
                    calculate_stack_top -= 2;
                }
                break;
            case mbmov:
                {
                    struct Object *obj = (struct Object *)*(calculate_stack_top - 2);
                    VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                    *(uint8_t *)*(calculate_stack_top - 1) = *(uint8_t *)calculate_stack_top;
                    calculate_stack_top -= 3;
                }
                break;
            case mi32mov:
                {
                    struct Object *obj = (struct Object *)*(calculate_stack_top - 2);
                    VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                    *(unsigned int *)*(calculate_stack_top - 1) = *(unsigned int *)calculate_stack_top;
                    calculate_stack_top -= 3;
                }
                break;
            case mi64mov:
                {
                    struct Object *obj = (struct Object *)*(calculate_stack_top - 2);
                    VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                    *(uint64_t *)*(calculate_stack_top - 1) = *(uint64_t *)calculate_stack_top;
                    calculate_stack_top -= 3;
                }
                break;
             case mfmov:
                {
                    struct Object *obj = (struct Object *)*(calculate_stack_top - 2);
                    VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                    *(double *)*(calculate_stack_top - 1) = *(double *)calculate_stack_top;
                    calculate_stack_top -= 3;
                }
                break;
            case momov:
                {
                    struct Object *par = (struct Object *)*(calculate_stack_top - 2),
                                        *lst_obj = (struct Object *)*(uint64_t *)*(calculate_stack_top - 1);
                    VM_ReduceRootReference(par, 1), VM_ReduceReference(par, 1);
                    if (lst_obj != NULL) {
                        // remove the reference from stack and par
                        VM_ReduceRootReference(lst_obj, 1), VM_ReduceReference(lst_obj, 2);
                        // modify the cross generation reference if it is necessary
                        if (par->Generation > lst_obj->Generation) VM_ReduceCrossReference(lst_obj, 1);
                    }
                    *(uint64_t *)*(calculate_stack_top - 1) = *calculate_stack_top;
                    if ((struct Object*)*calculate_stack_top != NULL) {
                        struct Object *new_obj = (struct Object*)*calculate_stack_top;
                        // If it is a cross generation reference, -> cross reference count += 1
                        // remove it from stack -> root reference count -= 1, reference count -= 1
                        // add reference from par -> reference count += 1
                        VM_ReduceRootReference(new_obj, 1);
                        if (par->Generation > new_obj->Generation) VM_AddCrossReference(new_obj, 1);
                    }
                    calculate_stack_top -= 3;
                }
                break;
            #pragma endregion
            #pragma region basic operator (+ - * / %)
            case add:
                *((int *)(calculate_stack_top - 1)) += *((int *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case sub:
                *((int *)(calculate_stack_top - 1)) -= *((int *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case mul:
                *((int *)(calculate_stack_top - 1)) *= *((int *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case _div:
                *((int *)(calculate_stack_top - 1)) /= *((int *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case mod:
                *((int *)(calculate_stack_top - 1)) %= *((int *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case ladd:
                *((long long *)(calculate_stack_top - 1)) += *((long long *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case lsub:
                *((long long *)(calculate_stack_top - 1)) -= *((long long *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case lmul:
                *((long long *)(calculate_stack_top - 1)) *= *((long long *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case _ldiv:
                *((long long *)(calculate_stack_top - 1)) /= *((long long *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case lmod:
                *((long long *)(calculate_stack_top - 1)) %= *((long long *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case fadd:
                *((double *)(calculate_stack_top - 1)) += *((double *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case fsub:
                *((double *)(calculate_stack_top - 1)) -= *((double *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case fmul:
                *((double *)(calculate_stack_top - 1)) *= *((double *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case fdiv:
                *((double *)(calculate_stack_top - 1)) /= *((double *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case uadd:
                *((uint64_t *)(calculate_stack_top - 1)) += *((uint64_t *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case usub:
                *((uint64_t *)(calculate_stack_top - 1)) -= *((uint64_t *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case umul:
                *((uint64_t *)(calculate_stack_top - 1)) *= *((uint64_t *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case udiv:
                *((uint64_t *)(calculate_stack_top - 1)) /= *((uint64_t *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case umod:
                *((uint64_t *)(calculate_stack_top - 1)) %= *((uint64_t *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case badd:
                *((uint64_t *)(calculate_stack_top - 1)) += *((uint64_t *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case bsub:
                *((char *)(calculate_stack_top - 1)) -= *((char *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case bmul:
                *((char *)(calculate_stack_top - 1)) *= *((char *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case bdiv:
                *((char *)(calculate_stack_top - 1)) /= *((char *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case bmod:
                *((char *)(calculate_stack_top - 1)) %= *((char *)calculate_stack_top);
                calculate_stack_top--;
                break;
            #pragma endregion
            #pragma region compare operator
            case ne:
                *((uint64_t *)(calculate_stack_top - 1)) = (*((uint64_t *)(calculate_stack_top - 1)) != *((uint64_t *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case eq:
                *((uint64_t *)(calculate_stack_top - 1)) = (*((uint64_t *)(calculate_stack_top - 1)) == *((uint64_t *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case gt:
                *((uint64_t *)(calculate_stack_top - 1)) = (*((uint64_t *)(calculate_stack_top - 1)) > *((uint64_t *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case ge:
                *((uint64_t *)(calculate_stack_top - 1)) = (*((uint64_t *)(calculate_stack_top - 1)) >= *((uint64_t *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case ls:
                *((uint64_t *)(calculate_stack_top - 1)) = (*((uint64_t *)(calculate_stack_top - 1)) < *((uint64_t *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case le:
                *((uint64_t *)(calculate_stack_top - 1)) = (*((uint64_t *)(calculate_stack_top - 1)) <= *((uint64_t *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case fne:
                *((double *)(calculate_stack_top - 1)) = (*((double *)(calculate_stack_top - 1)) != *((double *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case feq:
                *((double *)(calculate_stack_top - 1)) = (*((double *)(calculate_stack_top - 1)) == *((double *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case fgt:
                *((double *)(calculate_stack_top - 1)) = (*((double *)(calculate_stack_top - 1)) > *((double *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case fge:
                *((double *)(calculate_stack_top - 1)) = (*((double *)(calculate_stack_top - 1)) >= *((double *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case _fls:
                *((double *)(calculate_stack_top - 1)) = (*((double *)(calculate_stack_top - 1)) < *((double *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case fle:
                *((double *)(calculate_stack_top - 1)) = (*((double *)(calculate_stack_top - 1)) <= *((double *)calculate_stack_top));
                calculate_stack_top--;
                break;

            #pragma endregion
            #pragma region bit operator 
            case _and:
                *((int *)(calculate_stack_top - 1)) &= *((int *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case _or:
                *((int *)(calculate_stack_top - 1)) |= *((int *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case _xor:
                *((int *)(calculate_stack_top - 1)) ^= *((int *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case _not:
                *((int *)calculate_stack_top) = ~(*(int *)calculate_stack_top);
                break;
            case lmv:
                *((int *)(calculate_stack_top - 1)) <<= *((int *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case rmv:
                *((int *)(calculate_stack_top - 1)) >>= *((int *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case land:
                *((long long *)(calculate_stack_top - 1)) &= *((long long *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case lor:
                *((long long *)(calculate_stack_top - 1)) |= *((long long *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case lxor:
                *((long long *)(calculate_stack_top - 1)) ^= *((long long *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case lnot:
                *((long long *)calculate_stack_top) = ~(*(long long *)calculate_stack_top);
                break;
            case llmv:
                *((long long *)(calculate_stack_top - 1)) <<= *((long long *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case lrmv:
                *((long long *)(calculate_stack_top - 1)) >>= *((long long *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case uand:
                *((uint64_t *)(calculate_stack_top - 1)) &= *((uint64_t *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case uor:
                *((uint64_t *)(calculate_stack_top - 1)) |= *((uint64_t *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case uxor:
                *((uint64_t *)(calculate_stack_top - 1)) ^= *((uint64_t *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case unot:
                *((uint64_t *)calculate_stack_top) = ~(*(uint64_t *)calculate_stack_top);
                break;
            case ulmv:
                *((uint64_t *)(calculate_stack_top - 1)) <<= *((uint64_t *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case urmv:
                *((uint64_t *)(calculate_stack_top - 1)) >>= *((uint64_t *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case band:
                *((char *)(calculate_stack_top - 1)) &= *((char *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case bor:
                *((char *)(calculate_stack_top - 1)) |= *((char *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case bxor:
                *((char *)(calculate_stack_top - 1)) ^= *((char *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case bnot:
                *((char *)calculate_stack_top) = ~(*(char *)calculate_stack_top);
                break;
            case blmv:
                *((char *)(calculate_stack_top - 1)) <<= *((char *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case brmv:
                *((char *)(calculate_stack_top - 1)) >>= *((char *)calculate_stack_top);
                calculate_stack_top--;
                break;
            #pragma endregion
            #pragma region xxgvl
            case vbgvl:
                *calculate_stack_top = *(uint8_t *)*calculate_stack_top;
                break;
            case vi32gvl:
                *calculate_stack_top = *(unsigned int *)*calculate_stack_top;
                break;
            case vi64gvl:
                *calculate_stack_top = *(uint64_t *)*calculate_stack_top;
                break;
            case vfgvl:
                *((double *)(calculate_stack_top)) = *(double *)*calculate_stack_top;
				break;
            case vogvl:
                *calculate_stack_top = (uint64_t)(struct Object*)*(uint64_t*)*calculate_stack_top;
                break;
            case mbgvl:
                {
                    struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                    VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                    *(calculate_stack_top - 1) = *(uint8_t *)*calculate_stack_top;
                    calculate_stack_top--;
                }
                break;
            case mi32gvl:
                {
                    struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                    VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                    *(calculate_stack_top - 1) = *(unsigned *)*calculate_stack_top;
                    calculate_stack_top--;
                }
                break;
            case mi64gvl:
                {
                    struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                    VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                    *(uint64_t *)(calculate_stack_top - 1) = *(uint64_t *)*calculate_stack_top;
                    calculate_stack_top--;
                }
                break;
            case mfgvl:
                {
                    struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                    VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                    *(double *)(calculate_stack_top - 1) = *(double *)*calculate_stack_top;
                    calculate_stack_top--;
                }
                break;
            case mogvl:
                {
                    struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                    VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                    *(uint64_t *)(calculate_stack_top - 1) = *(uint64_t *)*calculate_stack_top;
                    calculate_stack_top--;
                }
                break;
            #pragma endregion
            #pragma region object operator 
            case _new:
                {
                    struct Object *obj = VM_CreateObject(*calculate_stack_top);
                    VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);
                    *calculate_stack_top = (uint64_t)obj;
                }
                break;
            case mem:
                {
                    arg1 = *(uint64_t *)&current_runtime_block->ExecContent[call_stack_top->Address];
                    call_stack_top->Address += sizeof(uint64_t);
                    struct Object *obj = (struct Object*)*(calculate_stack_top++);
                    *calculate_stack_top = (uint64_t)(obj->Data + arg1);
                }
                break;
            case omem:
                {
                    //find the address
                    arg1 = *(uint64_t *)&current_runtime_block->ExecContent[call_stack_top->Address];
                    call_stack_top->Address += sizeof(uint64_t);
                    //get the object
                    struct Object *obj = (struct Object*)*(calculate_stack_top++), *_mem;
                    *calculate_stack_top = (uint64_t)(obj->Data + arg1);
                    //set the flag
                    *(obj->Flag + VM_FlagAddr(arg1)) |= VM_FlagBit(arg1);
                    _mem = (struct Object*)*(uint64_t*)*calculate_stack_top;
                    if (_mem != NULL)
                        VM_AddReference(_mem, 1), VM_AddRootReference(_mem, 1);
                }
                break;
            case arrnew:
                {
                    arg1 = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                    call_stack_top->Address += sizeof(uint64_t);
                    for (int i = arg1; i >= 0; i--) tmp_data[i] = *(calculate_stack_top--);
                    tmp_data[0] = min(sizeof(uint64_t), tmp_data[0]);
                    for (int i = 1; i <= arg1; i++) tmp_data[i] *= tmp_data[i - 1];
                    struct Object *obj = VM_CreateObject((arg1 << 3) + tmp_data[arg1]);
                    VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);
                    for (int i = 0; i < arg1; i++)
                        *(uint64_t *)&obj->Data[i << 3] = tmp_data[i];
                    *(++calculate_stack_top) = (uint64_t)obj;
                }
                break;
            case arrmem: 
                {
                    // Get the dimension
                    arg1 = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                    call_stack_top->Address += sizeof(uint64_t);
                    // Get the index of each dimension
                    for (int i = arg1; i >= 0; i--) tmp_data[i] = *(calculate_stack_top--);
                    calculate_stack_top++;
                    // Calculate the address
                    arg2 = arg1 * sizeof(uint64_t);
                    uint8_t *obj_data = ((struct Object*)tmp_data[0])->Data;
                    for (uint64_t i = 1, p = 0; i <= arg1; i++, p += sizeof(uint64_t)) 
                        arg2 += tmp_data[i] * (*(uint64_t*)(obj_data + p));
                    // Update the calculate_stack
                    *(++calculate_stack_top) = (uint64_t)(obj_data + arg2);
                }
                break;
            case arrmem1:
                {
                    arg2 = *(calculate_stack_top--), arg1 = *calculate_stack_top;
                    uint8_t *obj_data = ((struct Object*)arg1)->Data;
                    *(++calculate_stack_top) = (uint64_t)(obj_data + sizeof(uint64_t) + arg2 * (*(uint64_t *)obj_data));
                }
                break;
            case arromem:
                {
                    // Get the dimension
                    arg1 = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                    call_stack_top->Address += sizeof(uint64_t);
                    // Get the index of each dimension
                    for (int i = arg1; i >= 0; i--) tmp_data[i] = *(calculate_stack_top--);
                    calculate_stack_top++;
                    // Calculate the address
                    arg2 = arg1 * sizeof(uint64_t);
                    uint8_t *obj_data = ((struct Object*)tmp_data[0])->Data;
                    for (uint64_t i = 1, p = 0; i <= arg1; i++, p += sizeof(uint64_t)) 
                        arg2 += tmp_data[i] * (*(uint64_t*)(obj_data + p));
                    // Update the calculate_stack
                    *(++calculate_stack_top) = (uint64_t)(obj_data + arg2);
                    // Set Flag
                    *(((struct Object*)tmp_data[0])->Flag + VM_FlagAddr(arg2)) |= VM_FlagBit(arg2);
                    
                    struct Object *mem_obj = (struct Object*)*(uint64_t*)*calculate_stack_top;
                    if (mem_obj != NULL) VM_AddRootReference(mem_obj, 1), VM_AddReference(mem_obj, 1);
                }
                break;
            case arromem1:
                {
                    // arg2 illustrates the index, arg1 is the array object
                    arg2 = *(calculate_stack_top--), arg1 = *calculate_stack_top;
                    uint8_t *obj_data = ((struct Object*)arg1)->Data;
                    arg2 *= *(uint64_t *)obj_data; arg2 += sizeof(uint64_t);
                    *(++calculate_stack_top) = (uint64_t)(obj_data + arg2);
                    // Set Flag
                    *(((struct Object*)arg1)->Flag + VM_FlagAddr(arg2)) |= VM_FlagBit(arg2);

                    struct Object *mem_obj = (struct Object*)*(uint64_t*)*calculate_stack_top;
                    if (mem_obj != NULL) VM_AddRootReference(mem_obj, 1), VM_AddReference(mem_obj, 1);
                }
                break;
            case pack:
                {
                    struct Object* obj = VM_CreateObject(9);
                    VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);
                    *(uint64_t *)obj->Data = *calculate_stack_top;
                    *calculate_stack_top = (uint64_t)obj;
                }
                break;
            case unpack:
                {
                    struct Object *obj = (struct Object *)*calculate_stack_top;
                    VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                    *(calculate_stack_top) = *(uint64_t*)obj->Data;
                }
                break;
            #pragma endregion
            #pragma region jump and function operator
            case jmp:
                call_stack_top->Address = *(uint64_t*)(current_runtime_block->ExecContent + call_stack_top->Address);
                break;
            case jz:
                if (*(calculate_stack_top--)) call_stack_top->Address += sizeof(uint64_t);
                else call_stack_top->Address = *(uint64_t*)(current_runtime_block->ExecContent + call_stack_top->Address);
                break;
            case jp:
                if (*(calculate_stack_top--)) 
                    call_stack_top->Address = *(uint64_t*)(current_runtime_block->ExecContent + call_stack_top->Address);
                else call_stack_top->Address += sizeof(uint64_t);
                break;
            case call:
                arg1 = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(uint64_t);
                arg2 = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(uint64_t);
                Function_Call(arg1, arg2, -1);
                break;
            case ecall:
                arg1 = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(uint64_t);
                arg2 = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(uint64_t);
                Function_ECall(arg1, arg2);
                break;
            case ret:
                Function_Return();
                break;
            #pragma endregion
            #pragma region constant and variable operator
            case setvar:
                arg1 = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(uint64_t);
                call_stack_top->Variables = malloc((arg1 + 1) * sizeof(uint64_t));
                for (int i = 0; i < arg1; i++) call_stack_top->Variables[i] = 0;
                break;
            case poparg:
                arg1 = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(uint64_t);
                for (int i = 0; i < arg1; i++) call_stack_top->Variables[i] = arg_data[i];
                break;
            case push:
                *(++calculate_stack_top) = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(uint64_t);
                break;
            case push0:
                *(++calculate_stack_top) = 0;
                break;
            case push1:
                *(++calculate_stack_top) = 1;
                break;
            case pvar:
                *(++calculate_stack_top) = (uint64_t)(
                    call_stack_top->Variables + *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address));
                call_stack_top->Address += sizeof(uint64_t);
                break;
            case pvar0:
                *(++calculate_stack_top) = (uint64_t)(&call_stack_top->Variables[0]);
                break;
            case pvar1:
                *(++calculate_stack_top) = (uint64_t)(&call_stack_top->Variables[1]);
                break;
             case pvar2:
                *(++calculate_stack_top) = (uint64_t)(&call_stack_top->Variables[2]);
                break;
             case pvar3:
                *(++calculate_stack_top) = (uint64_t)(&call_stack_top->Variables[3]);
                break;
            case povar:
                *(++calculate_stack_top) = (uint64_t)(
                    call_stack_top->Variables + *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address));
                call_stack_top->Address += sizeof(uint64_t);
                {
                    struct Object* obj = (struct Object *)*(uint64_t*)*calculate_stack_top;
                    if (obj != NULL) VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);
                }
                break;
            case povar0:
                *(++calculate_stack_top) = (uint64_t)(&call_stack_top->Variables[0]);
                {
                    struct Object* obj = (struct Object *)*(uint64_t*)*calculate_stack_top;
                    if (obj != NULL) VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);

                }
                break;
            case povar1:
                *(++calculate_stack_top) = (uint64_t)(&call_stack_top->Variables[1]);
                {
                    struct Object* obj = (struct Object *)*(uint64_t*)*calculate_stack_top;
                    if (obj != NULL) VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);
                }
                break;
            case povar2:
                *(++calculate_stack_top) = (uint64_t)(&call_stack_top->Variables[2]);
                {
                    struct Object* obj = (struct Object *)*(uint64_t*)*calculate_stack_top;
                    if (obj != NULL) VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);
                }
                break;
            case povar3:
                *(++calculate_stack_top) = (uint64_t)(&call_stack_top->Variables[3]);
                {
                    struct Object* obj = (struct Object *)*(uint64_t*)*(calculate_stack_top);
                    if (obj != NULL) VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);
                }
                break;
            case pglo:
                *(++calculate_stack_top) = (uint64_t)(
                    current_runtime_block->Glomem + *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address));
                call_stack_top->Address += sizeof(uint64_t);
                break;
            case poglo:
                *(++calculate_stack_top) = (uint64_t)(
                    current_runtime_block->Glomem + *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address));
                call_stack_top->Address += sizeof(uint64_t);
                {
                    struct Object* obj = (struct Object *)*(uint64_t*)*(calculate_stack_top);
                    if (obj != NULL) VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);
                }
                break;
            case pstr:
                *(++calculate_stack_top) = (uint64_t)(&current_runtime_block->StringObjectList[
                    *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address)
                ]);
                call_stack_top->Address += sizeof(uint64_t);
                {
                    struct Object* obj = (struct Object *)*(uint64_t*)*(calculate_stack_top);
                    if (obj != NULL) VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);
                }
                break;
            case cpy:
                *(calculate_stack_top + 1) = *(calculate_stack_top);
                calculate_stack_top++;
                break;
            case ocpy:
                *(calculate_stack_top + 1) = *(calculate_stack_top);
                calculate_stack_top++;
                if ((struct Object *)*calculate_stack_top != NULL) {
                    struct Object *obj = (struct Object *)*calculate_stack_top;
                    VM_AddRootReference(obj, 1), VM_AddReference(obj, 1);
                }
                break;
            case pop:
                --calculate_stack_top;
                break;
            case opop:
                {
                    struct Object *obj = (struct Object *)calculate_stack_top;
                    if (obj != NULL) VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                    --calculate_stack_top;
                }
                break;
            #pragma endregion
            #pragma region system operator
            case sys:
                arg1 = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(uint64_t);

                if (arg1 < 6)
                    System_IO(arg1, *(calculate_stack_top--));
                else if (arg1 < 12)
                    *(++calculate_stack_top) = System_IO(arg1, 0);
                break;
            #pragma endregion
            case excmd:
                {
                    uint64_t excid = *(uint64_t *)(current_runtime_block->ExecContent + call_stack_top->Address);
                    call_stack_top->Address += sizeof(uint64_t);
                    switch (excid) {
                        #pragma region ++ and --
                        case EX_vbpinc:
                            *(uint8_t *)calculate_stack_top = ++*(uint8_t *)*calculate_stack_top;
                            *calculate_stack_top = (*calculate_stack_top & ~(1ull << 8)) ^ (~(1ull << 8));
                            break;
                        case EX_vi32pinc:
                            *(int *)calculate_stack_top = ++*(int *)*calculate_stack_top;
                            *((int *)calculate_stack_top + 1) = 0;
                            break;
                        case EX_vi64pinc:
                            *(long long *)calculate_stack_top = ++*(long long *)*calculate_stack_top;
                            break;
                        case EX_vupinc:
                            *calculate_stack_top = ++*(uint64_t *)*calculate_stack_top;
                            break;
                        case EX_vbsinc:
                            *calculate_stack_top = (*(uint8_t *)*calculate_stack_top)++;
                            *calculate_stack_top = (*calculate_stack_top & ~(1ull << 8)) ^ (~(1ull << 8));
                            break;
                        case EX_vi32sinc:
                            *calculate_stack_top = (*(int *)*calculate_stack_top)++;
                            *((int *)calculate_stack_top + 1) = 0;
                            break;
                        case EX_vi64sinc:
                            *(long long *)calculate_stack_top = (*(long long *)*calculate_stack_top)++;
                            break;
                        case EX_vusinc:
                            *calculate_stack_top = (*(uint64_t *)*calculate_stack_top)++;
                            break;

                        case EX_vbpdec:
                            *(uint8_t *)calculate_stack_top = --*(uint8_t *)*calculate_stack_top;
                            *calculate_stack_top = (*calculate_stack_top & ~(1ull << 8)) ^ (~(1ull << 8));
                            break;
                        case EX_vi32pdec:
                            *(int *)calculate_stack_top = --*(int *)*calculate_stack_top;
                            break;
                        case EX_vi64pdec:
                            *(long long *)calculate_stack_top = --*(long long *)*calculate_stack_top;
                            break;
                        case EX_vupdec:
                            *calculate_stack_top = --*(uint64_t *)*calculate_stack_top;
                            break;
                        case EX_vbsdec:
                            *calculate_stack_top = *(uint8_t *)*calculate_stack_top--;
                            break;
                        case EX_vi32sdec:
                            *calculate_stack_top = (*(int *)*calculate_stack_top)--;
                            *((int *)calculate_stack_top + 1) = 0;
                            break;
                        case EX_vi64sdec:
                            *(long long *)calculate_stack_top = (*(long long *)*calculate_stack_top)--;
                            break;
                        case EX_vusdec:
                            *calculate_stack_top = *(uint64_t *)*calculate_stack_top--;
                            break;

                        case EX_mbpinc:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(uint8_t *)(calculate_stack_top - 1) = ++*(uint8_t *)*calculate_stack_top;
                                calculate_stack_top--;
                                *calculate_stack_top = (*calculate_stack_top & ~(1ull << 8)) ^ (~(1ull << 8));
                            }
                            break;
                        case EX_mi32pinc:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(int *)(calculate_stack_top - 1) = ++*(int *)*calculate_stack_top;
                                calculate_stack_top--;
                                *((int *)calculate_stack_top + 1) = 0;
                            }
                            break;
                        case EX_mi64pinc:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(long long *)(calculate_stack_top - 1) = ++*(long long *)*calculate_stack_top;
                                calculate_stack_top--;
                            }
                            break;
                        case EX_mupinc:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(uint64_t *)(calculate_stack_top - 1) = ++*(uint64_t *)*calculate_stack_top;
                                calculate_stack_top--;
                            }
                            break;
                        case EX_mbsinc:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(uint8_t *)(calculate_stack_top - 1) = (*(uint8_t *)*calculate_stack_top)++;
                                calculate_stack_top--;
                                *calculate_stack_top = (*calculate_stack_top & ~(1ull << 8)) ^ (~(1ull << 8));
                            }
                            break;
                        case EX_mi32sinc:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(int *)(calculate_stack_top - 1) = (*(int *)*calculate_stack_top)++;
                                calculate_stack_top--;
                                *((int *)calculate_stack_top + 1) = 0;
                            }
                            break;
                        case EX_mi64sinc:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(long long *)(calculate_stack_top - 1) = (*(long long *)*calculate_stack_top)++;
                                calculate_stack_top--;
                            }
                            break;
                        case EX_musinc:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(uint64_t *)(calculate_stack_top - 1) = (*(uint64_t *)*calculate_stack_top)++;
                                calculate_stack_top--;
                            }
                            break;
                        case EX_mbpdec:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(uint8_t *)(calculate_stack_top - 1) = --*(uint8_t *)*calculate_stack_top;
                                calculate_stack_top--;
                                *calculate_stack_top = (*calculate_stack_top & ~(1ull << 8)) ^ (~(1ull << 8));
                            }
                            break;
                        case EX_mi32pdec:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(int *)(calculate_stack_top - 1) = --*(int *)*calculate_stack_top;
                                calculate_stack_top--;
                                *((int *)calculate_stack_top + 1) = 0;
                            }
                            break;
                        case EX_mi64pdec:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(long long *)(calculate_stack_top - 1) = --*(long long *)*calculate_stack_top;
                                calculate_stack_top--;
                            }
                            break;
                        case EX_mupdec:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(uint64_t *)(calculate_stack_top - 1) = --*(uint64_t *)*calculate_stack_top;
                                calculate_stack_top--;
                            }
                            break;
                        case EX_mbsdec:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(uint8_t *)(calculate_stack_top - 1) = (*(uint8_t *)*calculate_stack_top)--;
                                calculate_stack_top--;
                                *calculate_stack_top = (*calculate_stack_top & ~(1ull << 8)) ^ (~(1ull << 8));
                            }
                            break;
                        case EX_mi32sdec:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(int *)(calculate_stack_top - 1) = (*(int *)*calculate_stack_top)--;
                                calculate_stack_top--;
                                *((int *)calculate_stack_top + 1) = 0;
                            }
                            break;
                        case EX_mi64sdec:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(long long *)(calculate_stack_top - 1) = (*(long long *)*calculate_stack_top)--;
                                calculate_stack_top--;
                            }
                            break;
                        case EX_musdec:
                            {
                                struct Object *obj = (struct Object *)*(calculate_stack_top - 1);
                                VM_ReduceRootReference(obj, 1), VM_ReduceReference(obj, 1);
                                *(uint64_t *)(calculate_stack_top - 1) = (*(uint64_t *)*calculate_stack_top)--;
                                calculate_stack_top--;
                            }
                            break;
                        #pragma endregion
                    }
                }
                break;
        }
    }
    return NULL;
}

void InitVM(uint64_t calculate_stack_size, uint64_t call_stack_size) {
    LibraryMap = HashMap_CreateMap(HASHMAP_DEFAULT_RANGE);
    ExternMap = HashMap_CreateMap(HASHMAP_DEFAULT_RANGE);
    calculate_stack = (uint64_t *) malloc(sizeof(uint64_t) * calculate_stack_size);
    call_stack = (uint64_t *) malloc(sizeof(struct CallFrame) * call_stack_size);
    calculate_stack_top = calculate_stack;
    call_stack_top = call_stack;
    runtime_block_count = 0;
}
void VM_Launch(char *path, uint64_t calculate_stack_size, uint64_t call_stack_size) {
    InitVM(calculate_stack_size, call_stack_size);
    VM_InitGC(DEFAULT_GENERATION0_MAX_SIZE, DEFAULT_GENERATION1_MAX_SIZE, DEFAULT_GC_TIME_INTERVAL);
    
    clock_t st = clock();
    VM_VMThread((void *)path);
    printf("\n exited, cost %.3lf\n", 1.0 * (clock() - st) / CLOCKS_PER_SEC);
    pthread_exit(NULL);
}