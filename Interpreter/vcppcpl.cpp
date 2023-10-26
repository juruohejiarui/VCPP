#include "vcppcpl.h"

namespace Interpreter {
    ExpressionType char_etype, int32_etype, int64_etype, uint64_etype, float64_etype, void_etype, object_etype;
    ClassInfo *char_cls, *int32_cls, *int64_cls, *uint64_cls, *float64_cls, *void_cls, *object_cls;

    ExpressionType::ExpressionType(string type_name, int dimc, bool is_quote, bool is_member) {
        this->TypeName = type_name;
        this->Dimc = dimc;
        this->IsQuote = is_quote;
        this->IsMember = is_member;
        this->Type = nullptr;
    }
    string ExpressionType::ToString() {
        string res = "";
        if (Type == nullptr) res = TypeName;
        else res = Type->FullName;
        for (int i = 0; i < Dimc; i++) res.append("[]");
        if (IsQuote) res.append("@");
        return res;
    }
    string ExpressionType::ToIdenString() {
        string res = "";
        if (Type == nullptr) 
            res = "<error-type>";
        else res = Type->FullName;
        res.append("_Dimc");
        res.append(to_string(Dimc));
        return res;
    }
    bool ExpressionType::operator==(ExpressionType &b) { return Type == b.Type && Dimc == b.Dimc; }
    bool ExpressionType::operator!=(ExpressionType &b) { return Type != b.Type || Dimc != b.Dimc; }

    bool IsInteger(ExpressionType &type) { return type == char_etype || type == int32_etype || type == int64_etype || type == uint64_etype; }
    bool IsFloat(ExpressionType &type) { return type == float64_etype; }
    bool IsBasicType(ExpressionType type) {
        if (type.Dimc > (TypeVarDimc >> 1)) type.Dimc = TypeVarDimc - type.Dimc;
        return IsInteger(type) || IsFloat(type); 
    }

    IdentifierInfo::~IdentifierInfo() { }

    VarInfo::VarInfo() {
        Type = IdenType::Variable;
        Node = nullptr;
        Address = 0;
    }
    VarInfo::~VarInfo() { }
    void VarFrame::Init(int total) {
        Previous = nullptr;
        Total = total;
        for (auto pir : VarMap) delete pir.second;
    }
    int VarFrame::InsertVar(VarInfo *v_info) {
        if (VarMap.count(v_info->Name)) return 1;
        VarMap[v_info->Name] = v_info;
        v_info->Address = Total++;
        v_info->Region = IdenRegion::Local;
        return 0;
    }
    void VarFrame::BuildCleanVasm() {
        for (auto pir : VarMap) {
            auto &v_info = pir.second;
            VasmBuilder_WriteCommand((IsBasicType(v_info->ExprType) ? pvar : povar), v_info->Address);
            VasmBuilder_WriteCommand(push, 0);
            VasmBuilder_WriteCommand((IsBasicType(v_info->ExprType) ? vi64mov : vomov));
        }
    }

    VarFrame local_var_stack[256];
    int local_var_stack_size;
    const int local_var_stack_max_size = 256;
    
    void LocalVarStack_Init() {
        local_var_stack_size = 0;
        for (int i = 0; i < local_var_stack_max_size; i++) {
            local_var_stack[i].Init();
            if (i) local_var_stack[i].Previous = &local_var_stack[i - 1];
        }
    }
    VarFrame *LocalVarStack_Top() {
        return (local_var_stack_size ? &local_var_stack[local_var_stack_size - 1] : nullptr);
    }
    void LocalVarStack_Pop(bool clean) {
        if (clean) LocalVarStack_Top()->BuildCleanVasm();
        for (auto pir : LocalVarStack_Top()->VarMap) delete pir.second;
        LocalVarStack_Top()->VarMap.clear();
        local_var_stack_size--;
    }
    void LocalVarStack_Push() {
        local_var_stack_size++;
        LocalVarStack_Top()->Total = (local_var_stack_size > 1 ? LocalVarStack_Top()->Previous->Total : 0);
    }

    FuncInfo::FuncInfo() {
        Type = IdenType::Function;
        Node = nullptr;
    }
    FuncInfo::~FuncInfo() { 
        for (auto arg_info : ArgList) delete arg_info;
    }
    ClassInfo::ClassInfo() {
        Type = IdenType::Class;
        Status = ClassInfoStatus::Uncompleted;
        Parent = nullptr;
        Size = 0;
    }
    ClassInfo::~ClassInfo() {
        for (auto pir : MemberMap) delete pir.second;
    }
    int ClassInfo::InsertMember(IdentifierInfo *iden_info) {
        if (MemberMap.count(iden_info->Name)) return 1;
        iden_info->Region = IdenRegion::Member;
        iden_info->FullName = FullName + "@" + iden_info->Name;
        MemberMap[iden_info->Name] = iden_info;
        return 0;
    }
    void ClassInfo::InheritMember(IdentifierInfo *iden_info) {
        if (MemberMap.count(iden_info->Name) || iden_info->Visibility == IdenVisibility::Private) return ;
        MemberMap[iden_info->Name] = iden_info;
    }
    bool IsBasicClass(ClassInfo *cls) {
        return cls == char_cls || cls == int32_cls || cls == int64_cls || cls == uint64_cls || cls == float64_cls || cls == void_cls || cls == object_cls;
    }
    NamespaceInfo::NamespaceInfo() {
        Type = IdenType::Namespace;
        Visibility = IdenVisibility::Public;
    }
    NamespaceInfo::~NamespaceInfo() { 
        for (auto pir : IdentifierMap) delete pir.second;
    }
    int NamespaceInfo::Merge(NamespaceInfo *source) {
        bool res = false;
        for (auto pir : source->IdentifierMap) res |= InsertIdentifier(pir.second);
        delete source;
        return res;
    }
    int NamespaceInfo::InsertIdentifier(IdentifierInfo *iden_info) {
        if (IdentifierMap.count(iden_info->Name)) {
            if (iden_info->Type == IdenType::Namespace && IdentifierMap[iden_info->Name]->Type == IdenType::Namespace)
                return Merge((NamespaceInfo *)iden_info);
            return 1;
        }
        if (this == global_nsp) iden_info->FullName = iden_info->Name;
        else iden_info->FullName = FullName + "@" + iden_info->Name;
        iden_info->Region = IdenRegion::Global;
        IdentifierMap[iden_info->Name] = iden_info;
        return 0;
    }
    NamespaceInfo *global_nsp;

    NamespaceInfo *current_namespace;
	ClassInfo *current_class;
	FuncInfo *current_function;

    const string    IdenVisibilityString[]  = { "private", "protected", "public" },
                    IdenTypeString[]        = { "var", "func", "class", "namespace" };
    void Debug_PrintIndent(int deep) {
        for (int i = 0; i < deep; i++) printf("  ");
    }
    void Debug_PrintClassTree(ClassInfo *cls, int deep) {
        Debug_PrintIndent(deep);
        PrintLog("class " + cls->Name + " -> " + cls->FullName + "\n");
        for (auto iden : cls->MemberMap) {
            Debug_PrintIndent(deep + 1);
            cout << IdenVisibilityString[(int)iden.second->Visibility] << " " << IdenTypeString[(int)iden.second->Type] << " ";
            cout << iden.first << " -> " << iden.second->FullName << endl;
        }
        for (auto child : cls->Children) Debug_PrintClassTree(child, deep + 1);
    }
    void Debug_PrintIdentifierTree(IdentifierInfo *iden, int deep) {
        switch (iden->Type) {
            case IdenType::Function:
            case IdenType::Variable:
            case IdenType::Class:
                Debug_PrintIndent(deep);
                cout << IdenVisibilityString[(int)iden->Visibility] << " " << IdenTypeString[(int)iden->Type] << " ";
                cout << iden->Name << " -> " << iden->FullName << endl;
                break;
            case IdenType::Namespace:
                Debug_PrintIndent(deep);
                cout << IdenVisibilityString[(int)iden->Visibility] << " " << IdenTypeString[(int)iden->Type] << " ";
                cout << iden->Name << " -> " << iden->FullName << endl;
                for (auto pir : ((NamespaceInfo *)iden)->IdentifierMap)
                    Debug_PrintIdentifierTree(pir.second, deep + 1);
                break;
        }
    }
}