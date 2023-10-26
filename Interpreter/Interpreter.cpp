#include "gloconst.h"
#include "pretreat.h"
#include "vasm.h"
#include "pbuilder.h"
#include "vcppcpl.h"

using namespace Interpreter;

void test() {
    ifstream ifs("test.in", ios::in);
    int task_cnt; 
    ifs >> task_cnt;
    for (int id = 1; id <= task_cnt; id++) {
        printf("Task %d\n", id);
        int type, vc_cnt, vlib_cnt; 
        ifs >> type >> vc_cnt >> vlib_cnt;
        ProjectBuilder_Init();
        int res = 0;
        for (int i = 0; i < vc_cnt; i++) {
            string path; ifs >> path;
            printf("Load vc file : %s\n", path.c_str());
            res |= ProjectBuilder_LoadVcpp(path);
        }
        for (int i = 0; i < vlib_cnt; i++) {
            string path; ifs >> path;
            printf("Load vlib file : %s\n", path.c_str());
            res |= ProjectBuilder_LoadVlib(path, true);
        }
        if (res == -1) { PrintError("Fail at loading process..."); return ; }
        string target_path; ifs >> target_path;
        if (type == 1)
            res = ProjectBuilder_BuildVexe(target_path);
        else res = ProjectBuilder_BuildVlib(target_path);
        if (res == -1) { PrintError("Fail at building process..."); return ; }
    }
    ifs.close();
    // printf("Running\n");
    // string vexe_path = "test.vexe";
    // system(("./vm_c " + vexe_path + " < input.in").c_str());
}


int main() {
    #ifdef DEBUG_FILESTYLE
    freopen("input.in", "r", stdin);
    // freopen("test.out", "w", stdout);
    #endif
    printf("test\n");
    test();
    return 0;
}