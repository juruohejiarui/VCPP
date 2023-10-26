#pragma once
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <thread>
#include <stack>
#include <queue>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <fstream>
using namespace std;

// #define DEBUG_MODULE
// #define VM_DEBUG_MODULE
// #define VM_DEBUG_MODULE_BREAK
// #define PLATFORM_WINDOWS
#define PLATFORM_MACOS

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif
// #define DEBUG_FILESTYLE


constexpr int MACHINE_BIT = 8;

constexpr int CMD0_COUNT = 100, CMD1_COUNT = 17, CMD2_COUNT = 2;

#define COMMAND_NAME_LS "label", \
"vbmov", "vi32mov", "vi64mov", "vfmov", "vomov", "mbmov", "mi32mov", "mi64mov", "mfmov", "momov", \
"add", "sub", "mul", "div", "mod", "ladd", "lsub", "lmul", "ldiv", "lmod", "fadd", "fsub", "fmul", "fdiv", "uadd", "usub", "umul", "udiv", "umod", "badd", "bsub", "bmul", "bdiv", "bmod", \
"eq", "ne", "gt", "ge", "ls", "le", "feq", "fne", "fgt", "fge", "fls", "fle", \
"and", "or", "xor", "not", "lmv", "rmv", "land", "lor", "lxor", "lnot", "llmv", "lrmv", "uand", "uor", "uxor", "unot", "ulmv", "urmv", "band", "bor", "bxor", "bnot", "blmv", "brmv",\
"ret", "opop", "pop", \
"vbgvl", "vi32gvl", "vi64gvl", "vfgvl", "vogvl", "mbgvl", "mi32gvl", "mi64gvl", "mfgvl", "mogvl", \
"push0", "push1", \
"pvar0", "pvar1", "pvar2", "pvar3", "povar0", "povar1", "povar2", "povar3", \
"cpy", "ocpy", \
"arrmem1", "arromem1",\
"pack", "unpack", \
"new", "jmp", "jz", "jp", \
"setvar", \
"poparg", \
"push", \
"pvar", "povar", "pglo", "poglo", "pstr",\
"mem", "omem", \
"sys", \
"arrnew", "arrmem", "arromem", \
"call", "ecall"


namespace Interpreter {
	enum commandid {
		label,
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
		_new, 
		jmp, jz, jp,
		setvar,
		poparg, 
		push,
		pvar, povar, pglo, poglo, pstr,
		mem, omem, 
		sys,
		arrnew, arrmem, arromem,
		call, ecall
	};

	typedef unsigned long long ulong;
	typedef unsigned char vbyte;
#ifdef PLATFORM_WINDOWS
#define FOREGROUND_WHITE 0x07
	void PrintError(string info, int line = 0);
	void PrintLog(string info, WORD foreground_color = FOREGROUND_INTENSITY | FOREGROUND_BLUE);
#endif
#ifdef PLATFORM_MACOS

#define FOREGROUND_RED 0x01
#define FOREGROUND_GREEN 0x02
#define FOREGROUND_BLUE 0x04
#define FOREGROUND_WHITE 0x07
#define FOREGROUND_INTENSITY 0x08
	void PrintError(string info, int line = 0);
	void PrintLog(string info, short foreground_color = FOREGROUND_INTENSITY | FOREGROUND_BLUE);

#endif
	int GetCommandSize(int id);
	int GetCommandIndex(string cmd_name);
	int GetCommndArgCount(int cmdid);

	vector<string> FuncNameSplit(string name);
	vector<string> StringSplit(const string& str, const char& delimiter);

	void Write(ofstream& ofs, vbyte data);
	void Write(ofstream& ofs, ulong data);
	void Write(ofstream& ofs, const string& data);
	vbyte ReadByte(ifstream& ifs);
	ulong ReadUlong(ifstream& ifs);
	string ReadString(ifstream& ifs);
}

