#include "vcppcpl.h"

namespace Interpreter {
    string definition_string;
    inline void AppendDefinition(const string& str) { definition_string.append(str); }

    string MakeidentifierString(string& str) {
        string res = "";
        int pos = str.find('.');
        if (pos == -1) pos = str.size();
        vector<string> prt = StringSplit(str.substr(0, pos), '@');
        for (string p : prt) {
            if (res.size()) res.append("::");
            res.append(p);
        }
        return res;
    }
    string MakeETypeString(ExpressionType& expr) {
        string res = MakeidentifierString(expr.Type->FullName);
        for (int i = 1; i <= expr.Dimc; i++) res.append("[]");
        res.push_back(' ');
        return res;
    }
    string MakeVisibilityString(IdenVisibility visibility) {
        switch (visibility) {
            case IdenVisibility::Public: return "public ";
            case IdenVisibility::Private: return "private ";
            case IdenVisibility::Protected: return "protected ";
        }
        return "[errror]";
    }
    bool IsDefinition(CplNode* node) {
        for (; node->Parent != nullptr; node = node->Parent) ;
        return node->Type == CplNodeType::Definition;
    }
    inline bool IsSpecificMem(CplNode* node, ClassInfo* cls) { return node->Content.Ulong == (ulong)cls; }
    void MakeGlobalFunctionDefinition(FuncInfo* func) {
        if (!IsDefinition(func->Node)) return ;
        AppendDefinition(MakeVisibilityString(func->Visibility));
        AppendDefinition("func "), AppendDefinition(MakeETypeString(func->ExprType));
        string fname;
        {
            int pos = func->Name.find('.');
            if (pos == -1) pos = func->Name.size();
            fname = func->Name.substr(0, pos);
        }
        AppendDefinition(fname + "(");
        CplNode* fnd = func->Node;
        for (int i = 0; i < fnd->Children.size() - 1; i++) {
            if (i) AppendDefinition(", ");
            CplNode* arg = fnd->At(i);
            AppendDefinition(MakeETypeString(arg->ExprType));
            AppendDefinition(arg->Content.String);
        }
        AppendDefinition("){}\n");
    }
    void MakeMemberFunctionDefinition(FuncInfo* func, ClassInfo* cls) {
        if (!IsDefinition(func->Node) || !IsSpecificMem(func->Node, cls)) return ;
        AppendDefinition(MakeVisibilityString(func->Visibility));
        AppendDefinition("func "), AppendDefinition(MakeETypeString(func->ExprType));
        string fname;
        {
            int pos = func->Name.find('.');
            if (pos == -1) pos = func->Name.size();
            fname = func->Name.substr(0, pos);
        }
        AppendDefinition(fname + "(");
        CplNode* fnd = func->Node;
        for (int i = 1; i < fnd->Children.size() - 1; i++) {
            CplNode *arg = fnd->At(i);
            if (i > 1) AppendDefinition(", ");
            AppendDefinition(MakeETypeString(arg->ExprType));
            AppendDefinition(arg->Content.String);
        }
        AppendDefinition("){}\n");
    }
    void MakeClassDefintion(ClassInfo* cls) {
        if (IsBasicClass(cls)) return ;
        AppendDefinition(MakeVisibilityString(cls->Visibility));
        AppendDefinition("class "), AppendDefinition(cls->Name);
        AppendDefinition(" :: ");
        AppendDefinition(MakeidentifierString(cls->Parent->FullName));
        AppendDefinition("{\n");
        for (auto pir : cls->MemberMap) {
            switch (pir.second->Type) {
                case IdenType::Variable:
                    AppendDefinition(MakeVisibilityString(pir.second->Visibility));
                    AppendDefinition("var ");
                    AppendDefinition(MakeETypeString(pir.second->ExprType));
                    AppendDefinition(pir.second->Name);
                    AppendDefinition(";\n");
                    break;
                case IdenType::Function:
                    MakeMemberFunctionDefinition((FuncInfo *)pir.second, cls);
                    break;
            }
        }
        AppendDefinition("}\n");
    }
    void MakeNamespaceDefinition(NamespaceInfo* nsp) {
        if (nsp != global_nsp) {
            AppendDefinition("namespace ");
            AppendDefinition(MakeidentifierString(nsp->FullName));
            AppendDefinition("{\n");
        }

        //不生成全局变量的定义（组要是难搞）
        for (auto pir : nsp->IdentifierMap) {
            switch (pir.second->Type) {
                case IdenType::Class:       MakeClassDefintion((ClassInfo *)pir.second);            break;
                case IdenType::Function:    MakeGlobalFunctionDefinition((FuncInfo *)pir.second);  break;
            }
        }
        if (nsp != global_nsp) AppendDefinition("}\n");
        for (auto pir : nsp->IdentifierMap) if (pir.second->Type == IdenType::Namespace)
            MakeNamespaceDefinition((NamespaceInfo *)pir.second);
    }
    string IdenSystem_MakeDefinationSource()
    {
        definition_string.clear();
        MakeNamespaceDefinition(global_nsp);
        return definition_string;
    }
}