#include "vcppcpl.h"
#include <stack>

namespace Interpreter {
	vector<Token> *token_list;
	inline Token &tk(int index) { return (*token_list)[index]; }

#pragma region 调试相关
#ifdef DEBUG_MODULE
	static string CplNodeTypeString[] = {
		"Empty", "Error", "Block",
		"Identifier", "FuncCall", "Integer", "Float", "String", "Char",
		"Expression",
		"Comma"/* , */, "Assign"/* = */,
		"Add" /* + */, "Minus"/* - */, "Mul"/* * */, "Divison" /* / */, "Mod", "BitAnd", "BitOr", "BitXor", "BitNot", "Lmov", "Rmov",
		//比较运算符（都是缩写）
		"Equ", "Neq", "Gt", "Ge", "Ls", "Le",
		"LogicAnd", "LogicOr", "LogicNot",
		"CallMember"/* . */, "New", "As", "Region", "ArrIndex",
		"VarDefine", "FuncDefine", "ClassDefine", "NamespaceDefine",
		"If", "Else", "Switch", "Case", "While", "For", "Continue", "Break", "Return",
		"Using", "Super",
		"VasmRegion",
		"Declaration"/*声明*/, "Definition"
	};
	void debug_print_cpltree(CplNode *nd, int dep) {
		for (int i = 1; i < dep; i++) printf("  ");
		PrintLog(CplNodeTypeString[(int)nd->Type], FOREGROUND_RED | FOREGROUND_INTENSITY);
		if ((nd->Type >= CplNodeType::Identifier && nd->Type <= CplNodeType::ArrIndex)
			|| (nd->Type >= CplNodeType::VarDefine && nd->Type <= CplNodeType::FuncDefine)) {
			PrintLog(" res_info = ", FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			PrintLog(nd->ExprType.ToString(), FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		}
		PrintLog(" lvar_cnt = ", FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		PrintLog(to_string(nd->LocalVarCount) + "\n", FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		for (int i = 1; i < dep; i++) printf("  ");
		cout << ">> TokenContent : ";
		PrintToken(nd->Content);
		for (auto child : nd->Children) debug_print_cpltree(child, dep + 1);
	}
#endif
#pragma endregion

#pragma region 括号匹配
	static inline bool IsBracketLeft(TokenType tkt) {
		return tkt == TokenType::LBracketL || tkt == TokenType::MBracketL || tkt == TokenType::SBracketL;
	}
	static inline bool IsBracketRight(TokenType tkt) {
		return tkt == TokenType::LBracketR || tkt == TokenType::MBracketR || tkt == TokenType::SBracketR;
	}
	stack<pair<int, int>> bracket_stack;
	int Matchbracket() {
		while (!bracket_stack.empty()) bracket_stack.pop();
		string error_msg = "Can't match brackets\n";
		for (int i = 0; i < token_list->size(); i++) {
			TokenType tkt = tk(i).Type;
			switch (tkt) {
				case TokenType::SBracketL:
					bracket_stack.push(make_pair(i, 1)); break;
				case TokenType::SBracketR:
					if (bracket_stack.empty() || bracket_stack.top().second != 1) {
						PrintError(error_msg, tk(i).Line);
						return -1;
					}
					tk(i).Ulong = bracket_stack.top().first;
					tk(bracket_stack.top().first).Ulong = i;
					bracket_stack.pop();
					break;
				case TokenType::MBracketL:
					bracket_stack.push(make_pair(i, 2)); break;
				case TokenType::MBracketR:
					if (bracket_stack.empty() || bracket_stack.top().second != 2) {
						PrintError(error_msg, tk(i).Line);
						return -1;
					}
					tk(i).Ulong = bracket_stack.top().first;
					tk(bracket_stack.top().first).Ulong = i;
					bracket_stack.pop();
					break;
				case TokenType::LBracketL:
					bracket_stack.push(make_pair(i, 3)); break;
				case TokenType::LBracketR:
					if (bracket_stack.empty() || bracket_stack.top().second != 3) {
						PrintError(error_msg, tk(i).Line);
						return -1;
					}
					tk(i).Ulong = bracket_stack.top().first;
					tk(bracket_stack.top().first).Ulong = i;
					bracket_stack.pop();
					break;
			}
		}
		if (!bracket_stack.empty()) {
			PrintError(error_msg);
			return -1;
		}
		return 0;
	}
#pragma endregion

#pragma region 结构体函数
	CplNode::CplNode(CplNodeType type) {
		LocalVarCount = 0;
		Type = type;
		Parent = nullptr;
	}

	void CplNode::AddChild(CplNode *child) { 
		Children.emplace_back(child), LocalVarCount += child->LocalVarCount; 
		child->Parent = this;
		if (child != nullptr && child->Type == CplNodeType::Error) Type = CplNodeType::Error;
	}
	void CplNode::InsertChild(int index, CplNode *child) {
		auto iter = (Children.begin() + index);
		Children.insert(iter, child), LocalVarCount += child->LocalVarCount;
		child->Parent = this;
		if (child != nullptr && child->Type == CplNodeType::Error) Type = CplNodeType::Error;
	}
	void CplNode::ChangeChild(int index, CplNode *child) {
		LocalVarCount += child->LocalVarCount - Children[index]->LocalVarCount;
		Children[index] = child, child->Parent = this;
		if (child != nullptr && child->Type == CplNodeType::Error) Type = CplNodeType::Error;
	}
	CplNode *&CplNode::At(int index) { return Children[index]; }
	CplNode::~CplNode() {
		for (CplNode *child : Children) delete child;
	}
#pragma endregion
	
	const string IdenError = "[Error]";
	//得到标识符的索引字符串 
	string BuildIdentifier(int st, int &ed) {
		string res;
		TokenType lst = TokenType::Empty;
		for (ed = st; tk(ed).Type != lst && (tk(ed).Type == TokenType::Identifier || tk(ed).Type == TokenType::Region); ed++) {
			lst = tk(ed).Type;
			Token &token = tk(ed);
			if (token.Type == TokenType::Identifier) res.append(token.String);
			else if (token.Type == TokenType::Region) res.push_back(IdentifierPathSeparateChar);
			else {
				PrintError("Invalid identifier syntax", tk(st).Line), res = IdenError;
				return res;
			}
		}
		ed--;
		if (res.empty() || tk(ed).Type == TokenType::Region || tk(st).Type == TokenType::Region)
			PrintError("Invalid identifier syntax", tk(st).Line),
			res = IdenError;
		return res;
	}

	CplNode *BuildUsingNode(int st, int &ed) {
		CplNode *nd = new CplNode(CplNodeType::Using);
		string nsp = BuildIdentifier(st + 1, ed);
		if (nsp == IdenError) nd->Type = CplNodeType::Error;
		nd->Content.String = nsp;
		ed++;
		return nd;
	}

	CplNode *BuildNode(int st, int &ed);

#pragma region 表达式
	ExpressionType GetEType(int st, int &ed) {
		ExpressionType res;
		string t_iden = BuildIdentifier(st, ed);
		res.TypeName = t_iden;
		while (tk(ed + 1).Type == TokenType::MBracketL) res.Dimc++, ed = tk(ed + 1).Ulong;
		return res;
	}

	const int OperWeight[] = {
		1/* , */, 2/* = */,
		11 /*+*/, 11/*-*/, 12/***/, 12 /*/*/, 12/*%*/ , 7/*&*/, 5/*|*/, 6/*^*/, 13/*~*/, 10/*<<*/, 10/*>>*/,
		//比较运算符（都是缩写）
		8/*==*/, 8/*!=*/, 9/*>*/, 9/*>=*/, 9/*<*/, 9/*<=*/,
		4/*&&*/, 3/*||*/, 13/*!*/,
		14/* . */, 13/*$*/, 13/*=>*/, 15/*@*/, 14/*[]*/,
	};
	const int IdenWeight = 20;

	inline bool IsOperatorToken(TokenType tkt) { return tkt >= OperTokenTypeStart && tkt <= OperTokenTypeEnd; }
	CplNode *BuildExpressionNode(int st, int ed) {
		if (st > ed) { return new CplNode(); }
		vector<CplNode *> expr_ele;
		CplNode *nd = new CplNode(CplNodeType::Expression);
		for (int i = st; i <= ed; i++) {
			Token &fir = tk(i);
			CplNode *ele = nullptr;
			switch (fir.Type) {
 				case TokenType::Identifier:
					{
						ele = new CplNode(CplNodeType::Identifier);
						string iden = BuildIdentifier(i, i);
						if (iden == IdenError) {
							for (CplNode *nd : expr_ele) delete nd;
							return nd->Type = CplNodeType::Error, nd;
						}
						ele->Content.String = iden;
						//是一个函数调用
						if (tk(i + 1).Type == TokenType::SBracketL) {
							ele->Type = CplNodeType::FuncCall;
							i++; int to = tk(i++).Ulong;
							for (; i < to; i++) {
								int st = i;
								for (; tk(i).Type != TokenType::Comma && i < to; i++)
									if (IsBracketLeft(tk(i).Type)) i = tk(i).Ulong;
								ele->AddChild(BuildExpressionNode(st, i - 1));
							}
							i = to;
						}
					}
					break;
				case TokenType::MBracketL:
					{
						CplNode *oper_nd = new CplNode(CplNodeType::ArrIndex);
						oper_nd->Content.Type = TokenType::ArrIndex;
						expr_ele.emplace_back(oper_nd);
						ele = BuildExpressionNode(i + 1, tk(i).Ulong - 1);
						i = tk(i).Ulong;
					}
					break;
				case TokenType::Integer:
					ele = new CplNode(CplNodeType::Integer);
					ele->Content = fir;
					break;
				case TokenType::Float:
					ele = new CplNode(CplNodeType::Float);
					ele->Content = fir;
					break;
				case TokenType::String:
					ele = new CplNode(CplNodeType::String);
					ele->Content = fir;
					break;
				case TokenType::Char:
					ele = new CplNode(CplNodeType::Char);
					ele->Content = fir;
					break;
				case TokenType::SBracketL:
					ele = BuildExpressionNode(i + 1, tk(i).Ulong - 1);
					i = tk(i).Ulong;
					break;
				default:
					if (IsOperatorToken(fir.Type)) 
						ele = new CplNode((CplNodeType)((int)OperCplNodeTypeStart + (int)fir.Type - (int)OperTokenTypeStart)),
						ele->Content = fir;
					else {
						ele = new CplNode(CplNodeType::Error);
						PrintError("Invalid content of a expression", fir.Line);
						for (CplNode *nd : expr_ele) delete nd;
						return nd->Type = CplNodeType::Error, nd;
					}
					break;
			}
			if (ele == nullptr) continue;
			expr_ele.emplace_back(ele);
		}
		const auto &build_exprtree = [&expr_ele]() -> CplNode* {
			const auto &func = [&expr_ele](auto &&self, int l, int r) -> CplNode * {
				if (l == r) return expr_ele[l];
				int mnp = l - 1, mnw = IdenWeight + 1;
				for (int i = l; i <= r; i++) {
					int w = IdenWeight + 1;
					if (IsOperatorToken(expr_ele[i]->Content.Type))
						w = OperWeight[(int)expr_ele[i]->Type - (int)OperCplNodeTypeStart];
					else w = IdenWeight;
					if (w <= mnw) mnw = w, mnp = i;
				}
				CplNode *lnd = nullptr, *rnd = nullptr, *res = expr_ele[mnp];
				if (mnp > l) lnd = self(self, l, mnp - 1), res->AddChild(lnd);
				if (mnp < r) rnd = self(self, mnp + 1, r), res->AddChild(rnd);
				return res;
			};
			return func(func, 0, expr_ele.size() - 1);
		};
		nd->AddChild(build_exprtree());
		return nd;
	} 
#pragma endregion
	
#pragma region 流程控制
	CplNode *BuildIfNode(int st, int &ed) {
		CplNode *nd = new CplNode(CplNodeType::If), *cond = nullptr, *succ = nullptr, *fail = nullptr;
		if (tk(st + 1).Type != TokenType::SBracketL) {
			PrintError("The condition expression of \"if\" must be included by ()", tk(st + 1).Line);
			nd->Type = CplNodeType::Error;
			return nd;
		}
		cond = BuildExpressionNode(st + 2, tk(st + 1).Ulong - 1);
		nd->AddChild(cond);
		ed = tk(st + 1).Ulong + 1;
		succ = BuildNode(ed, ed);
		nd->AddChild(succ);
		if (tk(ed + 1).Type == TokenType::Else) {
			ed += 2;
			fail = BuildNode(ed, ed);
			nd->AddChild(fail);
		}
		return nd;
	}
	CplNode *BuildWhileNode(int st, int &ed) {
		CplNode *nd = new CplNode(CplNodeType::While), *cond = nullptr, *ct = nullptr;
		if (tk(st + 1).Type != TokenType::SBracketL) {
			PrintError("The condition expression of \"while\" must be included by ()", tk(st + 1).Line);
			nd->Type = CplNodeType::Error;
			return nd;
		}
		cond = BuildExpressionNode(st + 2, tk(st + 1).Ulong - 1);
		nd->AddChild(cond);
		ed = tk(st + 1).Ulong + 1;
		ct = BuildNode(ed, ed);
		nd->AddChild(ct);
		return nd;
	}
	CplNode *BuildForNode(int st, int &ed) {
		CplNode *nd = new CplNode(CplNodeType::For), *init = nullptr, *cond = nullptr, *upd = nullptr, *ct = nullptr;
		if (tk(st + 1).Type != TokenType::SBracketL) {
			PrintError("The condition expression of \"for\" must be included by ()", tk(st + 1).Line);
			nd->Type = CplNodeType::Error;
			return nd;
		}
		st++; int pos = st + 1;
		init = BuildNode(pos, pos);
		if (init->Type != CplNodeType::VarDefine && init->Type != CplNodeType::Expression) {
			PrintError("Invalid content of the init part of \"for\".", tk(pos + 1).Line);
			nd->Type = CplNodeType::Error;
		}
		nd->AddChild(init);
		cond = BuildNode(pos + 1, pos);
		if (cond->Type != CplNodeType::Expression) {
			PrintError("Invalid content of the condition part of \"for\".", tk(pos + 1).Line);
			nd->Type = CplNodeType::Error;
		}
		nd->AddChild(cond);
		upd = BuildExpressionNode(pos + 1, tk(st).Ulong - 1);
		nd->AddChild(upd);
		ed = tk(st).Ulong;
		ct = BuildNode(ed + 1, ed);
		nd->AddChild(ct);
		return nd;
	}
	CplNode *BuildContinueNode(int st, int &ed) {
		ed = st + 1;
		return new CplNode(CplNodeType::Continue);
	}
	CplNode *BuildBreakNode(int st, int &ed) {
		ed = st + 1;
		return new CplNode(CplNodeType::Break);
	}
	CplNode *BuildReturnNode(int st, int &ed) {
		CplNode *nd = new CplNode(CplNodeType::Return), *expr = nullptr;
		ed = st + 1;
		while (tk(ed).Type != TokenType::ExpressionEnd) ed++;
		expr = BuildExpressionNode(st + 1, ed - 1);
		nd->AddChild(expr);
		return nd;
	}
	CplNode *BuildSuperNode(int st, int &ed) {
		CplNode *nd = new CplNode(CplNodeType::Super);
		ed = st + 1;
		if (tk(ed).Type != TokenType::SBracketL) {
			PrintError("\"super\" must be used as a function or an identifier.", tk(ed).Line);
			while (tk(ed).Type != TokenType::ExpressionEnd) ed++;
			return nd;
		}
		CplNode *arg_nd = new CplNode(CplNodeType::FuncCall);
		nd->AddChild(arg_nd);
		int to = tk(ed++).Ulong;
		for (int to = tk(ed++).Ulong; ed < to; ed++) {
			int l = ed;
			for (; ed < to && tk(ed).Type != TokenType::Comma; ed++)
				if (IsBracketLeft(tk(ed).Type)) ed = tk(ed).Ulong;
			CplNode* expr = BuildExpressionNode(l, ed - 1);
			arg_nd->AddChild(expr);
			if (ed == to) ed--;
		}
		ed = to + 1;
		return nd;
	}
#pragma endregion

	CplNode *BuildVasmRegionNode(int st, int &ed) {
		CplNode *nd = new CplNode(CplNodeType::VasmRegion);
		nd->Content = tk(ed = st);
		return nd;
	}
	CplNode *BuildBlockNode(int st, int &ed) {
		ed = tk(st).Ulong;
		CplNode *nd = new CplNode(CplNodeType::Block);
		int lcl_vcnt = 0, res_lcl_vcnt = 0;
		for (int pos = st + 1; pos < ed; pos++) {
			CplNode *child = BuildNode(pos, pos);
			nd->AddChild(child);
			res_lcl_vcnt = max(res_lcl_vcnt, lcl_vcnt + child->LocalVarCount);
			if (child->Type == CplNodeType::VarDefine) lcl_vcnt += child->LocalVarCount;
		}
		nd->LocalVarCount = res_lcl_vcnt;
		return nd;
	}
	
#pragma region 变量，函数，类，命名空间的定义
	inline bool IsVisibilityToken(TokenType tkt) { return tkt >= TokenType::Private && tkt <= TokenType::Public; }

	CplNode *BuildVarDefNode(int st, int &ed) {
		CplNode *nd = new CplNode(CplNodeType::VarDefine); 
		nd->Content.Type = TokenType::Private;
		if (st && IsVisibilityToken(tk(st - 1).Type)) nd->Content.Type = tk(st - 1).Type;
		//跳过var
		st = ++ed;
		ExpressionType v_type = GetEType(st, ed);
		v_type.IsQuote = true;
		nd->ExprType = v_type;
		for (ed++; tk(ed).Type != TokenType::ExpressionEnd; ed++) {
			if (tk(ed).Type == TokenType::Comma) continue;
			if (tk(ed).Type != TokenType::Identifier) {
				PrintError("Invalid identifier : " + tk(ed).String, tk(ed).Line);
				nd->Type = CplNodeType::Error;
				while (tk(ed).Type != TokenType::ExpressionEnd) ed++;
				return nd;
			}
			CplNode *iden_nd = new CplNode(CplNodeType::Identifier), *expr_nd;
			iden_nd->Content = tk(ed);
			iden_nd->ExprType = v_type;
			int l = ed;
			for (; tk(ed).Type != TokenType::Comma && tk(ed).Type != TokenType::ExpressionEnd; ed++)
				if (IsBracketLeft(tk(ed).Type)) ed = tk(ed).Ulong;
			ed--;
			expr_nd = BuildExpressionNode(l, ed);
			if (expr_nd->Type == CplNodeType::Error) {
				nd->Type = CplNodeType::Error;
				while (tk(ed).Type != TokenType::ExpressionEnd) ed++;
				return nd;
			}
			nd->AddChild(iden_nd), nd->AddChild(expr_nd);
			nd->LocalVarCount++;
		}
		return nd;
	}
	CplNode *BuildFunctionNode(int st, int &ed) {
		CplNode *nd = new CplNode(CplNodeType::FuncDefine);
		//先前的一个tk表示函数的Visibility
		nd->Content.Type = TokenType::Private;
		if (st && IsVisibilityToken(tk(st - 1).Type)) nd->Content.Type = tk(st - 1).Type;
		//跳过func
		st = ++ed;
		//获取返回值
		//如果是一个构造函数，那么就没有返回值，且这个标识符应该是 __init__
		if (tk(st).String != "__init__") {
			ExpressionType res_type = GetEType(st, ed);
			nd->ExprType = res_type;
			ed++;
		}
		//获取函数名称
		nd->Content.String = tk(ed).String;
		//获取参数表
		for (ed += 2; tk(ed).Type != TokenType::SBracketR; ed++) {
			if (tk(ed).Type == TokenType::Comma) continue;
			ExpressionType arg_type = GetEType(ed, ed);
			CplNode *arg_nd = new CplNode(CplNodeType::Identifier);
			arg_nd->ExprType = arg_type;
			arg_nd->ExprType.IsQuote = true;
			arg_nd->Content = tk(++ed);
			nd->AddChild(arg_nd);
			nd->LocalVarCount++;
		}
		CplNode *ct_nd = BuildNode(ed + 1, ed);
		if (ct_nd->Type == CplNodeType::Error) nd->Type = CplNodeType::Error;
		nd->AddChild(ct_nd);
		return nd;
	}
	CplNode *BuildClassNode(int st, int &ed) {
		CplNode *nd = new CplNode(CplNodeType::ClassDefine), *par_nd = new CplNode(CplNodeType::Identifier);
		string pare_iden = "object";
		nd->Content.Type = TokenType::Private;
		if (st && IsVisibilityToken(tk(st - 1).Type)) nd->Content.Type = tk(st - 1).Type;
		st = ++ed;
		nd->Content.String = tk(st).String;
		ed++;
		//有类继承
		if (tk(ed).Type == TokenType::Region) {
			ed++;
			pare_iden = BuildIdentifier(ed, ed);
			if (pare_iden == IdenError) nd->Type = CplNodeType::Error;
			ed++;
		} 
		par_nd->Content.String = pare_iden; 
		nd->AddChild(par_nd);
		if (tk(ed).Type != TokenType::LBracketL) {
			PrintError("The content of class must be included by {}", tk(ed).Line);
			nd->Type = CplNodeType::Error;
			return nd;
		}
		for (int to = tk(ed++).Ulong; ed < to; ed++) {
			Token &fir = tk(ed);
			CplNode *child = nullptr;
			bool flag = false;
			switch (fir.Type) {
				case TokenType::Private:
				case TokenType::Public:
				case TokenType::Protected:
					break;
				case TokenType::FuncDefine:
					child = BuildFunctionNode(ed, ed);
					break;
				case TokenType::VarDefine:
					child = BuildVarDefNode(ed, ed);
					//修改ExpressionType
					child->ExprType.IsMember = true;
					for (int i = 0; i < child->Children.size(); i += 2) child->At(i)->ExprType.IsMember = true;
					break;
				default:
					PrintError("Invalid content", fir.Line);
					break;
			} 
			if (child == nullptr) continue;
			if (child->Type == CplNodeType::Error) nd->Type = CplNodeType::Error;
			nd->AddChild(child);
		}
		return nd;
	}
	CplNode *BuildNamespaceNode(int st, int &ed) {
		CplNode *nd = new CplNode(CplNodeType::NamespaceDefine);
		st = ++ed;
		string nsp_name = BuildIdentifier(st, ed);
		if (nsp_name.size())
		nd->Content.String = nsp_name;
		ed++;
		if (tk(ed).Type != TokenType::LBracketL) {
			PrintError("The content of namespace must be included by {}", tk(ed).Line);
			nd->Type = CplNodeType::Error;
			return nd;
		}
		for (int to = tk(ed++).Ulong; ed < to; ed++) {
			Token &fir = tk(ed);
			if (IsVisibilityToken(fir.Type)) continue;
			CplNode *child = nullptr;
			switch (fir.Type) {
				case TokenType::VarDefine:
					child = BuildVarDefNode(ed, ed);
					break;
				case TokenType::FuncDefine:
					child = BuildFunctionNode(ed, ed);
					break;
				case TokenType::ClassDefine:
					child = BuildClassNode(ed, ed);
					break;
			}
			if (child == nullptr) {
				PrintError("Invalid content of namespace", tk(ed).Line);
				ed = to;
				return nd;
			}
			nd->AddChild(child);
		}
		return nd;
	}
#pragma endregion
	CplNode *BuildNode(int st, int &ed) {
		CplNode *nd = nullptr;
		ed = st;
		switch (tk(st).Type) {
			case TokenType::LBracketL:
				nd = BuildBlockNode(st, ed);
				break;
			case TokenType::Identifier:
			case TokenType::MBracketL:
			case TokenType::SBracketL:
			default:
				while (tk(ed).Type != TokenType::ExpressionEnd) ed++;
				nd = BuildExpressionNode(st, ed - 1);
				break;
			case TokenType::VarDefine:
				nd = BuildVarDefNode(st, ed);
				break;
			case TokenType::If:
				nd = BuildIfNode(st, ed);
				break;
			case TokenType::While:
				nd = BuildWhileNode(st, ed);
				break;
			case TokenType::For:
				nd = BuildForNode(st, ed);
				break;
			case TokenType::Continue:
				nd = BuildContinueNode(st, ed);
				break;
			case TokenType::Break:
				nd = BuildBreakNode(st, ed);
				break;
			case TokenType::Return:
				nd = BuildReturnNode(st, ed);
				break;
			case TokenType::VasmRegion:
				nd = BuildVasmRegionNode(st, ed);
				break;
			case TokenType::Super:
				nd = BuildSuperNode(st, ed);
				break;
			case TokenType::ExpressionEnd:
				nd = new CplNode();
				break;
		}
		return nd;
	}

	CplNode *CplTree_Build(vector<Token> *tk_list) {
		token_list = tk_list;
		int res = Matchbracket();
		if (res == -1) return nullptr;
		CplNode *root = new CplNode(), *glo_using, *glo_nsp;
		//加入一个namespace Global的节点 
		glo_nsp = new CplNode();
		glo_nsp->Type = CplNodeType::NamespaceDefine;
		glo_nsp->Content.String = "Global";
		root->AddChild(glo_nsp);
		//接着加入文件中的内容 
		for (int st = 0, ed = 0; st < token_list->size(); st = ++ed) {
			Token &fir = tk(st);
			CplNode *child = nullptr;
			bool flag = false;
			switch (fir.Type) {
				case TokenType::Using:
					child = BuildUsingNode(st, ed);
					root->AddChild(child);
					break;
				case TokenType::NamespaceDefine:
					child = BuildNamespaceNode(st, ed);
					root->AddChild(child);
					break;
				case TokenType::VarDefine:
					child = BuildVarDefNode(st, ed);
					glo_nsp->AddChild(child);
					break;
				case TokenType::FuncDefine:
					child = BuildFunctionNode(st, ed);
					glo_nsp->AddChild(child);
					break;
				case TokenType::ClassDefine:
					child = BuildClassNode(st, ed);
					glo_nsp->AddChild(child);
					break;
				case TokenType::Private:
				case TokenType::Public:
				case TokenType::Protected:
					break;
				default:
					child = new CplNode(CplNodeType::Error);
					PrintError("Invalid content ", fir.Line);
					flag = true;
					break;
			}
			if (child == nullptr) continue;
			if (child->Type == CplNodeType::Error && flag) return root->Type == CplNodeType::Error, root;
		}
		return root;
	}
}