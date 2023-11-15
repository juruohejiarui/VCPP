#pragma once
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// #define DEBUG_MODULE
// #define VM_DEBUG_MODULE
// #define VM_DEBUG_MODULE_BREAK
// #define PLATFORM_WINDOWS
#define PLATFORM_MACOS

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif
// #define DEBUG_FILESTYLE

#define MACHINE_BIT 8
#define CMD0_COUNT 100
#define CMD1_COUNT 17
#define CMD2_COUNT 2

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
"call", "ecall", "excmd"

enum CommandID {
	label,
	vbmov, vi32mov, vi64mov, vfmov, vomov, mbmov, mi32mov, mi64mov, mfmov, momov,
	add, sub, mul, _div, mod, ladd, lsub, lmul, _ldiv, lmod, fadd, fsub, fmul, fdiv, uadd, usub, umul, udiv, umod, badd, bsub, bmul, bdiv, bmod,
	eq, ne, gt, ge, ls, le, feq, fne, fgt, fge, _fls, fle,
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
	call, ecall, excmd, 
};

enum EXCommandID {
	EX_none, 
	//++ and --
	EX_vbpinc, EX_vi32pinc, EX_vi64pinc, EX_vupinc, EX_vbsinc, EX_vi32sinc, EX_vi64sinc, EX_vusinc,
	EX_vbpdec, EX_vi32pdec, EX_vi64pdec, EX_vupdec, EX_vbsdec, EX_vi32sdec, EX_vi64sdec, EX_vusdec,
	EX_mbpinc, EX_mi32pinc, EX_mi64pinc, EX_mupinc, EX_mbsinc, EX_mi32sinc, EX_mi64sinc, EX_musinc,
	EX_mbpdec, EX_mi32pdec, EX_mi64pdec, EX_mupdec, EX_mbsdec, EX_mi32sdec, EX_mi64sdec, EX_musdec,

	// big number
	EX_iadd, EX_isub, EX_imul, EX_idiv, EX_imod, EX_iand, EX_ior, EX_ixor, EX_inot, EX_ilmv, EX_irmv,
	EX_itof,

	//+= -= *= /= %= &= |= ^= <<= >>=
	EX_vbaddmov, EX_vaddmov, EX_vladdmov, EX_vuaddmov, EX_vfaddmov, 
	EX_mbaddmov, EX_maddmov, EX_mladdmov, EX_muaddmov, EX_mfaddmov, 
	EX_vbsubmov, EX_vsubmov, EX_vlsubmov, EX_vusubmov, EX_vfsubmov, 
	EX_mbsubmov, EX_msubmov, EX_mlsubmov, EX_musubmov, EX_mfsubmov,
	EX_vbmulmov, EX_vmulmov, EX_vlmulmov, EX_vumulmov, EX_vfmulmov, 
	EX_mbmulmov, EX_mmulmov, EX_mlmulmov, EX_mumulmov, EX_mfmulmov,
	EX_vbdivmov, EX_vdivmov, EX_vldivmov, EX_vudivmov, EX_vfdivmov, 
	EX_mbdivmov, EX_mdivmov, EX_mldivmov, EX_mudivmov, EX_mfdivmov,
	EX_vbmodmov, EX_vmodmov, EX_vlmodmov, EX_vumodmov, 
	EX_mbmodmov, EX_mmodmov, EX_mlmodmov, EX_mumodmov,
	EX_vbandmov, EX_vandmov, EX_vlandmov, EX_vuandmov, 
	EX_mbandmov, EX_mandmov, EX_mlandmov, EX_muandmov,
	EX_vbormov, EX_vormov, EX_vlormov, EX_vuormov, 
	EX_mbormov, EX_mormov, EX_mlormov, EX_muormov,
	EX_vbxormov, EX_vxormov, EX_vlxormov, EX_vuxormov, 
	EX_mbxormov, EX_mxormov, EX_mlxormov, EX_muxormov,
	EX_vblmvmov, EX_vlmvmov, EX_vllmvmov, EX_vulmvmov, 
	EX_mblmvmov, EX_mlmvmov, EX_mllmvmov, EX_mulmvmov,
	EX_vbrmvmov, EX_vrmvmov, EX_vlrmvmov, EX_vurmvmov,
	EX_mbrmvmov, EX_mrmvmov, EX_mlrmvmov, EX_murmvmov,

	EX_switch,

	// vector operator
	EX_newvec2, EX_newvec3, EX_newvec4, 
	EX_vvec2mov, EX_vvec3mov, EX_vvec4mov, EX_mvec2mov, EX_mvec3mov, EX_mvec4mov,
	EX_vvec2addmov, EX_vvec3addmov, EX_vvec4addmov, EX_mvec2addmov, EX_mvvec3addmov, EX_mvec4addmov, 
	EX_vvec2submov, EX_vvec3submov, EX_vvec4submov, EX_mvec2submov, EX_mvvec3submov, EX_mvec4submov, 

	EX_vec2add, EX_vec3add, EX_vec4add, EX_vec2sub, EX_vec3sub, EX_vec4sub,
	EX_vec2pmul, EX_vec3pmul, EX_vec4pmul, EX_vec2smul, EX_vec3smul, EX_vec4smul,
	EX_vec2div, EX_vec3div, EX_vec4div, 
	EX_vec2len, EX_vec3len, EX_vec4len, 

	// debug operator
	EX_hint_var, EX_hint_code, 
	
};

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
void PrintError(char* info, int line);
void PrintLog(char* info, short foreground_color);
#endif
void InitCommandInfo();
int GetCommandSize(int id);
int GetCommandArgCount(int cmdid);

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) < (b) ? (b) : (a))

uint64_t StringToUint64(char *str);
