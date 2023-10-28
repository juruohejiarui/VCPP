#include "syscall.h"

ullong System_IO(ullong id, ullong arg) {
    ullong res = 0;
    switch (id) {
        case 0:
            printf("%d", *(int*)&arg);
            return 0;
        case 1:
            printf("%lld", *(long long*)&arg);
            return 0;
        case 2:
            printf("%llu", arg);
            return 0;
        case 3:
            printf("%lf", *(double*)&arg);
            return 0;
        case 4:
            printf("%d", arg);
        case 5:
            putchar(*(char*)&arg);
            return 0;
        case 6:
            scanf("%d", &res);
            return res;
        case 7:
            scanf("%lld", &res);
            return res;
        case 8:
            scanf("%llu", &res);
            return res;
        case 9:
            scanf("%lf", &res);
            return res;
        case 10:
            scanf("%d", &res);
            return res;
        case 11:
            res = getchar();
            return res;
    }
    return -1;
}