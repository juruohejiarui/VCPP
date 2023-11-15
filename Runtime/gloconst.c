#include "gloconst.h"

int cmdsize[CMD0_COUNT + CMD1_COUNT + CMD2_COUNT + 1], cmdargcnt[CMD0_COUNT + CMD1_COUNT + CMD2_COUNT + 1];

void SetColor(short foreground_color) {
    #ifndef DEBUG_FILESTYLE
    if (foreground_color & 0x08) printf("\e[1m");
    printf("\e[3%cm", (foreground_color & 0x07) + '0');
    #endif
}
void PrintError(char *info, int line) {
    if (line != -1) {
        SetColor(FOREGROUND_WHITE);
        printf("line = %d: ", line);
    }
    SetColor(FOREGROUND_RED);
    printf("%s\n", info);
    SetColor(FOREGROUND_WHITE);
}

void PrintLog(char* info, short foreground_color) {
    SetColor(foreground_color);
    printf("%s", info);
    SetColor(FOREGROUND_WHITE);
}

void InitCommandInfo() {
    for (int i = 0; i <= CMD0_COUNT; i++) 
        cmdsize[i] = sizeof(uint8_t), cmdargcnt[i] = 0;
    for (int i = CMD0_COUNT + 1; i <= CMD1_COUNT; i++) 
        cmdsize[i] = sizeof(uint8_t) + sizeof(uint64_t), cmdargcnt[i] = 1;
    for (int i = CMD0_COUNT + CMD1_COUNT + 1; i <= CMD0_COUNT + CMD1_COUNT + CMD2_COUNT; i++)
        cmdsize[i] = sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint64_t), cmdargcnt[i] = 2;
}

int GetCommandSize(int cmdid) { return cmdsize[cmdid]; }

int GetCommandArgCount(int cmdid) { return cmdargcnt[cmdid]; }

uint64_t StringToUint64(char *str) {
    uint64_t res = 0;
    while (*str != '\0') res = (res << 3) + (res << 1) + (*str - '0');
    return res;
}
