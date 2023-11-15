#pragma once
#include "gloconst.h"

#define DEFAULT_CALCUATE_STACK_SIZE (1 << 24)
#define DEFAULT_CALL_STACK_SIZE (1 << 24)

void VM_Launch(char *path, uint64_t calculate_stack_size, uint64_t call_stack_size);

extern uint64_t *calculate_stack_top;