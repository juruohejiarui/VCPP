#include "vcppcpl.h"

namespace Interpreter {
	ofstream vasm_stream;
	static string command_name_list[] = { COMMAND_NAME_LS };
	void VasmBuilder_WriteCommand(vbyte cmd, ulong arg1, ulong arg2) {
		if (cmd == arrmem && arg1 == 1) { vasm_stream << command_name_list[arrmem1] << endl; return; }
		else if (cmd == arromem && arg1 == 1) { vasm_stream << command_name_list[arromem1] << endl; return; }
		else if (cmd == pvar && arg1 < 4) { vasm_stream << command_name_list[pvar0 + arg1] << endl; return; }
		else if (cmd == povar && arg1 < 4) { vasm_stream << command_name_list[povar0 + arg1] << endl; return; }
		else if (cmd == push && arg1 < 2) { vasm_stream << command_name_list[push0 + arg1] << endl; return; }
		vasm_stream << command_name_list[cmd];
		if (cmd > CMD0_COUNT) vasm_stream << " " << arg1;
		if (cmd > CMD0_COUNT + CMD1_COUNT) vasm_stream << " " << arg2;
		vasm_stream << endl;
	}
	void VasmBuilder_WriteCommandStr(vbyte cmd, string arg1, ulong arg2) {
		vasm_stream << command_name_list[cmd] << " " << arg1;
		if (cmd > CMD0_COUNT + CMD1_COUNT) vasm_stream << " " << arg2;
		vasm_stream << endl;
	}

    void VasmBuilder_WriteEXCommand(ulong cmd, ulong arg1, ulong *arg2) {
		vasm_stream << "excmd " << EXCommandName[cmd];
		if (cmd > EXCMD0_COUNT) vasm_stream << " " << arg1;
		vasm_stream << endl;
    }

#define wrtcmd VasmBuilder_WriteCommand
#define wrtcmdstr VasmBuilder_WriteCommandStr


	struct LoopInfo {
		string Start, CtEnd, End; VarFrame* Frame;
		LoopInfo() {}
		LoopInfo(int id) {
			string id_str = to_string(id);
			Start = "@LOOP_START" + id_str;
			CtEnd = "@LOOP_CTEND" + id_str;
			End = "@LOOP_END" + id_str;
			Frame = LocalVarStack_Top();
		}
	};
	stack<LoopInfo> loop_stack;
	struct CondInfo {
		string Succ, Fail, End;
		CondInfo() {}
		CondInfo(int id) {
			string id_str = to_string(id);
			Succ = "@IF_SUCC" + id_str;
			Fail = "@IF_FAIL" + id_str;
			End = "@IF_END" + id_str;
		}
	};
	int loop_count, cond_count, logic_count, string_count;

	int BuildNodeVasm(CplNode *node);

#pragma region Expression 
	void BuildGVLVasm(ExpressionType &type) {
		if (!type.IsQuote || type == void_etype) return;
		int cid;
		if (type == int32_etype) cid = vi32gvl;
		else if (type == char_etype) cid = vbgvl;
		else if (type == int64_etype || type == uint64_etype) cid = vi64gvl;
		else if (type == float64_etype) cid = vfgvl;
		else cid = vogvl;
		if (type.IsMember) cid = cid + (mi32gvl - vi32gvl);
		wrtcmd(cid);
	}

	void BuildPOPVasm(ExpressionType &type) {
		if (type == void_etype) return ;
		if (type.IsQuote) BuildGVLVasm(type);
		wrtcmd(IsBasicType(type) ? pop : opop);
	}
	int BuildExpressionVasm(CplNode *node) {
		auto write_identifier_cmd = [](CplNode *node) {
			if (node->ExprType.Dimc < (TypeVarDimc >> 1)) {
				int cmd = (node->Content.Char ? pglo : pvar);
				//检查是否需要改为一个取对象变量的指令
				if (!IsBasicType(node->ExprType)) cmd++;
				wrtcmd(cmd, node->Content.Ulong);
			}
			else wrtcmd(push, min(node->ExprType.Type->Size, 8ull));
		};
		int res = 0;
		switch (node->Type) {
			case CplNodeType::Integer:
				wrtcmd(push, node->Content.Ulong);
				break;
			case CplNodeType::Float:
				wrtcmd(push, *(ulong*)&node->Content.Float);
				break;
			case CplNodeType::String:
				vasm_stream << "#STRING \"" << node->Content.String << "\"\n";
				wrtcmd(pstr, string_count++);
				break;
			case CplNodeType::Char:
				wrtcmd(push, node->Content.Char);
				break;
			case CplNodeType::Expression:
				res = BuildExpressionVasm(node->At(0));
				break;
			case CplNodeType::Identifier:
				write_identifier_cmd(node);
				break;
			case CplNodeType::FuncCall:
				//是一个全局函数调用
				for (auto arg : node->Children) res |= BuildExpressionVasm(arg), BuildGVLVasm(arg->ExprType);
				wrtcmdstr(call, node->Content.String, node->Children.size());
				break;
			case CplNodeType::ArrIndex:
				{
					vector<CplNode *> part;
					auto cur = node;
					for (; cur->Type == CplNodeType::ArrIndex; cur = cur->At(0))
						part.emplace_back(cur->At(1));
					part.emplace_back(cur);
					reverse(part.begin(), part.end());
					for (int i = 0; i < part.size(); i++) 
						res |= BuildExpressionVasm(part[i]), BuildGVLVasm(part[i]->ExprType);
					if (IsBasicType(node->ExprType)) wrtcmd(arrmem, part.size() - 1);
					else wrtcmd(arromem, part.size() - 1);
				}
				break;
			case CplNodeType::Comma:
				res |= BuildExpressionVasm(node->At(0));
				BuildPOPVasm(node->At(0)->ExprType);
				res |= BuildExpressionVasm(node->At(1));
				break;
			case CplNodeType::New:
				if (node->At(0)->Type == CplNodeType::FuncCall) {
					wrtcmd(push, node->ExprType.Type->Size);
					wrtcmd(_new);
					wrtcmd(ocpy);
					for (auto arg : node->At(0)->Children) res |= BuildExpressionVasm(arg), BuildGVLVasm(arg->ExprType);
					wrtcmdstr(call, node->Content.String, node->At(0)->Children.size() + 1);
				} else if (node->ExprType.Dimc) {
					vector<CplNode *> part;
					CplNode* cur = node->At(0);
					for (; cur->Type == CplNodeType::ArrIndex; cur = cur->At(0)) part.emplace_back(cur->At(1));
					part.emplace_back(cur);
					reverse(part.begin(), part.end());
					for (auto nd : part) res |= BuildExpressionVasm(nd), BuildGVLVasm(nd->ExprType);
					wrtcmd(arrnew, part.size() - 1);
				} else {
					res |= BuildExpressionVasm(node->At(0)), BuildGVLVasm(node->At(0)->ExprType);
					wrtcmd(_new);
				}
				break;
			case CplNodeType::Assign:
				{
					res |= BuildExpressionVasm(node->At(0)) | BuildExpressionVasm(node->At(1));
					BuildGVLVasm(node->At(1)->ExprType);
					int cid = 0; auto &type = node->At(0)->ExprType;
					if (!IsBasicType(type)) cid = vomov;
					else if (type == int32_etype) cid = vi32mov;
					else if (type == int64_etype || type == uint64_etype) cid = vi64mov;
					else if (type == float64_etype) cid = vfmov;
					else if (type == char_etype) cid = vbmov;
					if (type.IsMember) cid += (mi32mov - vi32mov);
					wrtcmd(cid);
				}
				break;
			case CplNodeType::As:
				res = BuildExpressionVasm(node->At(0));
				if (IsBasicType(node->At(0)->ExprType) != IsBasicType(node->At(1)->ExprType))
					BuildGVLVasm(node->At(0)->ExprType),
					wrtcmd(IsBasicType(node->At(0)->ExprType) ? pack : unpack);
				break;
			case CplNodeType::CallMember:
				res |= BuildExpressionVasm(node->At(0)), BuildGVLVasm(node->At(0)->ExprType);
				if (node->At(1)->Type == CplNodeType::FuncCall) {
					for (auto arg : node->At(1)->Children) res |= BuildExpressionVasm(arg), BuildGVLVasm(arg->ExprType);
					wrtcmdstr(call, node->Content.String, node->At(1)->Children.size() + 1);
				} else {
					if (IsBasicType(node->ExprType)) wrtcmd(mem, node->Content.Ulong);
					else wrtcmd(omem, node->Content.Ulong);
				}
				break;
			case CplNodeType::Add:
			case CplNodeType::Minus:
			case CplNodeType::Mul:
			case CplNodeType::Divison:
			case CplNodeType::Mod:
				{
					res = BuildExpressionVasm(node->At(0)), BuildGVLVasm(node->At(0)->ExprType);
					res |= BuildExpressionVasm(node->At(1)), BuildGVLVasm(node->At(1)->ExprType);
					int cid = (int)node->Type - (int)CplNodeType::Add + add;
					if (node->ExprType == int64_etype) cid += ladd - add;
					else if (node->ExprType == uint64_etype) cid += uadd - add;
					else if (node->ExprType == float64_etype) cid += fadd - add;
					else if (node->ExprType == char_etype) cid += badd - add;
					wrtcmd(cid);
				}
				break;
			case CplNodeType::BitAnd:
			case CplNodeType::BitOr:
			case CplNodeType::BitXor:
			case CplNodeType::Lmov:
			case CplNodeType::Rmov:
			case CplNodeType::BitNot:
				{
					res |= BuildExpressionVasm(node->At(0)), BuildGVLVasm(node->At(0)->ExprType);
					if (node->Children.size() > 1) res |= BuildExpressionVasm(node->At(1)), BuildGVLVasm(node->At(1)->ExprType);
					int cid = (int)node->Type - (int)CplNodeType::BitAnd + _and;
					if (node->ExprType == int64_etype) cid += (land - _and);
					if (node->ExprType == uint64_etype) cid += (uand - _and);
					else if (node->ExprType == char_etype) cid += band - _and;
					wrtcmd(cid);
				}
				break;
			case CplNodeType::LogicAnd:
				{

					string idstr = to_string(logic_count++), fil = "@LOGIC_FAIL" + idstr, end = "@LOGIC_END" + idstr;
					res |= BuildExpressionVasm(node->At(0)), BuildGVLVasm(node->At(0)->ExprType);
					wrtcmdstr(jz, fil);
					res |= BuildExpressionVasm(node->At(1)), BuildGVLVasm(node->At(1)->ExprType);
					wrtcmdstr(jz, fil);
					wrtcmd(push, 1);
					wrtcmdstr(jmp, end);
					wrtcmdstr(label, fil);
					wrtcmd(push, 0);
					wrtcmdstr(label, end);
				}
				break;
			case CplNodeType::LogicOr:
				{
					string idstr = to_string(logic_count++), succ = "@LOGIC_SUCC" + idstr, end = "@LOGIC_END" + idstr;
					res |= BuildExpressionVasm(node->At(0)), BuildGVLVasm(node->At(0)->ExprType);
					wrtcmdstr(jp, succ);
					res |= BuildExpressionVasm(node->At(1)), BuildGVLVasm(node->At(1)->ExprType);
					wrtcmdstr(jp, succ);
					wrtcmd(push, 0);
					wrtcmdstr(jmp, end);
					wrtcmdstr(label, succ);
					wrtcmd(push, 1);
					wrtcmdstr(label, end);
				}
				break;
			case CplNodeType::Ls:
			case CplNodeType::Le:
			case CplNodeType::Gt:
			case CplNodeType::Ge:
			case CplNodeType::Equ:
			case CplNodeType::Neq:
				res = BuildExpressionVasm(node->At(0)), BuildGVLVasm(node->At(0)->ExprType);
				res |= BuildExpressionVasm(node->At(1)), BuildGVLVasm(node->At(1)->ExprType);
				{
					int cid = (int)node->Type - (int)CplNodeType::Equ + eq;
					if (node->At(0)->ExprType == float64_etype || node->At(1)->ExprType == float64_etype)
						cid += feq - eq;
					wrtcmd(cid);
				}
				break;
			case CplNodeType::PInc:
			case CplNodeType::SInc:
			case CplNodeType::PDec:
			case CplNodeType::SDec:
				res = BuildExpressionVasm(node->At(0));
				{	
					int cid = EX_vbsinc;
					if (node->ExprType == int32_etype) cid += (EX_vi32sinc - EX_vbsinc);
					if (node->ExprType == int64_etype) cid += (EX_vi64sinc - EX_vbsinc);
					if (node->ExprType == uint64_etype) cid += (EX_vusinc - EX_vbsinc);
					if (node->ExprType.IsMember) cid += EX_mbsinc - EX_vbsinc;
					if (node->Type == CplNodeType::PDec || node->Type == CplNodeType::SDec)
						cid += EX_vbsdec - EX_vbsinc;
					if (node->Type == CplNodeType::PInc || node->Type == CplNodeType::PDec)
						cid += EX_vbpinc - EX_vbsinc;
					if (node->At(0)->ExprType.IsMember) cid += EX_mbsinc - EX_vbsinc;
					VasmBuilder_WriteEXCommand(cid);
				}
				break;
			case CplNodeType::Empty:
				node->ExprType = void_etype;
				break;
		}
		return res;
	}
#pragma endregion
	int BuildIfVasm(CplNode* node) {
		CondInfo info(cond_count++);
		int res = BuildExpressionVasm(node->At(0));
		BuildGVLVasm(node->At(0)->ExprType);
		wrtcmdstr(jz, info.Fail);
		res |= BuildNodeVasm(node->At(1));
		wrtcmdstr(jmp, info.End);
		wrtcmdstr(label, info.Fail);
		if (node->Children.size() > 2) BuildNodeVasm(node->At(2));
		wrtcmdstr(label, info.End);
		return res;
	}

	int BuildWhiteVasm(CplNode* node) {
		LoopInfo info(loop_count++);
		loop_stack.push(info);
		wrtcmdstr(label, info.Start);
		int res = BuildExpressionVasm(node->At(0));
		BuildGVLVasm(node->At(0)->ExprType);
		wrtcmdstr(jz, info.End);
		res |= BuildNodeVasm(node->At(1));
		wrtcmdstr(label, info.CtEnd);
		wrtcmdstr(jmp, info.Start);
		wrtcmdstr(label, info.End);
		loop_stack.pop();
		return res;
	}

	int BuildForVasm(CplNode *node) {
		LocalVarStack_Push();
		LoopInfo info(loop_count++);
		loop_stack.push(info);
		int res = BuildNodeVasm(node->At(0));
		if (node->At(0)->Type == CplNodeType::Expression) BuildPOPVasm(node->At(0)->ExprType);
		wrtcmdstr(label, info.Start);
		res |= BuildExpressionVasm(node->At(1));
		if (node->At(1)->Type == CplNodeType::Expression) {
			BuildGVLVasm(node->At(1)->ExprType);
			wrtcmdstr(jz, info.End);
		}
		res |= BuildNodeVasm(node->At(3));
		wrtcmdstr(label, info.CtEnd);
		res |= BuildExpressionVasm(node->At(2));
		BuildPOPVasm(node->At(2)->ExprType);
		wrtcmdstr(jmp, info.Start);
		wrtcmdstr(label, info.End);
		LocalVarStack_Pop(true);
		loop_stack.pop();
		return res;
	}

	int BuildVarDefVasm(CplNode* node) {
		auto &type = node->ExprType;
		int res;
		for (int i = 0; i < node->Children.size(); i += 2) {
			auto &iden = node->At(i), &expr = node->At(i + 1);
			VarInfo *v_info = new VarInfo();
			v_info->Name = iden->Content.String;
			v_info->ExprType = node->ExprType;
			LocalVarStack_Top()->InsertVar(v_info);
			res |= BuildExpressionVasm(expr);
		}
		return res;
	}
	

	int BuildReturnVasm(CplNode* node) {
		int res = 0;
		if (node->Children.size()) res |= BuildExpressionVasm(node->At(0)), BuildGVLVasm(node->At(0)->ExprType);
		for (auto frm = LocalVarStack_Top(); frm != nullptr; frm = frm->Previous)
			frm->BuildCleanVasm();
		wrtcmd(ret);
		return res;
	}

	int BuildBreakVasm(CplNode* node) {
		int res = 0;
		LoopInfo &loop = loop_stack.top();
		for (auto frm = LocalVarStack_Top(); frm != loop.Frame; frm = frm->Previous)
			frm->BuildCleanVasm();
		wrtcmdstr(jmp, loop.End);
		return res;
	}
	int BuildContinueVasm(CplNode* node) {
		int res = 0;
		LoopInfo &loop = loop_stack.top();
		for (auto frm = LocalVarStack_Top(); frm != loop.Frame; frm = frm->Previous)
			frm->BuildCleanVasm();
		wrtcmdstr(jmp, loop.CtEnd);
		return res;
	}
	int BuildVasmRegionVasm(CplNode* node) {
		vasm_stream << node->Content.String << endl;
		return 0;
	}

	int BuildBlockVasm(CplNode* node) {
		LocalVarStack_Push();
		int res = 0;
		for (auto child : node->Children)
			res |= BuildNodeVasm(child);
		LocalVarStack_Pop(true);
		return res;
	}
	int BuildNodeVasm(CplNode *node) {
		int res = 0;
		switch(node->Type) {
			case CplNodeType::VarDefine: res = BuildVarDefVasm(node); break;
			case CplNodeType::Expression: res = BuildExpressionVasm(node), BuildPOPVasm(node->ExprType); break;
			case CplNodeType::If: res = BuildIfVasm(node); break;
			case CplNodeType::While: res = BuildWhiteVasm(node); break;
			case CplNodeType::For: res = BuildForVasm(node); break;
			case CplNodeType::Return: res = BuildReturnVasm(node); break;
			case CplNodeType::Break: res = BuildBreakVasm(node); break;
			case CplNodeType::Continue: res = BuildContinueVasm(node); break;
			case CplNodeType::VasmRegion: res = BuildVasmRegionVasm(node); break;
			case CplNodeType::Block: res = BuildBlockVasm(node); break;
		}
		return res;
	}

	int BuildFunctionVasm(CplNode* node) {
		LocalVarStack_Push();
		wrtcmdstr(label, node->Content.String);
		wrtcmd(setvar, node->LocalVarCount);
		wrtcmd(poparg, node->Children.size() - 1);
		for (int i = 0; i < node->Children.size() - 1; i++) {
			auto &arg = node->At(i);
			VarInfo* arg_info = new VarInfo();
			arg_info->Name = arg->Content.String;
			arg_info->ExprType = arg->ExprType;
			LocalVarStack_Top()->InsertVar(arg_info);
		}
		int res = BuildNodeVasm(node->At(node->Children.size() - 1));
		LocalVarStack_Pop(true);
		wrtcmd(ret);
		return res;
	}
	
	int VasmBuilder_Init() {
		LocalVarStack_Init();
		loop_count = cond_count = logic_count = 0;
		while (!loop_stack.empty()) loop_stack.pop();
		return 0;
	}
	
	int SearchFunctionNode(CplNode* node) {
		int res = 0;
		for (auto &child : node->Children) {
			if (child->Type == CplNodeType::FuncDefine) res |= BuildFunctionVasm(child);
			else res |= SearchFunctionNode(child);
		}
		return res;
	}
    int VasmBuilder_Build(vector<CplNode *> &roots, string vasm_path, bool is_append) {
		VasmBuilder_Init();
		vasm_stream.open(vasm_path, (is_append ? ios::app : ios::out));
        int res = 0;
		for (auto root : roots) if (root->Type == CplNodeType::Definition) res |= SearchFunctionNode(root);
		vasm_stream.close();
		return res;
	}
}