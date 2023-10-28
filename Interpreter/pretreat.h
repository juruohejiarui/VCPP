#pragma once

#include "gloconst.h"
namespace Interpreter {
	enum class TokenType {
		Empty,
		Identifier = 1, ExpressionEnd,
		Integer, Float, String, Char,
		SBracketL, SBracketR, MBracketL, MBracketR, LBracketL, LBracketR,
		//关键字
		VarDefine, FuncDefine, ClassDefine, NamespaceDefine,
		If, Else, Switch, Case, While, For, Continue, Break, Return, 
		Private, Protected, Public, Super, Using, 
		//运算符（除去[])
		Comma, Assign,
		Add, Minus, Mul, Divison, Mod, BitAnd, BitOr, BitXor, BitNot, Lmov, Rmov,
		Equ, Neq, Gt, Ge, Ls, Le,
		LogicAnd, LogicOr, LogicNot,
		CallMember, New, As, Region, PInc, SInc, PDec, SDec, ArrIndex, /*这个ArrIndex只是为了和CplNodeType对齐*/
		Expose, Extern, VasmRegion, Glomem, StringRegion, Rely,
		//下列内容为汇编模式的标志
		label = 1024,
		vbmov, vi32mov, vi64mov, vfmov, vomov, mbmov, mi32mov, mi64mov, mfmov, momov,
		add, sub, mul, _div, mod, ladd, lsub, lmul, _ldiv, lmod, fadd, fsub, fmul, fdiv, uadd, usub, umul, udiv, umod, badd, bsub, bmul, bdiv, bmod,
		eq, ne, gt, ge, ls, le, feq, fne, fgt, fge, fls, fle,
		_and, _or, _xor, _not, lmv, rmv, land, lor, lxor, lnot, llmv, lrmv, uand, uor, uxor, unot, ulmv, urmv, band, bor, bxor, bnot, blmv, brmv,
		ret, opop, pop,
		vbgvl, vi32gvl, vi64gvl, vfgvl, vogvl, mbgvl, mi32gvl, mi64gvl, mfgvl, mogvl,
		push0, push1,
		pvar0, pvar1, pvar2, pvar3, povar0, povar1, povar2, povar3,
		cpy, ocpy, 
		arrmem1, arromem1,
		pack, unpack, 
		_new, jmp, jz, jp,
		setvar,
		poparg, 
		push,
		pvar, povar, pglo, poglo, pstr,
		mem, omem,
		sys,
		arrnew, arrmem, arromem,
		call, ecall, excmd, 
	};
	constexpr TokenType OperTokenTypeStart = TokenType::Comma, OperTokenTypeEnd = TokenType::ArrIndex,
		KeywordTokenTypeStart = TokenType::VarDefine, KeywordTokenTypeEnd = TokenType::Using;
	constexpr int TokenTypeShift = 1024;
	constexpr int VCTokenCount = 64;
	const string TokenTypeStr[] = {
		"Empty",
		"Identifier", "ExpressionEnd",
		"Integer", "Float", "String", "Char",
		"SBracketL", "SBracketR", "MBracketL", "MBracketR", "LBracketL", "LBracketR",
		//关键字
		"VarDefine", "FuncDefine", "ClassDefine", "NamespaceDefine",
		"If", "Else", "Switch", "Case", "While", "For", "Continue", "Break", "Return", 
		"Private", "Protected", "Public", "Super", "Using",
		//运算符（除去[])
		"Comma", "Assign",
		"Add", "Minus", "Mul", "Divison", "Mod", "BitAnd", "BitOr", "BitXor", "BitNot", "Lmov", "Rmov",
		"Equ", "Neq", "Gt", "Ge", "Ls", "Le",
		"LogicAnd", "LogicOr", "LogicNot",
		"CallMember", "New", "As", "Region", "PInc", "SInc", "PDec", "SDec", "ArrIndex", /*这个ArrIndex只是为了和CplNodeType对齐*/
		"Expose", "Extern", "VasmRegion", "Glomem", "StringRegion", "Rely",

		//下列内容为汇编模式的标志
		COMMAND_NAME_LS
	};
	//可以用于设定头文件寻找的路径的根
	extern string Pretreat_HeaderPathRoot;
	struct Token {
		TokenType Type;

		ulong Ulong;
		double Float;
		string String;
		char Char;

		int Line;

		Token();
	};
	void PrintToken(Token& Token);
	
	void Pretreat_Init();
	//生成一个Token序列，不会清空已有的内容
	int Pretreat_GetTokenList(vector<Token>& list, string& str, bool is_asm_style);
}
