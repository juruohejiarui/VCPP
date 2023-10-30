#include "vcppcpl.h"

namespace Interpreter {
	const char *CodeIdentifierSeparator = "::";
	vector<NamespaceInfo *> using_list;
	ulong global_memory_size;
	ulong GetGlobalMemorySize() { return global_memory_size; }

	inline IdenVisibility GetIdenVisibility(TokenType token_type) { 
		return (IdenVisibility)((int)token_type - (int)TokenType::Private + (int)IdenVisibility::Private); 
	}
	// If a is large than or equal to b
	inline bool LargerVisbility(IdenVisibility a, IdenVisibility b) {
		return (int)a >= (int)b;
	}

	string GetCodeIdentifier(string &iden) {
		string res = "";
		for(int i = 0; i < iden.size(); i++) {
			if (iden[i] == '.') break;
			else if (iden[i] == '@') res.append("::");
			else res.push_back(iden[i]);
		}
		return res;
	}

	void BuildBasicType(ClassInfo *&cls, ExpressionType &etype, string name, ulong size) {
		cls = new ClassInfo();
		cls->Name = name;
		cls->Size = size;
		cls->Visibility = IdenVisibility::Public;
		global_nsp->InsertIdentifier(cls);
		etype = ExpressionType(cls->FullName), etype.Type = cls;
		cls->ExprType = etype;
		cls->ExprType.Dimc = TypeVarDimc;
	}
	void IdenSystem_Init() {
		global_nsp = new NamespaceInfo();
		global_nsp->Region = IdenRegion::Global;
		global_nsp->Name = global_nsp->FullName = "Global";
		global_memory_size = 0;
		BuildBasicType(char_cls, char_etype, "char", 1);
		BuildBasicType(int32_cls, int32_etype, "int", 4);
		BuildBasicType(int64_cls, int64_etype, "long", 8);
		BuildBasicType(uint64_cls, uint64_etype, "ulong", 8);
		BuildBasicType(float64_cls, float64_etype, "double", 8);
		BuildBasicType(void_cls, void_etype, "void", 0);
		BuildBasicType(object_cls, object_etype, "object", 8);
	}

	#pragma region operation of using_list
	void CleanUsingList() { return using_list.clear(); }

	/// @brief Add the using infomation from the using_nd
	/// @param using_nd the cplnode of the using expression
	/// @return if it's successful to add this infomation
	int AddUsing(CplNode *using_nd) {
		auto path = StringSplit(using_nd->Content.String, IdentifierPathSeparateChar);
		auto cur_nsp = global_nsp;

		auto finish_path = global_nsp->Name;
		for (int i = 0; i < path.size(); i++) {
			auto iter = cur_nsp->IdentifierMap.find(path[i]);
			// this identifier doesn't exist or is not a namespace
			if (iter == cur_nsp->IdentifierMap.end() || iter->second->Type != IdenType::Namespace)
				return PrintError("Can't find namespace " + cur_nsp->FullName + IdentifierPathSeparateChar + path[i], using_nd->Content.Line), 1;
			cur_nsp = (NamespaceInfo *)iter->second;
			// update finish_path
		}
		using_list.push_back(cur_nsp);
		return 0;
	}
	#pragma endregion

	#pragma region The identifier searching functions
	/// @brief Find the identifier named NAME
	/// @param name the name of the identfier
	/// @param min_visbility the required visibility in the current namespace
	/// @param line the line id of the code
	/// @return the identifier's pointer
	IdentifierInfo *FindIdentifier(string name, IdenVisibility min_visbility = IdenVisibility::Private, int line = 0) {
		if (name == "Global") return global_nsp;
		// Delete the prefix of the CurrentNamespace->FullName
		if (current_namespace != nullptr && name.size() > current_namespace->FullName.size()
			 && name.substr(0, current_namespace->FullName.size()) == current_namespace->FullName)
			name = name.substr(current_namespace->FullName.size());
		int dot_p = name.find('.');
		if (dot_p == -1) dot_p = name.size();
		string tmp = name.substr(0, dot_p);
		auto path = StringSplit(tmp, IdentifierPathSeparateChar);
		path[path.size() - 1].append(name.substr(dot_p));
		if (path.size() == 1) {
			// Check if it is a local variable
			for (auto frm = LocalVarStack_Top(); frm != nullptr; frm = frm->Previous) {
				auto iter = frm->VarMap.find(path[0]);
				if (iter != frm->VarMap.end()) return iter->second;
			}
			// Check if it is a member of CurrentClass
			if (current_class != nullptr) {
				auto iter = current_class->MemberMap.find(path[0]);
				if (iter != current_class->MemberMap.end()) return iter->second;
			}
			
		}
		// Check if it is a identifier inside CurrentNamespace
		IdentifierInfo *nsp = global_nsp;
		int st = 0;
		if (current_namespace != nullptr) {
			auto iter = current_namespace->IdentifierMap.find(path[0]);
			if (iter != current_namespace->IdentifierMap.end() && LargerVisbility(iter->second->Visibility, min_visbility)) {
				nsp = iter->second;
				st = 1;
			}
		}
		for (auto *using_nsp : using_list) {
			auto iter = using_nsp->IdentifierMap.find(path[0]);
			if (iter != using_nsp->IdentifierMap.end() && iter->second->Visibility == IdenVisibility::Public) {
				nsp = iter->second;
				st = 1;
				break;
			}
		}
		// search the identifier along the path
		for (int i = st; i < path.size(); i++) {
			auto iter = ((NamespaceInfo *)nsp)->IdentifierMap.find(path[i]);
			if (iter == ((NamespaceInfo *)nsp)->IdentifierMap.end() || iter->second->Visibility != IdenVisibility::Public) 
				return PrintError("Can't find identifier " + nsp->FullName + IdentifierPathSeparateChar + path[i], line), nullptr;
			nsp = iter->second;
		}
		return nsp;
	}
	#pragma endregion
	/// @brief Create a namespace pointer using the name
	/// @param name the name of the namespace
	/// @param line the line id of the source
	/// @return if it's successful to create this namespace pointer
	NamespaceInfo* BuildNamespaceIdentifier(const string& name, int line = 0) {
		if (name == "Global") return global_nsp;
		auto path = StringSplit(name, IdentifierPathSeparateChar);
		NamespaceInfo *res = global_nsp;
		for (int i = 0; i < path.size(); i++) {
			auto iter = res->IdentifierMap.find(path[i]);
			if (iter == res->IdentifierMap.end()) {
				//create a new namespace pointer
				NamespaceInfo *nsp = new NamespaceInfo();
				nsp->Name = path[i];
				res->InsertIdentifier(nsp);
				res = nsp;
			} else if (iter->second->Type != IdenType::Namespace) 
				// This identifier exists and is not a namespace
				return PrintError(res->FullName + "@" + path[i] + " is not a namespace", line), nullptr;
			else res = (NamespaceInfo *)iter->second;
		}
		return res;
	}
	
	/// @brief Create the identifier of class from ND into NSP
	/// @param nsp the namespace the class belongs to
	/// @param nd the cplnode of the class
	/// @return if it is successful to do this
	int BuildClassIdentifier(NamespaceInfo *nsp, CplNode *nd) {
		// set the environment
		current_namespace = nsp;
		current_class = nullptr;
		current_function = nullptr;
		auto &name = nd->Content.String;
		auto visibility = GetIdenVisibility(nd->Content.Type);
		auto iter = nsp->IdentifierMap.find(name);
		if (iter != nsp->IdentifierMap.end()) {
			if (iter->second->Type != IdenType::Class || iter->second->Visibility != visibility)
				return PrintError("Can't create the multiple-defined class : " + name, nd->Content.Line), 1;
			// It is valid to create this multiple-defined class
			((ClassInfo *)iter->second)->Node.push_back(nd);
			return 0;
		}
		ClassInfo *cls = new ClassInfo();
		cls->Name = name;
		cls->Visibility = visibility;
		cls->Node.push_back(nd);

		nsp->InsertIdentifier(cls);
		cls->ExprType = ExpressionType(cls->FullName), cls->ExprType.Type = cls;
		return 0;
	}

	int CompleteEType(ExpressionType &etype, IdenVisibility min_visibility = IdenVisibility::Private, int line = 0) {
		auto iter = FindIdentifier(etype.TypeName);
		if (iter == nullptr || iter->Type != IdenType::Class || !LargerVisbility(iter->Visibility, min_visibility)) 
			return PrintError("Identifier " + etype.TypeName + " is not a class identifier.", line), 1;
		etype.Type = (ClassInfo *)iter;
		return 0;
	}
	/// @brief get the arguments' identifier string
	/// @param nd the cplnode of the function definition 
	/// @param min_visibility the required visibility
	/// @param name the result
	/// @return if it is successful
	int GetArgumentIdentifier(CplNode *nd, IdenVisibility min_visibility, string& name) {
		int res = 0;
		for (int i = 0; i < nd->Children.size() - 1; i++) {
			res |= CompleteEType(nd->At(i)->ExprType, min_visibility);
			name.push_back('.');
			name.append(nd->At(i)->ExprType.ToIdenString());
		}
		return res;
	}

	/// @brief build the ArgList of f_info
	/// @param nd the cplnode of the function definition
	/// @param f_info 
	/// @return if it is successful
	int GetArgumentList(CplNode *nd, FuncInfo *f_info) {
		set<string> arg_name_set;
		int res = 0;
		for (int i = 0; i < nd->Children.size() - 1; i++) {
			if (arg_name_set.count(nd->At(i)->Content.String)) {
				PrintError("multiple definition of argument " + nd->At(i)->Content.String, nd->At(i)->Content.Line);
				res = 1;
			}
			VarInfo *arg_info = new VarInfo();
			arg_info->Region = IdenRegion::Local;
			arg_info->Name = arg_info->FullName = nd->At(i)->Content.String;
			arg_info->Node = nd->At(i);
			arg_info->ExprType = nd->At(i)->ExprType;
			f_info->ArgList.push_back(arg_info);
		}
		return res;
	}

	/// @brief Create the identifier of the global function from ND into NSP
	/// @param nsp the namespace the class belongs to
	/// @param nd the cplnode of the class
	/// @return if it is successful to do this
	int BuildGlobalFunction(NamespaceInfo *nsp, CplNode *nd) {
		// set the environment
		current_namespace = nsp;
		current_class = nullptr;
		current_function = nullptr;
		// we need to change the name of the function
		auto &name = nd->Content.String;
		auto visibility = GetIdenVisibility(nd->Content.Type);
		// get the result etype
		int res = CompleteEType(nd->ExprType, visibility);
		// get the arguments' identifier
		res |= GetArgumentIdentifier(nd, visibility, name);
		//check if it is multiple definition
		auto iter = nsp->IdentifierMap.find(name);
		if (iter != nsp->IdentifierMap.end()) 
			return PrintError("Multiple definition of identifier" + nsp->FullName + "@" + name, nd->Content.Line), 1;

		FuncInfo *f_info = new FuncInfo();
		f_info->Name = name;
		f_info->Visibility = visibility;
		f_info->Node = nd;
		f_info->ExprType = nd->ExprType;
		// build the argument list
		res |= GetArgumentList(nd, f_info);
		nsp->InsertIdentifier(f_info);
		nd->Content.String = f_info->FullName;
		return res;
	}
	int BuildGlobalVariable(NamespaceInfo *nsp, CplNode *nd) {
		// set the environment
		current_namespace = nsp;
		current_class = nullptr;
		current_function = nullptr;

		auto visibility = GetIdenVisibility(nd->Content.Type);
		int res = CompleteEType(nd->ExprType);
		for (int i = 0; i < nd->Children.size(); i += 2) {
			string &name = nd->At(i)->Content.String;
			auto iter = nsp->IdentifierMap.find(name);
			if (iter != nsp->IdentifierMap.end()) {
				PrintError("Multiple definition of identifier : " + nsp->FullName + "@" + name, nd->Content.Line);
				res = 1;
				continue;
			}
			VarInfo *v_info = new VarInfo();
			v_info->Visibility = visibility;
			v_info->Name = name;
			v_info->ExprType = nd->ExprType;
			v_info->Address = global_memory_size;
			nsp->InsertIdentifier(v_info);
			// update the global_memory_size
			global_memory_size += sizeof(ulong);
			//ignore the initialization expression
		}
		return res;
	}

	/// @brief build the initialization function of CLS using the cplnode ND
	/// @param nsp the namespace 
	/// @param cls the class
	/// @param nd the cplnode
	/// @return if it is successful
	int BuildInitFunction(NamespaceInfo *nsp, ClassInfo *cls, CplNode *nd) {
		// set the enviroment
		current_namespace = nsp;
		current_class = cls;
		current_function = nullptr;
		// the function name is __init__(cls->FullName, arglist...)
		// this operation will modify the Content.String of the CplNode
		string name = nd->Content.String;
		// Change the argument list
		auto arg_this = new CplNode(CplNodeType::Identifier);
		arg_this->ExprType = ExpressionType(cls->FullName, 0, true, 0);
		arg_this->Content.String = "this";
		nd->InsertChild(0, arg_this);
		nd->LocalVarCount++;
		// the visbility of __init__ function is the same as the class
		auto visibility = cls->Visibility;
		auto res = GetArgumentIdentifier(nd, visibility, name);
		// modify the string in the cplnode
		// insert this function into the global_nsp after check if it is multiple definition
		auto iter = global_nsp->IdentifierMap.find(name);
		if (iter != global_nsp->IdentifierMap.end()) {
			PrintError("Multiple definition of identifier " + name, nd->Content.Line);
			res = 1;
		}
		FuncInfo *f_info = new FuncInfo;
		f_info->Visibility = IdenVisibility::Public;
		f_info->Name = name;
		f_info->ExprType = void_etype;
		f_info->Node = nd;
		res |= GetArgumentList(nd, f_info);

		global_nsp->InsertIdentifier(f_info);
		nd->Content.String = f_info->FullName;
		nd->Content.Type = TokenType::Public;
		
		// change the super expression into __init__(this => parent_type, arglist...)
		CplNode *blk = nd->At(nd->Children.size() - 1);
		if (blk->Children.size() > 0 && blk->At(0)->Type == CplNodeType::Super) {
			// It is valid to use the priavte InitFunction of parent
			CplNode *sup = blk->At(0);
			CplNode *arg_expr = new CplNode(CplNodeType::Expression),
					*expr_this = new CplNode(CplNodeType::Identifier),
					*expr_as = new CplNode(CplNodeType::As),
					*expr_par = new CplNode(CplNodeType::Identifier),
					*rt_expr = new CplNode(CplNodeType::Expression);
			expr_this->Content.String = "this";
			expr_par->Content.String = cls->Parent->FullName;
			sup->InsertChild(0, arg_expr);
			arg_expr->AddChild(expr_as);
			expr_as->AddChild(expr_this);
			expr_as->AddChild(expr_par);
			sup->Type = CplNodeType::FuncCall;
			sup->Content.String = "__init__";
			rt_expr->AddChild(sup);
			blk->At(0) = rt_expr;
		}
		return res;
	}

	int BuildMemberFunction(NamespaceInfo *nsp, ClassInfo *cls, CplNode *nd) {
		// set the enviroment
		current_namespace = nsp;
		current_class = cls;
		current_function = nullptr;
		// the function name is cls->FullName@Function_RealName
		// this operation will modify the Content.String of the CplNode
		auto &name = nd->Content.String;
		auto visibility = GetIdenVisibility(nd->Content.Type);
		// complete the result etype and argument identifier
		auto res = CompleteEType(nd->ExprType, cls->Visibility);
		res |= GetArgumentIdentifier(nd, cls->Visibility, name);
		// use the Content.Ulong as the pointer of cls
		nd->Content.Ulong = (ulong)cls;
		
		//check if it is multiple definition
		auto iter = cls->MemberMap.find(name);
		if (iter != cls->MemberMap.end()) {
			PrintError("Multiple definition of identifier " + cls->FullName + "@" + name, nd->Content.Line);
			res = 1;
		}
		// change the argument list
		CplNode *arg_this = new CplNode(CplNodeType::Identifier);
		arg_this->Content.String = "this";
		arg_this->ExprType = ExpressionType(cls->FullName, 0, true, false);
		arg_this->ExprType.Type = cls;
		nd->InsertChild(0, arg_this);
		nd->LocalVarCount++;

		FuncInfo *f_info = new FuncInfo();
		f_info->Name = name;
		f_info->Visibility = visibility;
		f_info->ExprType = nd->ExprType;
		f_info->Node = nd;
		res = GetArgumentList(nd, f_info);

		cls->InsertMember(f_info);
		nd->Content.String = f_info->FullName;
		return res;
	}

	/// @brief Build the content of CLS using ND
	/// @param nsp the namespace that CLS belongs to
	/// @param cls the class
	/// @param nd the cplnode of the content
	/// @return if it is successful
	int BuildClassContent(NamespaceInfo *nsp, ClassInfo *cls, CplNode *nd) {
		// set the enviroment
		current_namespace = nsp;
		current_class = cls;
		current_function = nullptr;

		auto res = 0;
		// find the parent
		if (cls->Parent == nullptr) {
			auto par_iden = FindIdentifier(nd->At(0)->Content.String);
			if (par_iden == nullptr || par_iden->Type != IdenType::Class) {
				PrintError("The identifier " + nd->At(0)->Content.String + " is not class", nd->Content.Line);
				res = 1;
			} else {
				cls->Parent = (ClassInfo *) par_iden;
				((ClassInfo *)par_iden)->Children.push_back(cls);
			}
		}

		for (int i = 1; i < nd->Children.size(); i++) {
			switch (nd->At(i)->Type) {
				case CplNodeType::FuncDefine:
					if (nd->At(i)->Content.String == "__init__")
						res |= BuildInitFunction(nsp, cls, nd->At(i));
					else res |= BuildMemberFunction(nsp, cls, nd->At(i));
					break;
				// variables
				case CplNodeType::VarDefine: 
					{
						auto var_def = nd->At(i);
						auto visibility = GetIdenVisibility(var_def->Content.Type);
						//complete the etype of the variables
						res |= CompleteEType(var_def->ExprType);
						for (int i = 0; i < var_def->Children.size(); i += 2) {
							auto iden_nd = var_def->At(i);
							// check if it is multiple definition
							auto iter = cls->MemberMap.find(iden_nd->Content.String);
							if (iter != cls->MemberMap.end()) {
								PrintError("Multiple definition of identifier " + cls->FullName + "@" + iden_nd->Content.String, 
											nd->Content.Line);
								res = 1;
								continue;
							}
							// create the pointer of the variable
							VarInfo *v_info = new VarInfo();
							v_info->Name = iden_nd->Content.String;
							v_info->Visibility = visibility;
							v_info->ExprType = var_def->ExprType;
							v_info->Node = iden_nd;
							// use the Content.Ulong as the pointer of cls
							iden_nd->Content.Ulong = (ulong)cls;
							
							cls->InsertMember(v_info);
						}
					}
					break;
			}
		}
		return res;
	}

	/// @brief Build the content of NSP using ND
	/// @param nsp the namespace
	/// @param nd the cplnode of the content
	/// @return if it is successful
	int BuildNamespaceContent(NamespaceInfo *nsp, CplNode *nd) {
		// set the environment
		current_namespace = nsp;
		current_class = nullptr;
		current_function = nullptr;

		int res = 0;
		for (auto child : nd->Children) {
			switch (child->Type) {
				case CplNodeType::VarDefine:
					res |= BuildGlobalVariable(nsp, child);
					break;
				case CplNodeType::FuncDefine:
					res |= BuildGlobalFunction(nsp, child);
					break;
				case CplNodeType::ClassDefine:
					{
						auto iter = nsp->IdentifierMap.find(child->Content.String);
						if (iter == nsp->IdentifierMap.end() || iter->second->Type != IdenType::Class) res = 1;
						else res |= BuildClassContent(nsp, (ClassInfo *)iter->second, child);
					}
					break;
			}
		}
		return res;
	}

	int BuildClassTree(ClassInfo *cls) {
		if (cls->Status == ClassInfoStatus::Completed) return 0;
		cls->Status = ClassInfoStatus::Completed;
		int res = 0;
		if (cls != object_cls) {
			ClassInfo *par = cls->Parent;
			cls->Size = par->Size;
			// calculate the address(offset) of the cls's variable 
			for (auto iter = cls->MemberMap.begin(); iter != cls->MemberMap.end(); iter++) 
				if (iter->second->Type == IdenType::Variable) {
					((VarInfo*)iter->second)->Address = cls->Size;
					cls->Size += sizeof(ulong);
				}
			for (auto iter = par->MemberMap.begin(); iter != par->MemberMap.end(); iter++)
				cls->InheritMember(iter->second);
		}
		for (auto child : cls->Children) res |= BuildClassTree(child);
		return res;
	}

	/// @brief build the identifier of namespace and class
	/// @param nd the cplnode of the content
	/// @return if it is successful
	int BuildIdentifier(CplNode *nd) {
		int res = 0;
		NamespaceInfo *nsp = BuildNamespaceIdentifier(nd->Content.String, nd->Content.Line);
		for (auto child : nd->Children) if (child->Type == CplNodeType::ClassDefine)
			res |= BuildClassIdentifier(nsp, child);
		return res;
	}

	int IdenSystem_LoadDefinition(vector<CplNode*> &roots) {
		int res = 0;
		for (CplNode *root : roots) {
			for (auto *child : root->Children) if (child->Type == CplNodeType::NamespaceDefine)
				res |= BuildIdentifier(child);
		}

		for (CplNode *root : roots) {
			CleanUsingList();
			for (auto child : root->Children) if (child->Type == CplNodeType::Using)
				res |= AddUsing(child);
			for (auto child : root->Children) if (child->Type == CplNodeType::NamespaceDefine)
				res |= BuildNamespaceContent(BuildNamespaceIdentifier(child->Content.String), child);
		}
		res |= BuildClassTree(object_cls);
#ifdef DEBUG_MODULE
		printf("---------------------------------------------------------------\n");
		Debug_PrintClassTree(object_cls);
		printf("---------------------------------------------------------------\n");
		Debug_PrintIdentifierTree(global_nsp);
#endif
		return res;
	}

	int BuildExpressionEType(CplNode *&node);

	ExpressionType GetOperResultType(ExpressionType& a, ExpressionType& b) {
		if (a == float64_etype || b == float64_etype) return float64_etype;
		else if (a == uint64_etype || b == uint64_etype) return uint64_etype;
		else if (a == int64_etype || b == int64_etype) return int64_etype;
		else if (a == int32_etype || b == int32_etype) return int32_etype;
		return char_etype;
	}

	/// @brief Build the etype of the argument list and the arg_suffix
	/// @param node the FuncCall Node
	/// @param arg_suffix the suffix of the function call
	/// @return if it is successful
	int BuildArgumentEtype(CplNode *node, string &arg_suffix) {
		int res = 0;
		for (auto &arg : node->Children) {
			res |= BuildExpressionEType(arg);
			arg_suffix.push_back('.');
			arg_suffix.append(arg->ExprType.ToIdenString());
		}
		return res;
	}
	

	int BuildExpressionEType(CplNode *&node) {
		int res = 0;
		switch (node->Type) {
			#pragma region constant
			case CplNodeType::Empty: 	node->ExprType = void_etype; 	break;
			case CplNodeType::Integer: 	node->ExprType = int32_etype;	break;
			case CplNodeType::Float: 	node->ExprType = float64_etype; break;
			case CplNodeType::Char: 	node->ExprType = char_etype;	break;
			case CplNodeType::String:
				node->ExprType = char_etype;
				node->ExprType.Dimc = 1;
				node->ExprType.IsQuote = true;
				break;
			#pragma endregion
			#pragma region Arithmetic operations
			case CplNodeType::Comma:
				res = BuildExpressionEType(node->At(0)) | BuildExpressionEType(node->At(1));
				node->ExprType = node->At(1)->ExprType;
				break;
			case CplNodeType::Assign:
				res = BuildExpressionEType(node->At(0)) | BuildExpressionEType(node->At(1));
				if (!node->At(0)->ExprType.IsQuote || node->At(0)->ExprType != node->At(1)->ExprType) {
					PrintError("Invalid operand of \"=\"", node->Content.Line);
					node->Type = CplNodeType::Error, res = 1;
				}
				node->ExprType = void_etype;
				break;
			case CplNodeType::Add:
			case CplNodeType::Minus:
			case CplNodeType::Mul:
			case CplNodeType::Divison:
				res |= BuildExpressionEType(node->At(0)) | BuildExpressionEType(node->At(1));
				if (!IsBasicType(node->At(0)->ExprType) || !IsBasicType(node->At(1)->ExprType))
					PrintError("The operand of + - * / must be number.", node->Content.Line),
					node->Type = CplNodeType::Error, res = 1;
				else node->ExprType = GetOperResultType(node->At(0)->ExprType, node->At(1)->ExprType);
				break;
			case CplNodeType::Mod:
			case CplNodeType::BitAnd:
			case CplNodeType::BitOr:
			case CplNodeType::BitXor:
			case CplNodeType::Lmov:
			case CplNodeType::Rmov:
				res |= BuildExpressionEType(node->At(0)) | BuildExpressionEType(node->At(1));
				if (!IsInteger(node->At(0)->ExprType) || !IsInteger(node->At(1)->ExprType))
					PrintError("The operand of % & | ^ << >> must be integer.", node->Content.Line),
					node->Type = CplNodeType::Error, res = 1;
				else node->ExprType = GetOperResultType(node->At(0)->ExprType, node->At(1)->ExprType);
				break;
			case CplNodeType::Le:
			case CplNodeType::Ls:
			case CplNodeType::Ge:
			case CplNodeType::Gt:
			case CplNodeType::Equ:
			case CplNodeType::Neq:
				res |= BuildExpressionEType(node->At(0)) | BuildExpressionEType(node->At(1));
				if (!IsBasicType(node->At(0)->ExprType) || !IsBasicType(node->At(1)->ExprType))
					PrintError("The operand of < <= > >= == != must be number.", node->Content.Line),
					node->Type = CplNodeType::Error, res = 1;
				else node->ExprType = int32_etype;
				break;
			case CplNodeType::LogicAnd:
			case CplNodeType::LogicOr:
				res |= BuildExpressionEType(node->At(0)) | BuildExpressionEType(node->At(1));
				node->ExprType = int32_etype;
				break;
			case CplNodeType::BitNot:
				res |= BuildExpressionEType(node->At(0));
				if (!IsInteger(node->At(0)->ExprType))
					PrintError("The operand of ~ must be integer.", node->Content.Line),
					node->Type = CplNodeType::Error, res = 1;
				else
					node->ExprType = node->At(0)->ExprType,
					node->ExprType.IsMember = node->ExprType.IsQuote = false;
				break;
			case CplNodeType::LogicNot:
				res |= BuildExpressionEType(node->At(0));
				if (!IsInteger(node->At(0)->ExprType))
					PrintError("The operand of ~ must be integer.", node->Content.Line),
					node->Type = CplNodeType::Error, res = 1;
				else node->ExprType = int32_etype;
				break;
			#pragma endregion
			#pragma region inc and dec
			case CplNodeType::PInc:
			case CplNodeType::PDec:
			case CplNodeType::SInc:
			case CplNodeType::SDec:
				res = BuildExpressionEType(node->At(0));
				if (!IsInteger(node->At(0)->ExprType) || !node->At(0)->ExprType.IsQuote) {
					PrintError("The operand of ++ and -- must be Integer and a reference.", node->Content.Line);
					node->Type = CplNodeType::Error, res = 1;
				}
				node->ExprType = node->At(0)->ExprType;
				node->ExprType.IsQuote = node->ExprType.IsMember = false;
				break;
			#pragma endregion
			case CplNodeType::As:
				res |= BuildExpressionEType(node->At(0)) | BuildExpressionEType(node->At(1));
				if (node->At(1)->ExprType.Dimc <= (TypeVarDimc >> 1))
					PrintError("The right operand of => must a type identifier.", node->Content.Line),
					node->Type = CplNodeType::Error, res = 1;
				node->ExprType = node->At(1)->ExprType;
				node->ExprType.Dimc = TypeVarDimc - node->ExprType.Dimc;
				node->ExprType.IsMember = node->At(0)->ExprType.IsMember;
				node->ExprType.IsQuote = node->At(0)->ExprType.IsQuote;
				if (IsBasicType(node->At(0)->ExprType) != IsBasicType(node->At(1)->ExprType))
					node->ExprType.IsMember = node->ExprType.IsQuote = false;
				break;
			case CplNodeType::New:
				// use the InitFunction
				if (node->At(0)->Type == CplNodeType::FuncCall) {
					CplNode *fcall_nd = node->At(0);
					//Find the identifier of the function
					auto iden = FindIdentifier(fcall_nd->Content.String);
					if (iden == nullptr || iden->Type != IdenType::Class) {
						PrintError("The operand of $ must be a class or its initialization function.", node->Content.Line);
						node->Type = CplNodeType::Error, res = 1;
					} else 
						node->ExprType = ExpressionType(iden->FullName), node->ExprType.Type = (ClassInfo *)iden;
					auto name = "__init__." + (iden != nullptr ? iden->FullName : "<error-type>") + "_Dimc0";
					res |= BuildArgumentEtype(fcall_nd, name);
					if (!global_nsp->IdentifierMap.count(name))
						PrintError("Can't find identifier " + name, fcall_nd->Content.Line),
						res = 1;
					node->Content.String = name;
				} else {
					res = BuildExpressionEType(node->At(0));
					node->ExprType = node->At(0)->ExprType;
					node->ExprType.Dimc = TypeVarDimc - node->ExprType.Dimc;
					node->ExprType.IsMember = node->ExprType.IsQuote = false;
				}
				break;
			case CplNodeType::ArrIndex:
				res |= BuildExpressionEType(node->At(0)) | BuildExpressionEType(node->At(1));
				if (node->At(1)->Type == CplNodeType::Empty || IsInteger(node->At(1)->ExprType))
					node->ExprType = node->At(0)->ExprType, node->ExprType.Dimc--, node->ExprType.IsMember = true;
				else {
					PrintError("The right operand of [] must be an integer.", node->Content.Line);
					node->Type = CplNodeType::Error, res = 1;
				}
				break;
			case CplNodeType::CallMember:
				res = BuildExpressionEType(node->At(0));
				if (!res && !node->At(0)->ExprType.Dimc) {
					auto cls = node->At(0)->ExprType.Type;
					bool is_this = (node->At(0)->Type == CplNodeType::Identifier && node->At(0)->Content.String == "this");
					string &name = node->At(1)->Content.String;
					if (node->At(1)->Type == CplNodeType::FuncCall) res = BuildArgumentEtype(node->At(1), name);
					auto pir = cls->MemberMap.find(name);
					// check the visibility
					if (pir == cls->MemberMap.end() && (is_this || pir->second->Visibility == IdenVisibility::Public)) {
						PrintError("Can't find member of class " + name);
						node->Type = CplNodeType::Error, res = 1;
					} else {
						// store the infomation of member in the operator
						if (pir->second->Type == IdenType::Function) node->Content.String = pir->second->FullName;
						else node->Content.Ulong = ((VarInfo *)pir->second)->Address, node->ExprType.IsMember = true;
					}
					node->ExprType = pir->second->ExprType;
					break;
				} else {
					PrintError("Invalid right operand of \".\"", node->At(0)->Content.Line);
					node->Type = CplNodeType::Error; res = 1;
				}
				break;
			case CplNodeType::Identifier:
				{
					auto iden = FindIdentifier(node->Content.String);
					// it is invalid to use namespace and function
					if (iden == nullptr || iden->Type == IdenType::Function || iden->Type == IdenType::Namespace)
						PrintError("Invalid type of identifier", node->Content.Line),
						node->Type = CplNodeType::Error, res = 1;
					// it is a variable
					if (iden->Type == IdenType::Variable) {
						node->ExprType = iden->ExprType;
						// if it is a member variable -> add "this."
						if (iden->Region == IdenRegion::Member) {
							node->Type = CplNodeType::CallMember;
							CplNode *iden_this = new CplNode(CplNodeType::Identifier),
									*iden_mem = new CplNode(CplNodeType::Identifier);
							iden_this->Content.String = "this";
							iden_this->ExprType = FindIdentifier("this")->ExprType;
							iden_mem->Content.String = node->Content.String;
							node->AddChild(iden_this), node->AddChild(iden_mem);
						} else if (iden->Region == IdenRegion::Global) node->Content.Char = 1;
						node->Content.Ulong = ((VarInfo *)iden)->Address;
					} else // it is a class
						node->ExprType = ExpressionType(iden->FullName, TypeVarDimc), 
						node->ExprType.Type = (ClassInfo *)iden;
				}
				break;
			case CplNodeType::FuncCall:
				{
					auto &name = node->Content.String;
					res = BuildArgumentEtype(node, name);
					auto iden = FindIdentifier(name);
					if (iden == nullptr || iden->Type != IdenType::Function) {
						PrintError("Identifier " + name + " is not a function", node->Content.Line);
						node->Type = CplNodeType::Error, res = 1;
					} else {
						// if it is a member function -> add "this."
						if (iden->Region == IdenRegion::Member) {
							CplNode *expr_cm = new CplNode(CplNodeType::CallMember),
									*iden_this = new CplNode(CplNodeType::Identifier);
							iden_this->Content.String = "this";
							iden_this->ExprType = FindIdentifier("this")->ExprType;
							expr_cm->AddChild(iden_this), expr_cm->AddChild(node);
							node = expr_cm;
							node->Content.String = iden->FullName;
						} else name = iden->FullName;
						node->ExprType = iden->ExprType;
					}
				}
				break;
			case CplNodeType::Expression: res = BuildExpressionEType(node->At(0)), node->ExprType = node->At(0)->ExprType; break;
		}
		return res;
	}

	int BuildNodeEType(CplNode *&node);

	int BuildIfEType(CplNode *node) {
		int res = BuildExpressionEType(node->At(0));
		if (!IsBasicType(node->At(0)->ExprType)) {
			PrintError("The condition expression must be number.", node->At(0)->Content.Line);
			res = 1, node->Type = CplNodeType::Error;
		}
		res |= BuildNodeEType(node->At(1)) | (node->Children.size() > 2 ? BuildNodeEType(node->At(2)) : 0);
		return res;
	}
	int BuildWhileEType(CplNode *node) {
		int res = BuildExpressionEType(node->At(0));
		if (!IsBasicType(node->At(0)->ExprType)) {
			PrintError("The condition expression must be number.", node->At(0)->Content.Line);
			res = 1, node->Type = CplNodeType::Error;
		}
		res |= BuildNodeEType(node->At(1));
		return res;
	}
	int BuildForEType(CplNode *node) {
		LocalVarStack_Push();
		int res = BuildNodeEType(node->At(0));
		res |= BuildExpressionEType(node->At(1));
		if (node->At(1) != nullptr && !IsBasicType(node->At(1)->ExprType)) {
			PrintError("The condition expression must be number.", node->At(0)->Content.Line);
			res = 1, node->Type = CplNodeType::Error;
		}
		res |= BuildExpressionEType(node->At(2)) | BuildNodeEType(node->At(3));
		LocalVarStack_Pop();
		return 0;
	}

	int BuildReturnEType(CplNode *node) {
		ExpressionType ret = void_etype;
		int res = 0;
		if (node->Children.size()) res |= BuildExpressionEType(node->At(0)), ret = node->At(0)->ExprType;
		if (ret != current_function->ExprType) {
			PrintError("The expression type of return is not match the expression type of function " + current_function->FullName, 
						node->Content.Line);
			res = 1, node->Type = CplNodeType::Error;
		}
		return res;
	}

	int BuildVariableEType(CplNode *node, ExpressionType &etype) {
		auto top = LocalVarStack_Top();
		int res = 0;
		if (top->VarMap.count(node->Content.String)) {
			PrintError("Multiple definition of identifier : " + node->Content.String, node->Content.Line);
			node->Type = CplNodeType::Error; res = 1;
		}
		VarInfo *v_info = new VarInfo();
		v_info->Name = node->Content.String;
		v_info->ExprType = etype;
		top->InsertVar(v_info);
		return res;
	}

	int BuildVarDefEType(CplNode *node) {
		int res = CompleteEType(node->ExprType, IdenVisibility::Private, node->Content.Line);
		node->ExprType.IsQuote = true;
		for (int i = 0; i < node->Children.size(); i += 2)
			res |= BuildVariableEType(node->At(i), node->ExprType) | BuildExpressionEType(node->At(i + 1));
		return res;
	}

	int BuildBlockEType(CplNode *node) {
		LocalVarStack_Push();
		int res = 0;
		for (CplNode *child : node->Children) res |= BuildNodeEType(child);
		LocalVarStack_Pop();
		return res;
	}

	int BuildNodeEType(CplNode *&node) {
		int res = 0;
		switch (node->Type) {
			case CplNodeType::Block: 		res = BuildBlockEType(node); break;
			case CplNodeType::Expression: 	res = BuildExpressionEType(node); break;
			case CplNodeType::VarDefine: 	res = BuildVarDefEType(node); break;
			case CplNodeType::If: 			res = BuildIfEType(node); break;
			case CplNodeType::While: 		res = BuildWhileEType(node); break;
			case CplNodeType::For: 			res = BuildForEType(node); break;
			case CplNodeType::Return: 		res = BuildReturnEType(node); break;
		}
		return res;
	}

	int BuildFunctionEType(CplNode *node, FuncInfo *func) {
		current_function = func;
		int res = 0;
		LocalVarStack_Push();
		for (int i = 0; i < node->Children.size() - 1; i++)
			res |= BuildVariableEType(node->At(i), node->At(i)->ExprType);
		res |= BuildNodeEType(node->At(node->Children.size() - 1));
		current_function = nullptr;
		LocalVarStack_Pop();
		return res;
	}

	int BuildClassEType(CplNode *node, ClassInfo *cls) {
		current_class = cls;
		int res = 0;
		for (auto child : node->Children) if (child->Type == CplNodeType::FuncDefine) {
			IdentifierInfo* iden;
			if (child->Content.String.size() >= 8 && child->Content.String.substr(0, 8) == "__init__")
				iden = global_nsp->IdentifierMap.find(child->Content.String)->second;
			else 
				iden = cls->MemberMap.find(child->Content.String.substr(cls->FullName.size() + 1))->second;
			res |= BuildFunctionEType(child, (FuncInfo *)iden);
		}
		current_class = nullptr;
		return res;
	}

	int BuildNamespaceEType(CplNode *node) {
		int res = 0;
		current_namespace = (NamespaceInfo *)FindIdentifier(node->Content.String);
		for (auto child : node->Children) {
			switch(child->Type) {
				case CplNodeType::FuncDefine:
					res |= BuildFunctionEType(child, (FuncInfo *)FindIdentifier(child->Content.String));
					break;
				case CplNodeType::ClassDefine:
					res |= BuildClassEType(child, (ClassInfo *)FindIdentifier(child->Content.String));
					break;
			}
		}
		current_namespace = nullptr;
		return res;
	}

	int IdenSystem_LoadExpressionType(vector<CplNode *> &roots) {
		LocalVarStack_Init();

		int res = 0;
		for (CplNode *root : roots) if (root->Type == CplNodeType::Definition) {
#ifdef DEBUG_MODULE
			for (int i = 0; i < 96; i++) putchar('-');
			putchar('\n');
			// debug_print_cpltree(root, 0);
#endif
			CleanUsingList();
			for (CplNode* und : root->Children) if (und->Type == CplNodeType::Using)
				res |= AddUsing(und);
			for (CplNode* nsp : root->Children) if (nsp->Type == CplNodeType::NamespaceDefine) 
				res |= BuildNamespaceEType(nsp);
			// debug_print_cpltree(root, 0);
		}
		return 0;
	}
}