#pragma once
#include "pretreat.h"

namespace Interpreter {
	enum class CplNodeType : int {
		Empty, Error, Block,
		Identifier, FuncCall, Integer, Float, String, Char,
		Expression,
		Comma, Assign,
		Add, Minus, Mul, Divison, Mod, BitAnd, BitOr, BitXor, BitNot, Lmov, Rmov,
		Equ, Neq, Gt, Ge, Ls, Le,
		LogicAnd, LogicOr, LogicNot,
		CallMember, New, As, Region, ArrIndex,
		VarDefine, FuncDefine, ClassDefine, NamespaceDefine,
		If, Else, Switch, Case, While, For, Continue, Break, Return,
		Using, Super,
		VasmRegion,
		Declaration/*声明*/, Definition
	};
	constexpr CplNodeType 	OperCplNodeTypeStart = CplNodeType::Comma, 
							OperCplNodeTypeEnd = CplNodeType::ArrIndex;

	struct ClassInfo;
	struct FuncInfo;
	struct VarInfo;

	constexpr char IdentifierPathSeparateChar = '@';

	/// <summary>
	/// 表达式类型信息
	/// </summary>
	struct ExpressionType{
		ClassInfo * Type;
		string TypeName;
		int Dimc;
		bool IsQuote, IsMember;
		ExpressionType(string type_name = "", int dimc = 0, bool is_quote = false, bool is_member = false);

		string ToString();
		string ToIdenString();

		bool operator == (ExpressionType &b);
		bool operator != (ExpressionType &b);
	};
	constexpr int TypeVarDimc = 32;


	bool IsInteger(ExpressionType &type);
	bool IsFloat(ExpressionType &type);
	bool IsBasicType(ExpressionType type);
	extern ExpressionType char_etype, int32_etype, int64_etype, uint64_etype, float64_etype, void_etype, object_etype;

	struct CplNode {
		CplNodeType Type;
		ExpressionType ExprType;
		Token Content;
		vector<CplNode *> Children;
		CplNode* Parent;
		int LocalVarCount;
		CplNode(CplNodeType type = CplNodeType::Empty);
		void AddChild(CplNode *child);
		void InsertChild(int index, CplNode *child);
		void ChangeChild(int index, CplNode *child);
		CplNode *&At(int index);
		~CplNode();
	};

	enum class IdenVisibility { Private, Protected, Public };
	enum class IdenRegion { Local, Global, Member };
	enum class IdenType { Variable, Function, Class, Namespace };

	struct IdentifierInfo {
		string Name, FullName;
		ExpressionType ExprType;
		IdenVisibility Visibility;
		IdenRegion Region;
		IdenType Type;

		~IdentifierInfo();
	};

	struct VarInfo : IdentifierInfo {
		ulong Address;
		CplNode* Node;
		VarInfo();
		~VarInfo();
	};
	struct VarFrame {
		map< string, VarInfo* > VarMap;
		VarFrame *Previous;
		int Total;
		void Init(int total = 0);
		int InsertVar(VarInfo* v_info);
		void BuildCleanVasm();
	};
	
	void LocalVarStack_Init();
	VarFrame* LocalVarStack_Top();
	void LocalVarStack_Pop(bool clean = false);
	void LocalVarStack_Push();

	struct FuncInfo : IdentifierInfo {
		vector<VarInfo *> ArgList;
		CplNode *Node;
		FuncInfo();
		~FuncInfo();
	};
	enum class ClassInfoStatus { Uncompleted, Completing, Completed };
	struct ClassInfo : IdentifierInfo {
		map<string, IdentifierInfo *> MemberMap; 

		//父类
		ClassInfo *Parent;
		string ParentName;
		//这个类占用的大小
		ulong Size;
		//所在的深度
		int Deep;

		ClassInfoStatus Status;
		vector<CplNode*> Node;
		vector<ClassInfo *> Children;
		ClassInfo();
		~ClassInfo();

		int InsertMember(IdentifierInfo *iden_info);
		void InheritMember(IdentifierInfo *iden_info);
	};
	extern ClassInfo *char_cls, *int32_cls, *int64_cls, *uint64_cls, *float64_cls, *void_cls, *object_cls;
	bool IsBasicClass(ClassInfo* cls);

	struct NamespaceInfo : IdentifierInfo {
		map<string, IdentifierInfo *> IdentifierMap;
		
		NamespaceInfo();
		~NamespaceInfo();
		int Merge(NamespaceInfo* source);
		int InsertIdentifier(IdentifierInfo *iden_info);
	};
	extern NamespaceInfo *global_nsp;

	extern NamespaceInfo *current_namespace;
	extern ClassInfo *current_class;
	extern FuncInfo *current_function;

	void IdenSystem_Init();
	//建立标识符信息库，确定函数的实际名称
	int IdenSystem_LoadDefinition(vector<CplNode *> &roots);
	//确定表达式的类型，变量的Addr和函数
	int IdenSystem_LoadExpressionType(vector<CplNode *> &roots);
	//将当前所有载入了IdenSystem的定义转化为一个字符串
	string IdenSystem_MakeDefinationSource();
	ulong GetGlobalMemorySize();

	CplNode *CplTree_Build(vector<Token> *token_list);

	void VasmBuilder_WriteCommand(vbyte cmd, ulong arg1 = 0, ulong arg2 = 0);
	void VasmBuilder_WriteCommandStr(vbyte cmd, string arg1, ulong arg2 = 0);

	int VasmBuilder_Init();
	int VasmBuilder_Build(vector<CplNode *> &roots, string vasm_path, bool is_append = false);

	void Debug_PrintClassTree(ClassInfo *cls, int deep = 0);
	void Debug_PrintIdentifierTree(IdentifierInfo *iden, int deep = 0);
#ifdef DEBUG_MODULE
	void debug_print_cpltree(CplNode *nd, int dep);
#endif
}