#include "pbuilder.h"
#include "vasm.h"
#include <unordered_set>
#include <unordered_map>

namespace Interpreter {
    vector<VLibraryInfo*> vlibraries;
    unordered_map<string, int> extern_link;
    unordered_set<string> direct_rely;
    unordered_set<string> vlibrary_path;
    vector<CplNode*> vc_node;
    vector<OutFileInfo*> outfile;

    static string command_name_list[] = { COMMAND_NAME_LS };

    OutFileInfo::~OutFileInfo() {
        printf("Release %s\n", Path.c_str());
        if (ExposeContent != nullptr) delete[] ExposeContent;
        if (RelyContent != nullptr) delete[] RelyContent;
        if (ExternContent != nullptr) delete[] ExternContent;
        if (StringContent != nullptr) delete[] StringContent;
        if (ExecContent != nullptr) delete[] ExecContent;
        if (Nodes != nullptr) {
            for (int i = 0; i < NodeCount; i++) delete Nodes[i];
            delete[] Nodes;
        }
    }

    void ProjectBuilder_Init() {
        IdenSystem_Init();
        for (auto library : vlibraries) delete library;
        vlibraries.clear();
        vlibrary_path.clear();
        extern_link.clear();
        direct_rely.clear();
        vc_node.clear();
        for (auto outfile : outfile) delete outfile;
        outfile.clear();
    }

    //Load the vc file build a CplTree of definition, and update the idensystem
	int ProjectBuilder_LoadVcpp(string &path) {
        vector<Token> tkls; int res = 0;
        ifstream ifs(path);
        if (!ifs.is_open()) { PrintError("Fail to open file \"" + path + "\""); return -1; }
        Pretreat_Init();
        while (!ifs.eof()) {
            string str; getline(ifs, str), str.push_back('\n');
            res |= Pretreat_GetTokenList(tkls, str, false);
        }
        if (res == -1) { PrintError("Fail to build the token list"); return -1; }
        CplNode *nd = CplTree_Build(&tkls);
        if (nd == nullptr || nd->Type == CplNodeType::Error) { PrintError("Fail to build the CplTree..."); return -1; }
        nd->Type = CplNodeType::Definition;
        vc_node.push_back(nd);
        return 0;
    }

    //Load the vlib file, build a CplTree of declaration, and update the idensystem
	int ProjectBuilder_LoadVlib(string &path, bool is_direct) {
        if (is_direct) direct_rely.insert(path);
        if (vlibrary_path.count(path)) return 0;
        vlibrary_path.insert(path);
        VLibraryInfo *lib = new VLibraryInfo;
        ifstream ifs(path, ios::binary);
        if (!ifs.is_open()) { PrintError("Fail to open VLibrary file : " + path); return -1; }

        //read the infomation is the vlib file
        lib->Path = path;
        lib->IsDirectRely = is_direct;
        lib->DefinitionContent = ReadString(ifs);
        lib->ExposeCount = ReadUlong(ifs);
        lib->ExposeContent = new pair<string, ulong>[lib->ExposeCount];
        for (int i = 0; i < lib->ExposeCount; i++)
            lib->ExposeContent[i].first = ReadString(ifs),
            lib->ExposeContent[i].second = ReadUlong(ifs);
        lib->RelyCount = ReadUlong(ifs);
        lib->RelyContent = new string[lib->RelyCount];
        for (int i = 0; i < lib->RelyCount; i++) 
            lib->RelyContent[i] = ReadString(ifs);
        lib->ExternCount = ReadUlong(ifs);
        lib->ExternContent = new string[lib->ExternCount];
        for (int i = 0; i < lib->ExternCount; i++)
            lib->ExternContent[i] = ReadString(ifs);
        lib->ExecSize = ReadUlong(ifs);
        lib->GlomemSize = ReadUlong(ifs);
        lib->StringCount = ReadUlong(ifs);
        lib->StringContent = new string[lib->StringCount];
        for (int i = 0; i < lib->StringCount; i++)
            lib->StringContent[i] = ReadString(ifs);
        //there is not main function in vlib file
        // lib->HasMain = ReadByte(ifs);
        // lib->MainAddr = ReadUlong(ifs);
        //ignore the ExecContent because it's not necessary.
        vlibraries.push_back(lib);

        int res = 0;
        //build the CplTree of the declaration
        vector<Token> tkls;
        Pretreat_Init();
        res = Pretreat_GetTokenList(tkls, lib->DefinitionContent, false);
        if (res == -1) return -1;
        CplNode *root = CplTree_Build(&tkls);
        if (root == nullptr || root->Type == CplNodeType::Error) return -1;
        root->Type = CplNodeType::Declaration;
        lib->Nodes = new CplNode*[lib->NodeCount = 1];
        lib->Nodes[0] = root;

        //load the libraries which this library relies on.
        for (int i = 0; i < lib->RelyCount; i++) res |= ProjectBuilder_LoadVlib(lib->RelyContent[i]);
        return res;
    }

    int ProjectBuilder_LoadOutFile(string &path) {
        OutFileInfo *ofile = new OutFileInfo();
        ifstream ifs(path, ios::binary);
        if (!ifs.is_open()) { PrintError("Fail to open Output File: " + path); return -1; }

        ofile->Path = path;
        ofile->ExposeCount = ReadUlong(ifs);
        if (ofile->ExposeCount) {
            ofile->ExposeContent = new pair<string, ulong>[ofile->ExposeCount];
            for (int i = 0; i < ofile->ExposeCount; i++)
                ofile->ExposeContent[i].first = ReadString(ifs),
                ofile->ExposeContent[i].second = ReadUlong(ifs);
        }
        ofile->RelyCount = ReadUlong(ifs);
        if (ofile->RelyCount) {
            ofile->RelyContent = new string[ofile->RelyCount];
            for (int i = 0; i < ofile->RelyCount; i++) 
                ofile->RelyContent[i] = ReadString(ifs);
        }
        ofile->ExternCount = ReadUlong(ifs);
        if (ofile->ExternCount) {
            ofile->ExternContent = new string[ofile->ExternCount];
            for (int i = 0; i < ofile->ExternCount; i++)
                ofile->ExternContent[i] = ReadString(ifs);
        }
        
        ofile->ExecSize = ReadUlong(ifs);
        ofile->GlomemSize = ReadUlong(ifs);
        ofile->StringCount = ReadUlong(ifs);
        if (ofile->StringCount) {
            ofile->StringContent = new string[ofile->StringCount];
            for (int i = 0; i < ofile->StringCount; i++)
                ofile->StringContent[i] = ReadString(ifs);
        }
        ofile->HasMain = ReadByte(ifs);
        ofile->MainAddr = ReadUlong(ifs);
        //Load the exec content
        ofile->ExecContent = new vbyte[ofile->ExecSize];
        ifs.read((char*)ofile->ExecContent, sizeof(vbyte) * ofile->ExecSize);

        outfile.push_back(ofile);
        return 0;
    }

    int Build_OutputFile(string vasm_path) {
        int res = 0;
        string vo_path = vasm_path.substr(0, vasm_path.size() - 5) + ".vo";
        Vasm_Init(vo_path);
        vector<Token> tkls;
        ifstream ifs(vasm_path);
        if (!ifs.is_open()) { PrintError("Fail to open vasm file : " + vasm_path); return -1; }
        Pretreat_Init();
        while (!ifs.eof()) {
            string str;
            getline(ifs, str), str.push_back('\n');
            res |= Pretreat_GetTokenList(tkls, str, true);
        }
        ifs.close();
        if (res == -1) return -1;
        res = Vasm_BuildOutfile(tkls);
        return res;
    }

    void Load_PreLabel(string &vasm_path, int separate_pos) {
        ofstream ofs(vasm_path);
        //write the rely paths
        for (auto path : direct_rely)
            ofs << "#RELY \"" << path << "\"\n";
        //Load the exposed and extern labels
        //write all the functions whose visibility is not private
        auto build_label = [&ofs](string label, CplNode* root) {
            const auto& func = [&ofs, &label](auto&& self, CplNode* root) -> void {
                switch(root->Type) {
                    case CplNodeType::FuncDefine:
                        if (root->Content.Type == TokenType::Private) break;
                        ofs << label << root->Content.String << "\n";
                        break;
                    case CplNodeType::ClassDefine:
                    case CplNodeType::NamespaceDefine:
                    case CplNodeType::Definition:
                    case CplNodeType::Declaration:
                        if (root->Type == CplNodeType::ClassDefine && root->Content.Type == TokenType::Private) break;
                        for (CplNode* child : root->Children) self(self, child);
                        break;
                }
            };
            func(func, root);
        };
        ofs << "#GLOMEM " << GetGlobalMemorySize() << endl;
        for (int i = 0; i < separate_pos; i++) build_label("#EXPOSE ", vc_node[i]);
        for (int i = separate_pos; i < vc_node.size(); i++) build_label("#EXTERN ", vc_node[i]);
        ofs.close();
    }

    int Build_IdenSystem() {
        for (auto vlib : vlibraries)
            for (int i = 0; i < vlib->NodeCount; i++) vc_node.push_back(vlib->Nodes[i]);
         //build the idensystem, and check the expression type 
        int res = IdenSystem_LoadDefinition(vc_node);
        if (res == -1) { PrintError("Fail to load the definition structure"); return -1; }
        res = IdenSystem_LoadExpressionType(vc_node);
        if (res == -1) { PrintError("Fail to load the Expression Type"); return -1; }
        return 0;
    }

    static unordered_map<string, ulong> expmap;
    static unordered_map<string, ulong> extset;
    //write the rely , extern and expose labels
    int write_prelabel(ofstream &ofs, bool is_vlib) {
        //load the declaration
        if (is_vlib) {
            string def = IdenSystem_MakeDefinationSource();
            Write(ofs, def);
        }
        //deal with the expose labels and extern labels
        expmap.clear(), extset.clear();
        ulong CurExecSize = 0;
        for (auto &ofile : outfile) {
            for (int i = 0; i < ofile->ExposeCount; i++) {
                auto exp = ofile->ExposeContent[i];
                if (expmap.count(exp.first)) { PrintError("Multiple expose label : " + exp.first); return -1; }
                exp.second += CurExecSize;
                expmap.insert(exp);
            }
            CurExecSize += ofile->ExecSize;
        }
        for (auto &ofile : outfile) {
            for (int i = 0; i < ofile->ExternCount; i++) {
                auto ext = ofile->ExternContent[i];
                //ignore the extern label that exists in the project.
                if (expmap.count(ext)) ofile->ExternContent[i].append("-" + to_string(expmap[ext]));
                else extset.insert(make_pair(ext, 0));
            }
        }
        //there is no expose label in vexe file
        if (is_vlib) {
            Write(ofs, (ulong)expmap.size());
            for (auto &exp : expmap) Write(ofs, exp.first), Write(ofs, exp.second);
        }
        //write rely path
        Write(ofs, (ulong)vlibraries.size());
        for (auto &vlib : vlibraries) Write(ofs, vlib->Path);
        //write the extern labels
        Write(ofs, (ulong)extset.size()); 
        
        ulong ext_cnt = 0;
        for (auto &ext : extset) Write(ofs, ext.first), extset[ext.first] = ext_cnt++;
        return 0;
    }

    int WriteVEXEC(ofstream &ofs, bool is_vlib) {
        ulong exec_sz = 0, str_cnt = 0, glomem = 0, mainaddr = 0; vbyte has_addr = 0;
        auto is_addrcmd = [](vbyte cid) { return (jmp <= cid && cid <= jp) || (call <= cid && cid <= ecall); };
        for (auto &ofile : outfile) {
            glomem += ofile->GlomemSize;
            if (has_addr && ofile->HasMain) {
                PrintError("Multiple definition of Main\n");
                return -1;
            }
            has_addr |= ofile->HasMain;
            if (ofile->HasMain) mainaddr = ofile->MainAddr;
            str_cnt += ofile->StringCount;

            //modify the the arguments of jmp, jz, jp and call, ecall
            for (ulong pos = 0; pos < ofile->ExecSize; ) {
                vbyte &cid = ofile->ExecContent[pos];
                if (is_addrcmd(cid)) {
                    ulong &arg1 = *(ulong*)&ofile->ExecContent[pos + 1];
                    if (cid == ecall) {
                        auto minus_pos = ofile->ExternContent[arg1].find('-');
                        if (minus_pos != -1) { // it can be change into a "call"
                            ofile->ExecContent[pos] = call,
                            arg1 = stoull(ofile->ExternContent[arg1].substr(minus_pos + 1));
                        } else arg1 = extset[ofile->ExternContent[arg1]];
                    } else arg1 += exec_sz;
                }
                pos += GetCommandSize(cid);
            }
            exec_sz += ofile->ExecSize;
        }

        Write(ofs, exec_sz), Write(ofs, glomem), Write(ofs, str_cnt);
        for (auto &ofile : outfile)
            for (int i = 0; i < ofile->StringCount; i++) 
                Write(ofs, ofile->StringContent[i]);
        if (!is_vlib) Write(ofs, has_addr), Write(ofs, mainaddr);
        for (auto &ofile : outfile)
            ofs.write((char*)ofile->ExecContent, sizeof(vbyte) * ofile->ExecSize);
        return 0;
    }

    int ProjectBuilder_BuildVexe(string &path) {
        int st = vc_node.size(), res = 0;
        
        res = Build_IdenSystem();
        if (res == -1) return -1;
        
        //build the vasm file
        string vasm_path = path.substr(0, path.size() - 5) + ".vasm",
                vo_path = path.substr(0, path.size() - 5) + ".vo";
        Load_PreLabel(vasm_path, st);
        VasmBuilder_Init();
        res = VasmBuilder_Build(vc_node, vasm_path, true);
        if (res == -1) return -1;

        // //build the output file front the vasm_file
        res = Build_OutputFile(vasm_path);
        
        // //load the outfile
        ProjectBuilder_LoadOutFile(vo_path);
        
        ofstream ofs(path, ios::binary);
        write_prelabel(ofs, false);
        WriteVEXEC(ofs, false);

        ofs.close();
        return 0;
    }

    int ProjectBuilder_BuildVlib(string &path) {
        int st = vc_node.size(), res = 0;
        res = Build_IdenSystem();
        if (res == -1) { PrintError("Fail to build Identifier System"); return -1; }

        string vasm_path = path.substr(0, path.size() - 5) + ".vasm",
                vo_path = path.substr(0, path.size() - 5) + ".vo";
        Load_PreLabel(vasm_path, st);
        VasmBuilder_Init();
        res = VasmBuilder_Build(vc_node, vasm_path, true);
        if (res == -1) { PrintError("Fail to build the vasm file"); return -1; }

        //build the output file front the vasm_file
        res = Build_OutputFile(vasm_path);

        //load the outfile
        ProjectBuilder_LoadOutFile(vo_path);
        
        ofstream ofs(path, ios::binary);
        
        write_prelabel(ofs, true);
        WriteVEXEC(ofs, true);

        ofs.close();
        return 0;
    }
}