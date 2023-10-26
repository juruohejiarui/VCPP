#pragma once
#include "gloconst.h"

#define DEFAULT_CALCUATE_STACK_SIZE (1 << 24)
#define DEFAULT_CALL_STACK_SIZE (1 << 24)

void VM_Launch(char *path, uulong calculate_stack_size, uulong call_stack_size);

extern uulong *calculate_stack_top;