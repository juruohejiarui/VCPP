#pragma once
#include "gloconst.h"

#define DEFAULT_CALCUATE_STACK_SIZE (1 << 24)
#define DEFAULT_CALL_STACK_SIZE (1 << 24)

void VM_Launch(char *path, ullong calculate_stack_size, ullong call_stack_size);

extern ullong *calculate_stack_top;