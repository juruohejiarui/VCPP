#include "syscall.h"

uulong System_IO(uulong id, uulong arg) {
    uulong res = 0;
    switch (id) {
        case 0:
            printf("%d\n", *(int*)&arg);
            return 0;
        case 1:
            printf("%lld\n", *(long long*)&arg);
            return 0;
        case 2:
            printf("%llu\n", arg);
            return 0;
        case 3:
            printf("%lf\n", *(double*)&arg);
            return 0;
        case 4:
            printf("%d\n", arg);
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