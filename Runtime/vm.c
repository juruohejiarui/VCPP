#include "vm.h"
#include "vmobj.h"
#include "hashmap.h"
#include "syscall.h"

#define RUNTIME_BLOCK_SIZE 1024

char* CommandNameList[] = {COMMAND_NAME_LS};

struct RuntimeBlock {
    char *Path;
    FILE *FileInfo;

    vbyte *Glomem, *ExecContent;
    ullong ExposeCount;
    struct HashMap* ExposeMap;
    ullong RelyCount;
    char **RelyList;
    ullong ExternCount;
    char **ExternList;

    ullong StringObjectCount;
    struct VM_ObjectInfo **StringObjectList;
    
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
    ullong Address, VariableCount, UsedMaxVariable, *Variables;
    int BlockID;
} *call_stack, *call_stack_top;

ullong arg_data[16], tmp_data[16];
ullong *calculate_stack, *calculate_stack_top;

#pragma region File Operations
static inline FILE *OpenFile(char *path) { return fopen(path, "rb"); }
static inline ullong ReadUULong(FILE *file) {
    ullong res = 0;
    fread(&res, sizeof(ullong), 1, file);
    return res;
}
static inline vbyte ReadVByte(FILE *file) { 
    vbyte res = 0;
    fread(&res, sizeof(vbyte), 1, file);
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


    ullong expose_count = blk->ExposeCount = ReadUULong(file_info);

    ullong *blkid_store = (ullong *)malloc(sizeof(ullong));
    *blkid_store = (ullong) blkid;

    for (int i = 0; i < expose_count; i++) {
        char *str = ReadString(file_info); ullong *addr = (ullong *)malloc(sizeof(ullong));
        *addr = ReadUULong(file_info);
        HashMap_Insert(blk->ExposeMap, str, addr);
        HashMap_Insert(ExternMap, str, blkid_store);
        free(str);
    }
    return 0;
}

void LoadStringRegion(FILE *file, struct RuntimeBlock *blk) {
    blk->StringObjectCount = ReadUULong(file);
    blk->StringObjectList = (struct VM_ObjectInfo **) malloc(sizeof(struct VM_ObjectInfo*) * blk->StringObjectCount);
    for (int i = 0; i < blk->StringObjectCount; i++) {
        char *str = ReadString(file);
        ullong len = strlen(str) + 1;
        struct VM_ObjectInfo *obj = VM_CreateObjectInfo(len + sizeof(ullong));
        obj->QuoteCount = obj->VarQuoteCount = 1;
        obj->FlagSize = 0;
        obj->Size = sizeof(ullong) + len;
        *(ullong*)(obj->Data) = 1;
        for (int j = 0; j < len; j++) obj->Data[j + sizeof(ullong)] = str[j];
        blk->StringObjectList[i] = obj;
    }
}

void LoadVlib(int blkid) {
    struct RuntimeBlock *blk = runtime_block[blkid];
    blk->IsLoaded = 1;
    ullong cnt = blk->RelyCount = ReadUULong(blk->FileInfo);
    for (int i = 0; i < cnt; i++) {
        char *path = ReadString(blk->FileInfo);
        PreloadVlib(path), free(path);
    }
    cnt = blk->ExternCount = ReadUULong(blk->FileInfo);
    blk->ExternList = (char **) malloc(sizeof(char*) * cnt);
    for (int i = 0; i < cnt; i++) blk->ExternList[i] = ReadString(blk->FileInfo);

    //load exec size
    blk->ExecContent = (vbyte *) malloc(sizeof(vbyte) * (cnt = ReadUULong(blk->FileInfo)));
    blk->Glomem = (vbyte *) malloc(sizeof(vbyte) * ReadUULong(blk->FileInfo));
    //load the string region
    LoadStringRegion(blk->FileInfo, blk);
    //load the exec content
    fread(blk->ExecContent, sizeof(vbyte), cnt, blk->FileInfo);
    fclose(blk->FileInfo);
    return ;
}

//Load a vexe file, modify the current_runtime_block to it and return the main pointer
ullong LoadVexe(char *path) {
    // create a runtime block
    int blkid = runtime_block_count++;
    struct RuntimeBlock *blk = (struct RuntimeBlock *) malloc(sizeof(struct RuntimeBlock));
    InitRuntimeBlock(blk);
    runtime_block[blkid] = blk;
    //modify the current_runtime_block
    current_runtime_block = blk;

    ullong main_addr, execsize, cnt; vbyte flag;
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

    execsize = ReadUULong(file), blk->ExecContent = (vbyte *) malloc(sizeof(vbyte) * execsize);
    cnt = ReadUULong(file), blk->Glomem = (vbyte *) malloc(sizeof(vbyte) * cnt);

    LoadStringRegion(file, blk);

    flag = ReadVByte(file), cnt = ReadUULong(file);
    if (!flag) main_addr = 0;
    else main_addr = cnt;

    fread(blk->ExecContent, sizeof(vbyte), execsize, file);
    fclose(file);
    return main_addr;
}
#pragma endregion


#pragma region Function Operations
// blkid == -1 means that call a function in the current_block
void Function_Call(ullong addr, int arg_count, int blkid) {
    for (arg_count--; arg_count >= 0; arg_count--, calculate_stack_top--) 
        arg_data[arg_count] = *calculate_stack_top;
    if (blkid == -1) blkid = call_stack_top->BlockID;
    call_stack_top++;
    call_stack_top->Address = addr, call_stack_top->BlockID = blkid;
    current_runtime_block = runtime_block[blkid];
}

void Function_ECall(ullong id, int arg_count) {
    char *extiden = current_runtime_block->ExternList[id];
    ullong libid = *(ullong *) HashMap_Get(ExternMap, extiden), addr;
    if (!runtime_block[libid]->IsLoaded) LoadVlib(libid);
    addr = *(ullong *) HashMap_Get(runtime_block[libid]->ExposeMap, extiden);
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
    ullong main_addr = LoadVexe((char *)vexe_path);
    // printf("Loaded vexe file %s\n", vexe_path);
    Function_Call(main_addr, 0, 0);
    vbyte cid; ullong arg1, arg2;
    while (call_stack_top != call_stack) {
        if (GCThreadState == ThreadState_Waiting) {
            VMThreadState = ThreadState_Pause;
            while (GCThreadState != ThreadState_Pause) ;
            VMThreadState = ThreadState_Running;
        }
        cid = current_runtime_block->ExecContent[call_stack_top->Address++];
        switch (cid) {
            #pragma region xxmov
            case vbmov:
                *(vbyte *)(*(calculate_stack_top - 1)) = *((vbyte *)calculate_stack_top);
                calculate_stack_top -= 2;
                break;
            case vi32mov:
                *(unsigned int *)(*(calculate_stack_top - 1)) = *((unsigned int *)calculate_stack_top);
                calculate_stack_top -= 2;
                break;
            case vi64mov:
                *((ullong *)(*(calculate_stack_top - 1))) = *((ullong *)calculate_stack_top);
                calculate_stack_top -= 2;
                break;
            case vfmov:
                *(double *)(*(calculate_stack_top - 1)) = *((double *)calculate_stack_top);
                calculate_stack_top -= 2;
                break;
            case vomov:
                {
                    struct VM_ObjectInfo *lst_obj = (struct VM_ObjectInfo*)*(ullong*)*(calculate_stack_top - 1);
                    if (lst_obj != NULL) {
                        lst_obj->VarQuoteCount -= 2, lst_obj->QuoteCount -= 2;
                        VM_GC(lst_obj);
                    }
                    *(ullong *)*(calculate_stack_top - 1) = *calculate_stack_top;
                    calculate_stack_top -= 2;
                }
                break;
            case mbmov:
                {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)*(calculate_stack_top - 2);
                    obj->VarQuoteCount--, obj->QuoteCount--;
                    VM_GC(obj);
                    *(vbyte *)*(calculate_stack_top - 1) = *(vbyte *)calculate_stack_top;
                    calculate_stack_top -= 3;
                }
                break;
            case mi32mov:
                {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)*(calculate_stack_top - 2);
                    obj->VarQuoteCount--, obj->QuoteCount--;
                    VM_GC(obj);
                    *(unsigned int *)*(calculate_stack_top - 1) = *(unsigned int *)calculate_stack_top;
                    calculate_stack_top -= 3;
                }
                break;
            case mi64mov:
                {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)*(calculate_stack_top - 2);
                    obj->VarQuoteCount--, obj->QuoteCount--;
                    VM_GC(obj);
                    *(ullong *)*(calculate_stack_top - 1) = *(ullong *)calculate_stack_top;
                    calculate_stack_top -= 3;
                }
                break;
             case mfmov:
                {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)*(calculate_stack_top - 2);
                    obj->VarQuoteCount--, obj->QuoteCount--;
                    VM_GC(obj);
                    *(double *)*(calculate_stack_top - 1) = *(double *)calculate_stack_top;
                    calculate_stack_top -= 3;
                }
                break;
            case momov:
                {
                    struct VM_ObjectInfo *par = (struct VM_ObjectInfo *)*(calculate_stack_top - 2),
                                        *lst_obj = (struct VM_ObjectInfo *)*(ullong *)*(calculate_stack_top - 1);
                    par->VarQuoteCount--, par->QuoteCount--;
                    VM_GC(par);
                    if (lst_obj != NULL) {
                        lst_obj->QuoteCount -= 2, lst_obj->VarQuoteCount--;
                        VM_GC(lst_obj);
                    }
                    *(ullong *)*(calculate_stack_top - 1) = *calculate_stack_top;
                    if ((struct VM_ObjectInfo*)*calculate_stack_top != NULL) 
                        ((struct VM_ObjectInfo*)*calculate_stack_top)->VarQuoteCount--;
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
                *((ullong *)(calculate_stack_top - 1)) += *((ullong *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case usub:
                *((ullong *)(calculate_stack_top - 1)) -= *((ullong *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case umul:
                *((ullong *)(calculate_stack_top - 1)) *= *((ullong *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case udiv:
                *((ullong *)(calculate_stack_top - 1)) /= *((ullong *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case umod:
                *((ullong *)(calculate_stack_top - 1)) %= *((ullong *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case badd:
                *((ullong *)(calculate_stack_top - 1)) += *((ullong *)calculate_stack_top);
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
                *((ullong *)(calculate_stack_top - 1)) = (*((ullong *)(calculate_stack_top - 1)) != *((ullong *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case eq:
                *((ullong *)(calculate_stack_top - 1)) = (*((ullong *)(calculate_stack_top - 1)) == *((ullong *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case gt:
                *((ullong *)(calculate_stack_top - 1)) = (*((ullong *)(calculate_stack_top - 1)) > *((ullong *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case ge:
                *((ullong *)(calculate_stack_top - 1)) = (*((ullong *)(calculate_stack_top - 1)) >= *((ullong *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case ls:
                *((ullong *)(calculate_stack_top - 1)) = (*((ullong *)(calculate_stack_top - 1)) < *((ullong *)calculate_stack_top));
                calculate_stack_top--;
                break;
            case le:
                *((ullong *)(calculate_stack_top - 1)) = (*((ullong *)(calculate_stack_top - 1)) <= *((ullong *)calculate_stack_top));
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
                *((ullong *)(calculate_stack_top - 1)) &= *((ullong *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case uor:
                *((ullong *)(calculate_stack_top - 1)) |= *((ullong *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case uxor:
                *((ullong *)(calculate_stack_top - 1)) ^= *((ullong *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case unot:
                *((ullong *)calculate_stack_top) = ~(*(ullong *)calculate_stack_top);
                break;
            case ulmv:
                *((ullong *)(calculate_stack_top - 1)) <<= *((ullong *)calculate_stack_top);
                calculate_stack_top--;
                break;
            case urmv:
                *((ullong *)(calculate_stack_top - 1)) >>= *((ullong *)calculate_stack_top);
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
                *calculate_stack_top = *(vbyte *)*calculate_stack_top;
                break;
            case vi32gvl:
                *calculate_stack_top = *(unsigned int *)*calculate_stack_top;
                break;
            case vi64gvl:
                *calculate_stack_top = *(ullong *)*calculate_stack_top;
                break;
            case vfgvl:
                *((double *)(calculate_stack_top)) = *(double *)*calculate_stack_top;
				break;
            case vogvl:
                *calculate_stack_top = (ullong)(struct VM_ObjectInfo*)*(ullong*)*calculate_stack_top;
                break;
            case mbgvl:
                {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)*(calculate_stack_top - 1);
                    obj->QuoteCount--, obj->VarQuoteCount--;
                    *(calculate_stack_top - 1) = *(vbyte *)*calculate_stack_top;
                    calculate_stack_top--;
                    if (!obj->QuoteCount) VM_GC(obj);
                }
                break;
            case mi32gvl:
                {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)*(calculate_stack_top - 1);
                    obj->QuoteCount--, obj->VarQuoteCount--;
                    *(calculate_stack_top - 1) = *(unsigned *)*calculate_stack_top;
                    calculate_stack_top--;
                    if (!obj->QuoteCount) VM_GC(obj);
                }
                break;
            case mi64gvl:
                {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)*(calculate_stack_top - 1);
                    obj->QuoteCount--, obj->VarQuoteCount--;
                    *(ullong *)(calculate_stack_top - 1) = *(ullong *)*calculate_stack_top;
                    calculate_stack_top--;
                    if (!obj->QuoteCount) VM_GC(obj);
                }
                break;
            case mfgvl:
                {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)*(calculate_stack_top - 1);
                    obj->QuoteCount--, obj->VarQuoteCount--;
                    *(double *)(calculate_stack_top - 1) = *(double *)*calculate_stack_top;
                    calculate_stack_top--;
                    if (!obj->QuoteCount) VM_GC(obj);
                }
                break;
            case mogvl:
                {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)*(calculate_stack_top - 1);
                    obj->QuoteCount--, obj->VarQuoteCount--;
                    *(ullong *)(calculate_stack_top - 1) = *(ullong *)*calculate_stack_top;
                    calculate_stack_top--;
                    if (!obj->QuoteCount) VM_GC(obj);
                }
                break;
            #pragma endregion
            #pragma region object operator 
            case _new:
                {
                    struct VM_ObjectInfo *obj = VM_CreateObjectInfo(*calculate_stack_top);
                    obj->QuoteCount++, obj->VarQuoteCount++;
                    *calculate_stack_top = (ullong)obj;
                }
                break;
            case mem:
                {
                    arg1 = *(ullong *)&current_runtime_block->ExecContent[call_stack_top->Address];
                    call_stack_top->Address += sizeof(ullong);
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo*)*(calculate_stack_top++);
                    *calculate_stack_top = (ullong)(obj->Data + arg1);
                }
                break;
            case omem:
                {
                    //find the address
                    arg1 = *(ullong *)&current_runtime_block->ExecContent[call_stack_top->Address];
                    call_stack_top->Address += sizeof(ullong);
                    //get the object
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo*)*(calculate_stack_top++);
                    *calculate_stack_top = (ullong)(obj->Data + arg1);
                    //set the flag
                    *(obj->Flag + VM_ObjectInfo_FlagAddr(arg1)) |= VM_ObjectInfo_FlagBit(arg1);
                    if ((struct VM_ObjectInfo*)*(ullong*)*calculate_stack_top != NULL)
                        ((struct VM_ObjectInfo*)*(ullong*)*calculate_stack_top)->QuoteCount++,
                        ((struct VM_ObjectInfo*)*(ullong*)*calculate_stack_top)->VarQuoteCount++;
                }
                break;
            case arrnew:
                {
                    arg1 = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                    call_stack_top->Address += sizeof(ullong);
                    for (int i = arg1; i >= 0; i--) tmp_data[i] = *(calculate_stack_top--);
                    tmp_data[0] = min(sizeof(ullong), tmp_data[0]);
                    for (int i = 1; i <= arg1; i++) tmp_data[i] *= tmp_data[i - 1];
                    struct VM_ObjectInfo *obj = VM_CreateObjectInfo((arg1 << 3) + tmp_data[arg1]);
                    obj->QuoteCount++, obj->VarQuoteCount++;
                    for (int i = 0; i < arg1; i++)
                        *(ullong *)&obj->Data[i << 3] = tmp_data[i];
                    *(++calculate_stack_top) = (ullong)obj;
                }
                break;
            case arrmem: 
                {
                    // Get the dimension
                    arg1 = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                    call_stack_top->Address += sizeof(ullong);
                    // Get the index of each dimension
                    for (int i = arg1; i >= 0; i--) tmp_data[i] = *(calculate_stack_top--);
                    calculate_stack_top++;
                    // Calculate the address
                    arg2 = arg1 * sizeof(ullong);
                    vbyte *obj_data = ((struct VM_ObjectInfo*)tmp_data[0])->Data;
                    for (ullong i = 1, p = 0; i <= arg1; i++, p += sizeof(ullong)) 
                        arg2 += tmp_data[i] * (*(ullong*)(obj_data + p));
                    // Update the calculate_stack
                    *(++calculate_stack_top) = (ullong)(obj_data + arg2);
                }
                break;
            case arrmem1:
                {
                    arg2 = *(calculate_stack_top--), arg1 = *calculate_stack_top;
                    vbyte *obj_data = ((struct VM_ObjectInfo*)arg1)->Data;
                    *(++calculate_stack_top) = (ullong)(obj_data + sizeof(ullong) + arg2 * (*(ullong *)obj_data));
                }
                break;
            case arromem:
                {
                    // Get the dimension
                    arg1 = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                    call_stack_top->Address += sizeof(ullong);
                    // Get the index of each dimension
                    for (int i = arg1; i >= 0; i--) tmp_data[i] = *(calculate_stack_top--);
                    calculate_stack_top++;
                    // Calculate the address
                    arg2 = arg1 * sizeof(ullong);
                    vbyte *obj_data = ((struct VM_ObjectInfo*)tmp_data[0])->Data;
                    for (ullong i = 1, p = 0; i <= arg1; i++, p += sizeof(ullong)) 
                        arg2 += tmp_data[i] * (*(ullong*)(obj_data + p));
                    // Update the calculate_stack
                    *(++calculate_stack_top) = (ullong)(obj_data + arg2);
                    // Set Flag
                    *(((struct VM_ObjectInfo*)tmp_data[0])->Flag + VM_ObjectInfo_FlagAddr(arg2)) |= VM_ObjectInfo_FlagBit(arg2);
                    
                    struct VM_ObjectInfo *mem_obj = (struct VM_ObjectInfo*)*(ullong*)*calculate_stack_top;
                    if (mem_obj != NULL) mem_obj->VarQuoteCount++, mem_obj->QuoteCount++;
                }
                break;
            case arromem1:
                {
                    // arg2 illustrates the index, arg1 is the array object
                    arg2 = *(calculate_stack_top--), arg1 = *calculate_stack_top;
                    vbyte *obj_data = ((struct VM_ObjectInfo*)arg1)->Data;
                    arg2 *= *(ullong *)obj_data; arg2 += sizeof(ullong);
                    *(++calculate_stack_top) = (ullong)(obj_data + arg2);
                    // Set Flag
                    *(((struct VM_ObjectInfo*)arg1)->Flag + VM_ObjectInfo_FlagAddr(arg2)) |= VM_ObjectInfo_FlagBit(arg2);

                    struct VM_ObjectInfo *mem_obj = (struct VM_ObjectInfo*)*(ullong*)*calculate_stack_top;
                    if (mem_obj != NULL) mem_obj->VarQuoteCount++, mem_obj->QuoteCount++;
                }
                break;
            case pack:
                {
                    struct VM_ObjectInfo* obj = VM_CreateObjectInfo(9);
                    obj->QuoteCount++, obj->VarQuoteCount++;
                    *(ullong *)obj->Data = *calculate_stack_top;
                    *calculate_stack_top = (ullong)obj;
                }
                break;
            case unpack:
                {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)*calculate_stack_top;
                    obj->VarQuoteCount--, obj->QuoteCount--;
                    *(calculate_stack_top) = *(ullong*)obj->Data;
                }
                break;
            #pragma endregion
            #pragma region jump and function operator
            case jmp:
                call_stack_top->Address = *(ullong*)(current_runtime_block->ExecContent + call_stack_top->Address);
                break;
            case jz:
                if (*(calculate_stack_top--)) call_stack_top->Address += sizeof(ullong);
                else call_stack_top->Address = *(ullong*)(current_runtime_block->ExecContent + call_stack_top->Address);
                break;
            case jp:
                if (*(calculate_stack_top--)) 
                    call_stack_top->Address = *(ullong*)(current_runtime_block->ExecContent + call_stack_top->Address);
                else call_stack_top->Address += sizeof(ullong);
                break;
            case call:
                arg1 = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(ullong);
                arg2 = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(ullong);
                Function_Call(arg1, arg2, -1);
                break;
            case ecall:
                arg1 = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(ullong);
                arg2 = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(ullong);
                Function_ECall(arg1, arg2);
                break;
            case ret:
                Function_Return();
                break;
            #pragma endregion
            #pragma region constant and varibale operator
            case setvar:
                arg1 = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(ullong);
                // call_stack_top->VariableCount = arg1, call_stack_top->UsedMaxVariable = 0;
                call_stack_top->Variables = malloc((arg1 + 1) * sizeof(ullong));
                // printf("malloc %llx\n", call_stack_top->Variables);
                for (int i = 0; i < arg1; i++) call_stack_top->Variables[i] = 0;
                break;
            case poparg:
                arg1 = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(ullong);
                for (int i = 0; i < arg1; i++) call_stack_top->Variables[i] = arg_data[i];
                break;
            case push:
                *(++calculate_stack_top) = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(ullong);
                break;
            case push0:
                *(++calculate_stack_top) = 0;
                break;
            case push1:
                *(++calculate_stack_top) = 1;
                break;
            case pvar:
                *(++calculate_stack_top) = (ullong)(
                    call_stack_top->Variables + *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address));
                // call_stack_top->UsedMaxVariable = max(call_stack_top->UsedMaxVariable, *(uulong *)(current_runtime_block->ExecContent + call_stack_top->Address));
                call_stack_top->Address += sizeof(ullong);
                break;
            case pvar0:
                *(++calculate_stack_top) = (ullong)(&call_stack_top->Variables[0]);
                // call_stack_top->UsedMaxVariable = max(call_stack_top->UsedMaxVariable, 0);
                break;
            case pvar1:
                *(++calculate_stack_top) = (ullong)(&call_stack_top->Variables[1]);
                // call_stack_top->UsedMaxVariable = max(call_stack_top->UsedMaxVariable, 1);
                break;
             case pvar2:
                *(++calculate_stack_top) = (ullong)(&call_stack_top->Variables[2]);
                // call_stack_top->UsedMaxVariable = max(call_stack_top->UsedMaxVariable, 2);
                break;
             case pvar3:
                *(++calculate_stack_top) = (ullong)(&call_stack_top->Variables[3]);
                // call_stack_top->UsedMaxVariable = max(call_stack_top->UsedMaxVariable, 3);
                break;
            case povar:
                *(++calculate_stack_top) = (ullong)(
                    call_stack_top->Variables + *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address));
                // call_stack_top->UsedMaxVariable = max(call_stack_top->UsedMaxVariable, *(uulong *)(current_runtime_block->ExecContent + call_stack_top->Address));
                call_stack_top->Address += sizeof(ullong);
                {
                    struct VM_ObjectInfo* obj = (struct VM_ObjectInfo *)*(ullong*)*calculate_stack_top;
                    if (obj != NULL) obj->QuoteCount++, obj->VarQuoteCount++;
                }
                break;
            case povar0:
                *(++calculate_stack_top) = (ullong)(&call_stack_top->Variables[0]);
                // call_stack_top->UsedMaxVariable = max(call_stack_top->UsedMaxVariable, 0);
                {
                    struct VM_ObjectInfo* obj = (struct VM_ObjectInfo *)*(ullong*)*(calculate_stack_top);
                    if (obj != NULL) obj->QuoteCount++, obj->VarQuoteCount++;
                }
                break;
            case povar1:
                *(++calculate_stack_top) = (ullong)(&call_stack_top->Variables[1]);
                // call_stack_top->UsedMaxVariable = max(call_stack_top->UsedMaxVariable, 1);
                {
                    struct VM_ObjectInfo* obj = (struct VM_ObjectInfo *)*(ullong*)*(calculate_stack_top);
                    if (obj != NULL) obj->QuoteCount++, obj->VarQuoteCount++;
                }
                break;
            case povar2:
                *(++calculate_stack_top) = (ullong)(&call_stack_top->Variables[2]);
                // call_stack_top->UsedMaxVariable = max(call_stack_top->UsedMaxVariable, 2);
                {
                    struct VM_ObjectInfo* obj = (struct VM_ObjectInfo *)*(ullong*)*calculate_stack_top;
                    if (obj != NULL) obj->QuoteCount++, obj->VarQuoteCount++;
                }
                break;
            case povar3:
                *(++calculate_stack_top) = (ullong)(&call_stack_top->Variables[3]);
                // call_stack_top->UsedMaxVariable = max(call_stack_top->UsedMaxVariable, 3);

                {
                    struct VM_ObjectInfo* obj = (struct VM_ObjectInfo *)*(ullong*)*(calculate_stack_top);
                    if (obj != NULL) obj->QuoteCount++, obj->VarQuoteCount++;
                }
                break;
            case pglo:
                *(++calculate_stack_top) = (ullong)(
                    current_runtime_block->Glomem + *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address));
                call_stack_top->Address += sizeof(ullong);
                break;
            case poglo:
                *(++calculate_stack_top) = (ullong)(
                    current_runtime_block->Glomem + *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address));
                call_stack_top->Address += sizeof(ullong);
                {
                    struct VM_ObjectInfo* obj = (struct VM_ObjectInfo *)*(ullong*)*(calculate_stack_top);
                    if (obj != NULL) obj->QuoteCount++, obj->VarQuoteCount++;
                }
                break;
            case pstr:
                *(++calculate_stack_top) = (ullong)(&current_runtime_block->StringObjectList[
                    *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address)
                ]);
                call_stack_top->Address += sizeof(ullong);
                {
                    struct VM_ObjectInfo* obj = (struct VM_ObjectInfo *)*(ullong*)*(calculate_stack_top);
                    if (obj != NULL) obj->QuoteCount++, obj->VarQuoteCount++;
                }
                break;
            case cpy:
                *(calculate_stack_top + 1) = *(calculate_stack_top);
                calculate_stack_top++;
                break;
            case ocpy:
                *(calculate_stack_top + 1) = *(calculate_stack_top);
                calculate_stack_top++;
                if ((struct VM_ObjectInfo *)*calculate_stack_top != NULL) {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)*calculate_stack_top;
                    obj->VarQuoteCount++, obj->QuoteCount++;
                }
                break;
            case pop:
                --calculate_stack_top;
                break;
            case opop:
                {
                    struct VM_ObjectInfo *obj = (struct VM_ObjectInfo *)calculate_stack_top;
                    if (obj != NULL) obj->VarQuoteCount--, obj->QuoteCount--;
                    --calculate_stack_top;
                }
                break;
            #pragma endregion
            #pragma region system operator
            case sys:
                arg1 = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                call_stack_top->Address += sizeof(ullong);

                if (arg1 < 6)
                    System_IO(arg1, *(calculate_stack_top--));
                else if (arg1 < 12)
                    *(++calculate_stack_top) = System_IO(arg1, 0);
                break;
            #pragma endregion
            case excmd:
                {
                    ullong excid = *(ullong *)(current_runtime_block->ExecContent + call_stack_top->Address);
                    call_stack_top->Address += sizeof(ullong);
                    switch (excid) {
                        #pragma region ++ and --
                        case EX_vbpinc:
                            *(vbyte *)calculate_stack_top = ++*(vbyte *)*calculate_stack_top;
                            break;
                        case EX_vi32pinc:
                            *(int *)calculate_stack_top = ++*(int *)*calculate_stack_top;
                            break;
                        case EX_vi64pinc:
                            *(long long *)calculate_stack_top = ++*(long long *)*calculate_stack_top;
                            break;
                        case EX_vupinc:
                            *calculate_stack_top = ++*(ullong *)*calculate_stack_top;
                            break;
                        case EX_vbsinc:
                            *calculate_stack_top = *(vbyte *)*calculate_stack_top++;
                            break;
                        case EX_vi32sinc:
                            *calculate_stack_top = (*(int *)*calculate_stack_top)++;
                            *((int *)calculate_stack_top + 1) = 0;
                            break;
                        case EX_vi64sinc:
                            *(long long *)calculate_stack_top = (*(long long *)*calculate_stack_top)++;
                            break;
                        case EX_vusinc:
                            *calculate_stack_top = *(ullong *)*calculate_stack_top++;
                            break;

                        case EX_vbpdec:
                            *(vbyte *)calculate_stack_top = --*(vbyte *)*calculate_stack_top;
                            break;
                        case EX_vi32pdec:
                            *(int *)calculate_stack_top = --*(int *)*calculate_stack_top;
                            break;
                        case EX_vi64pdec:
                            *(long long *)calculate_stack_top = --*(long long *)*calculate_stack_top;
                            break;
                        case EX_vupdec:
                            *calculate_stack_top = --*(ullong *)*calculate_stack_top;
                            break;
                        case EX_vbsdec:
                            *calculate_stack_top = *(vbyte *)*calculate_stack_top--;
                            break;
                        case EX_vi32sdec:
                            *calculate_stack_top = (*(int *)*calculate_stack_top)--;
                            *((int *)calculate_stack_top + 1) = 0;
                            break;
                        case EX_vi64sdec:
                            *(long long *)calculate_stack_top = (*(long long *)*calculate_stack_top)--;
                            break;
                        case EX_vusdec:
                            *calculate_stack_top = *(ullong *)*calculate_stack_top--;
                            break;

                        case EX_mbpinc:
                            
                            break;
                        case EX_mi32pinc:
                            break;
                        case EX_mi64pinc:
                            break;
                        case EX_mupinc:
                            break;
                        case EX_mbsinc:
                            break;
                        case EX_mi32sinc:
                            break;
                        case EX_mi64sinc:
                            break;
                        case EX_musinc:
                            break;

                        case EX_mbpdec:
                            break;
                        case EX_mi32pdec:
                            break;
                        case EX_mi64pdec:
                            break;
                        case EX_mupdec:
                            break;
                        case EX_mbsdec:
                            break;
                        case EX_mi32sdec:
                            break;
                        case EX_mi64sdec:
                            break;
                        case EX_musdec:
                            break;
                        #pragma endregion
                    }
                }
                break;
        }
    }
    VMThreadState = ThreadState_Exit;
    return NULL;
}

void InitVM(ullong calculate_stack_size, ullong call_stack_size) {
    LibraryMap = HashMap_CreateMap(HASHMAP_DEFAULT_RANGE);
    ExternMap = HashMap_CreateMap(HASHMAP_DEFAULT_RANGE);
    calculate_stack = (ullong *) malloc(sizeof(ullong) * calculate_stack_size);
    call_stack = (ullong *) malloc(sizeof(struct CallFrame) * call_stack_size);
    calculate_stack_top = calculate_stack;
    call_stack_top = call_stack;
    runtime_block_count = 0;
}
void VM_Launch(char *path, ullong calculate_stack_size, ullong call_stack_size) {
    InitVM(calculate_stack_size, call_stack_size);
    VM_InitGC();
    // PrintLog("Finish initialization process.\n", FOREGROUND_BLUE);
    VMThreadState = ThreadState_Running;
    GCThreadState = ThreadState_Pause;
    
    pthread_t vm_thread, gc_thread;
    pthread_create(&gc_thread, NULL, VM_GCThread, NULL);
    clock_t st = clock();
    VM_VMThread((void *)path);
    printf("\n exited, cost %.3lf\n", 1.0 * (clock() - st) / CLOCKS_PER_SEC);
    pthread_cancel(gc_thread);
    pthread_exit(NULL);
}