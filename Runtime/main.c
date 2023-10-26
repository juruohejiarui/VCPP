#include "gloconst.h"
#include "vm.h"

int main(int argc, char **argv) {
    // printf("Hello world\n");
    // freopen("test.out", "w", stdout);
    char *vexe_path = "./test.vexe";
    uulong calculate_stack_size = DEFAULT_CALCUATE_STACK_SIZE,
            call_stack_size = DEFAULT_CALL_STACK_SIZE;
    
    // handle all the arguments
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (!strcmp("-calc", argv[i])) calculate_stack_size = StringToUint64(argv[i + 1]) << 10;
            else if (!strcmp("-call", argv[i])) call_stack_size = StringToUint64(argv[i + 1]) << 10;
        } else {
            int len = strlen(argv[i]);
            if (len > 5 && !strcmp(argv[i] + len - 5, ".vexe")) vexe_path = argv[i];
        }
    }
    VM_Launch(vexe_path, calculate_stack_size, call_stack_size);
    return 0;
}