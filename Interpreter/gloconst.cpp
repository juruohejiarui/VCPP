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
		for (int i = 0; i <= CMD0_COUNT + CMD1_COUNT + CMD2_COUNT; i++) if (cmdnamels[i] == cmd_name) return i;
		return -1;
	}
	int GetCommndArgCount(int cmdid) {
		if (cmdid == 0) return 1;
		else if (CMD0_COUNT >= cmdid) return 0;
		else if (CMD0_COUNT + CMD1_COUNT >= cmdid) return 1;
		else if (CMD0_COUNT + CMD1_COUNT + CMD2_COUNT >= cmdid) return 2;
		else return 3;
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
