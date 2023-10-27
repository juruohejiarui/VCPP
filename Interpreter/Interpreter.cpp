#include "gloconst.h"
#include "pretreat.h"
#include "vasm.h"
#include "pbuilder.h"
#include "vcppcpl.h"

using namespace Interpreter;

void test() {
    PrintLog("Test Module\n");
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
}

int main(int argc, char **argv) {
    vector<string> rely, vcpp;
    string target = "a.vexe";
    bool is_vexe = false;
    if (argc == 1) { test(); return 0; }
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-vexe" || arg == "-vlib") {
            if (i == argc - 1 || argv[i + 1][0] == '-') {
                PrintError("Miss the target path");
                return -1;
            }
            target = argv[i + 1];
            is_vexe = (arg == "-vexe");
        } else if (arg == "-rely") {
            for (i++; i < argc; i++) {
                string path = argv[i];
                if (path[0] == '-') { i--; break; }
                rely.emplace_back(path);
            }
        } else if (arg == "-vcpp") {
            for (i++; i < argc; i++) {
                string path = argv[i];
                if (path[0] == '-') { i--; break; }
                vcpp.emplace_back(path);
            }
        }
    }

    if (vcpp.empty()) {
        PrintError("There is no .vc file");
        return 1;
    }

    ProjectBuilder_Init();
    int res = 0;
    for (auto &path : vcpp) res |= ProjectBuilder_LoadVcpp(path);
    for (auto &path : rely) res |= ProjectBuilder_LoadVlib(path);
    if (res == -1) { PrintError("Fail at loading process..."); return -1; }
    if (is_vexe) res = ProjectBuilder_BuildVexe(target);
    else res = ProjectBuilder_BuildVlib(target);
    if (res == -1) { PrintError("Fail at building process..."); return -1; }
    return 0;
}