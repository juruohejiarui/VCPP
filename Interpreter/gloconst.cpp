#include "gloconst.h"
#ifdef PLATFORM_WINDOWS
#include "windows.h"
#endif

static string cmdnamels[] = {
	COMMAND_NAME_LS
};

namespace Interpreter {

	int GetCommandSize(int id) {
		if (!id) return 0;
		if (id <= CMD0_COUNT) return 1;
		else if (id <= CMD0_COUNT + CMD1_COUNT) return 9;
		else if (id <= CMD0_COUNT + CMD1_COUNT + CMD2_COUNT) return 17;
		return -1;
	}
#ifdef PLATFORM_WINDOWS
    void PrintError(string info, int line) {
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		WORD wOldColorAttrs;
		CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
		GetConsoleScreenBufferInfo(h, &csbiInfo);
		wOldColorAttrs = csbiInfo.wAttributes;
		
		if (line) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_WHITE);
			cout << "Line " << line << ": ";
		}
		
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED);
        cout << info << endl;
		SetConsoleTextAttribute(h, wOldColorAttrs);
        return;
    }

	void PrintLog(string info, WORD foreground_color) {
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		WORD wOldColorAttrs;
		CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

		GetConsoleScreenBufferInfo(h, &csbiInfo);
		wOldColorAttrs = csbiInfo.wAttributes;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), foreground_color);
		cout << info;
		SetConsoleTextAttribute(h, wOldColorAttrs);
	}
#endif
#ifdef PLATFORM_MACOS
	void SetColor(short foreground_color) {
		#ifndef DEBUG_FILESTYLE
		if (foreground_color & 0x08) printf("\e[1m");
		printf("\e[3%cm", (foreground_color & 0x07) + '0');
		#endif
	} 
	void CleanColor() {
		#ifndef DEBUG_FILESTYLE
		printf("\e[0m");
		#endif
	}
	void PrintError(string info, int line) {
		if (line) {
			SetColor(FOREGROUND_WHITE);
			printf("Line ");
			SetColor(FOREGROUND_RED);
			printf("%d ", line);
			SetColor(FOREGROUND_WHITE);
		}
		SetColor(FOREGROUND_RED);
		printf("%s\n", info.c_str());
		CleanColor();
		
	}
	void PrintLog(string info, short foreground_color) {
		SetColor(foreground_color);
		cout << info;
		CleanColor();
	}
#endif

	int GetCommandIndex(string cmd_name) {
		for (int i = 0; i <= CMD0_COUNT + CMD1_COUNT + CMD2_COUNT + 1; i++) if (cmdnamels[i] == cmd_name) return i;
		return -1;
	}
	int GetCommndArgCount(int cmdid) {
		if (cmdid == 0) return 1;
		else if (CMD0_COUNT >= cmdid) return 0;
		else if (CMD0_COUNT + CMD1_COUNT >= cmdid) return 1;
		else if (CMD0_COUNT + CMD1_COUNT + CMD2_COUNT >= cmdid) return 2;
		else return 3;
	}

	const string EXCommandName[] = {
		"EX_none",
		//++ and --
		"EX_vbpinc", "EX_vi32pinc", "EX_vi64pinc", "EX_vupinc", "EX_vbsinc", "EX_vi32sinc", "EX_vi64sinc", "EX_vusinc", 
		"EX_vbpdec", "EX_vi32pdec", "EX_vi64pdec", "EX_vupdec", "EX_vbsdec", "EX_vi32sdec", "EX_vi64sdec", "EX_vusdec", 
		"EX_mbpinc", "EX_mi32pinc", "EX_mi64pinc", "EX_mupinc", "EX_mbsinc", "EX_mi32sinc", "EX_mi64sinc", "EX_musinc", 
		"EX_mbpdec", "EX_mi32pdec", "EX_mi64pdec", "EX_mupdec", "EX_mbsdec", "EX_mi32sdec", "EX_mi64sdec", "EX_musdec", 

		//+= -= *= /= %= &= |= ^= <<= >>=
		"EX_vbaddmov", "EX_vaddmov", "EX_vladdmov", "EX_vuaddmov", "EX_vfaddmov", 
		"EX_mbaddmov", "EX_maddmov", "EX_mladdmov", "EX_muaddmov", "EX_mfaddmov", 
		"EX_vbsubmov", "EX_vsubmov", "EX_vlsubmov", "EX_vusubmov", "EX_vfsubmov", 
		"EX_mbsubmov", "EX_msubmov", "EX_mlsubmov", "EX_musubmov", "EX_mfsubmov", 
		"EX_vbmulmov", "EX_vmulmov", "EX_vlmulmov", "EX_vumulmov", "EX_vfmulmov", 
		"EX_mbmulmov", "EX_mmulmov", "EX_mlmulmov", "EX_mumulmov", "EX_mfmulmov", 
		"EX_vbdivmov", "EX_vdivmov", "EX_vldivmov", "EX_vudivmov", "EX_vfdivmov", 
		"EX_mbdivmov", "EX_mdivmov", "EX_mldivmov", "EX_mudivmov", "EX_mfdivmov", 
		"EX_vbmodmov", "EX_vmodmov", "EX_vlmodmov", "EX_vumodmov", 
		"EX_mbmodmov", "EX_mmodmov", "EX_mlmodmov", "EX_mumodmov", 
		"EX_vbandmov", "EX_vandmov", "EX_vlandmov", "EX_vuandmov", 
		"EX_mbandmov", "EX_mandmov", "EX_mlandmov", "EX_muandmov", 
		"EX_vbormov", "EX_vormov", "EX_vlormov", "EX_vuormov", 
		"EX_mbormov", "EX_mormov", "EX_mlormov", "EX_muormov", 
		"EX_vbxormov", "EX_vxormov", "EX_vlxormov", "EX_vuxormov", 
		"EX_mbxormov", "EX_mxormov", "EX_mlxormov", "EX_muxormov", 
		"EX_vblmvmov", "EX_vlmvmov", "EX_vllmvmov", "EX_vulmvmov", 
		"EX_mblmvmov", "EX_mlmvmov", "EX_mllmvmov", "EX_mulmvmov", 
		"EX_vbrmvmov", "EX_vrmvmov", "EX_vlrmvmov", "EX_vurmvmov", 
		"EX_mbrmvmov", "EX_mrmvmov", "EX_mlrmvmov", "EX_murmvmov", 
		// vector operator
		"EX_newvec2", "EX_newvec3", "EX_newvec4", 
		"EX_vvec2mov", "EX_vvec3mov", "EX_vvec4mov", "EX_mvec2mov", "EX_mvec3mov", "EX_mvec4mov", 
		"EX_vvec2addmov", "EX_vvec3addmov", "EX_vvec4addmov", "EX_mvec2addmov", "EX_mvvec3addmov", "EX_mvec4addmov", 
		"EX_vvec2submov", "EX_vvec3submov", "EX_vvec4submov", "EX_mvec2submov", "EX_mvvec3submov", "EX_mvec4submov", 

		"EX_vec2add", "EX_vec3add", "EX_vec4add", "EX_vec2sub", "EX_vec3sub", "EX_vec4sub", 
		"EX_vec2pmul", "EX_vec3pmul", "EX_vec4pmul", "EX_vec2smul", "EX_vec3smul", "EX_vec4smul", 
		"EX_vec2div", "EX_vec3div", "EX_vec4div", 
		"EX_vec2len", "EX_vec3len", "EX_vec4len", 

		"EX_switch", 
		
		// debug operator
		"EX_hint_var", "EX_hint_code", 
	};

    EXCommand GetEXCommandIndex(string excmd_name) {
		for (int i = 1; i <= EXCMD0_COUNT + EXCMD1_COUNT + EXCMDX_COUNT; i++)
			if (excmd_name == EXCommandName[i]) return (EXCommand)i;
		return EX_none;
    }

    int GetEXCommandSize(int id) {
        if (id <= EXCMD0_COUNT) return sizeof(ulong);
		else if (id <= EXCMD1_COUNT) return sizeof(ulong) << 1;
		else return -1;
    }

    int GetEXCommandArgCount(int id) {
        if (id <= EXCMD0_COUNT) return 0;
		else if (id <= EXCMD1_COUNT) return 1;
		else return -1;
    }

    //split the function's name
	vector<string> FuncNameSplit(string name) {
		int pos = name.find('.');
		if (pos == -1) pos = name.size();
		vector<string> path = StringSplit(name.substr(0, pos), '@');
		path[path.size() - 1].append(name.substr(pos));
		return path;
	}
	
	vector<string> StringSplit(const string &s, const char &delim = ' ') {
		vector<string> tokens;
		size_t lastPos = s.find_first_not_of(delim, 0);
		size_t pos = s.find(delim, lastPos);
		while (lastPos != string::npos) {
			tokens.emplace_back(s.substr(lastPos, pos - lastPos));
			lastPos = s.find_first_not_of(delim, pos);
			pos = s.find(delim, lastPos);
		}
		return tokens;
	}
    void Write(ofstream &ofs, vbyte data) { ofs.write((char*)&data, sizeof(vbyte)); }
    void Write(ofstream &ofs, ulong data) { ofs.write((char*)&data, sizeof(ulong)); }
    void Write(ofstream &ofs, const string &data) { ofs.write(data.c_str(), sizeof(char) * (data.size() + 1)); }
	vbyte ReadByte(ifstream& ifs) {
		vbyte data;
		ifs.read((char*)&data, sizeof(vbyte));
		return data;
	}
	ulong ReadUlong(ifstream& ifs) {
		ulong data;
		ifs.read((char*)&data, sizeof(ulong));
		return data;
	}
	string ReadString(ifstream& ifs) {
		string str; char ch;
		for (ifs.read(&ch, sizeof(char)); ch != '\0'; ifs.read(&ch, sizeof(char))) str.push_back(ch);
		return str;
	}
}
