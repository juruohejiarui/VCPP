 #include "vasm.h"
#include <fstream>


namespace Interpreter {
	static ofstream outfile_stream;
	static map<string, ulong> label_addr;
	static map<string, int> extern_id;
	static vector<string> extern_list, rely_list;

	static map<string, int> expose_id;
	static vector<string> expose_list;
	static vector<ulong> expose_addr;

	static void WriteChar(char ch) {
		outfile_stream.write((char*)&ch, sizeof(char));
	}
	static void WriteUlong(ulong ull) {
		outfile_stream.write((char*)&ull, sizeof(ulong));
	}
	static void WriteString(string& str) {
		outfile_stream.write(str.c_str(), sizeof(char) * (str.size() + 1));
	}

	int Vasm_Init(string outfile_path) {
		outfile_stream.open(outfile_path, ios::out | ios::binary);
		if (outfile_stream.bad()) return -1;
		label_addr.clear();
		extern_id.clear(), extern_list.clear(), expose_id.clear(), expose_list.clear(), expose_addr.clear();
		return 0;
	}

	inline int GetCommandID(TokenType type) {
		return (int)type ^ TokenTypeShift;
	}

	static int GetLabelAddr(string& lbl) {
		if (!label_addr.count(lbl)) return -1;
		return label_addr[lbl];
	}
	static int GetExternID(string& lbl) {
		if (!extern_id.count(lbl)) return -1;
		return extern_id[lbl];
	}
	int Vasm_BuildOutfile(vector<Token>& tklist) {
		vector<string> str; ulong ull = 0, cur_size = 0, glomem = 0;

		//先处理所有的标签和预处理
		for (int i = 0; i < tklist.size(); i++) {
			Token &fir_tk = tklist[i];
			if (fir_tk.Type == TokenType::Extern) {
				//重复，直接跳过
				string &str = fir_tk.String;
				if (!extern_id.count(str)) {
					extern_id[str] = extern_list.size();
					extern_list.emplace_back(str);
				}
			} else if (fir_tk.Type == TokenType::Expose) {
				string &str = fir_tk.String;
				if (!expose_id.count(str)) {
					expose_id[str] = expose_list.size();
					expose_list.emplace_back(str);
				}
			} else if (fir_tk.Type == TokenType::Glomem) {
				if (tklist[i + 1].Type != TokenType::Integer) {
					PrintError("#GLOMEM must be followed by an integer.");
					return -1;
				}
				glomem = tklist[i + 1].Ulong;
				i++;
			} else if (fir_tk.Type == TokenType::StringRegion) {
				i++;
				if (tklist[i].Type != TokenType::String) {
					PrintError("#STRING must be followed by a string.");
					return -1;
				}
				str.emplace_back(tklist[i].String);
			} else if (fir_tk.Type == TokenType::Rely) {
				i++;
				if (tklist[i].Type != TokenType::String) {
					PrintError("#RELY must be followed by a string.");
					return -1;
				}
				rely_list.push_back(tklist[i].String);
			} else if ((int)fir_tk.Type < TokenTypeShift) {
				PrintError("invalid content : " + to_string((int)fir_tk.Type));
				cout << "Token id = " << i << endl;
				printf("This token is : "), PrintToken(tklist[i]);
				printf("The last token is :"), PrintToken(tklist[i - 1]);
				return -1;
			}
			if ((int)fir_tk.Type < TokenTypeShift) continue;
			int cid = GetCommandID(fir_tk.Type), argcnt = GetCommndArgCount(cid);
			Token fir_arg, sec_arg;
			if (argcnt >= 1) fir_arg = tklist[++i];
			if (argcnt >= 2) sec_arg = tklist[++i];

			if (fir_tk.Type == TokenType::label) {
				if (label_addr.count(fir_arg.String)) {
					PrintError("multiple label : " + fir_arg.String);
					return -1;
				}
				label_addr[fir_arg.String] = cur_size;
#ifdef DEBUG_MODULE
				cout << "find label : " << fir_arg.String << endl;
#endif
				continue;
			}
			cur_size += GetCommandSize(cid);
		}
		for (int i = 0; i < expose_list.size(); i++) {
			ulong addr = GetLabelAddr(expose_list[i]);
			if (addr == -1) { PrintError("Undefine expose label : " + expose_list[i]); return -1; }
			expose_addr.emplace_back(addr);
		}

		//先载入除程序之外的所有信息
		WriteUlong(expose_list.size());
		for (int i = 0; i < expose_list.size(); i++)
			WriteString(expose_list[i]), WriteUlong(expose_addr[i]);
		WriteUlong(rely_list.size());
		for (int i = 0; i < rely_list.size(); i++) WriteString(rely_list[i]);
		WriteUlong(extern_list.size());
		for (int i = 0; i < extern_list.size(); i++) WriteString(extern_list[i]);
		WriteUlong(cur_size), WriteUlong(glomem);
		WriteUlong(str.size());
		for (int i = 0; i < str.size(); i++) WriteString(str[i]);
		if (label_addr.count("Main")) WriteChar(1), WriteUlong(label_addr["Main"]);
		else if (label_addr.count("Global@Main")) WriteChar(1), WriteUlong(label_addr["Global@Main"]);
		else WriteChar(0), WriteUlong(0);

		//接着开始载入指令
		cur_size = 0;
		for (int i = 0; i < tklist.size(); i++) {
			Token& fir_tk = tklist[i];
			if (fir_tk.Type == TokenType::Extern || fir_tk.Type == TokenType::Expose) continue;
			if (fir_tk.Type == TokenType::label || fir_tk.Type == TokenType::Glomem || fir_tk.Type == TokenType::StringRegion
				|| fir_tk.Type == TokenType::Rely) { 
				i++; continue; 
			}
			int cid = GetCommandID(fir_tk.Type), argcnt = GetCommndArgCount(cid);
			ulong arg1 = 0, arg2 = 0;
			if (fir_tk.Type >= TokenType::jmp && fir_tk.Type <= TokenType::jp) {
				arg1 = GetLabelAddr(tklist[i + 1].String);
				if (arg1 == -1) { PrintError("Undefined label : " + tklist[i + 1].String); return -1; }
			} else if (fir_tk.Type == TokenType::call) {
				arg1 = GetLabelAddr(tklist[i + 1].String), arg2 = tklist[i + 2].Ulong;
				if (arg1 == -1) { 
					//check if it can be change into "ecall"
					arg1 = GetExternID(tklist[i + 1].String);
					if (arg1 == -1) { PrintError("Undefined label : " + tklist[i + 1].String); return -1; }
					cid = GetCommandID(fir_tk.Type = TokenType::ecall);
				}
			} else if (fir_tk.Type == TokenType::ecall) {
				arg1 = GetExternID(tklist[i + 1].String), arg2 = tklist[i + 2].Ulong;
				if (arg1 == -1) { PrintError("Undefined extern label : " + tklist[i + 1].String); return -1; }
			} else {
				if (argcnt >= 1) arg1 = tklist[i + 1].Ulong;
				if (argcnt >= 2) arg2 = tklist[i + 2].Ulong;
			}
			WriteChar(cid);
			if (argcnt >= 1) WriteUlong(arg1);
			if (argcnt >= 2) WriteUlong(arg2);
			cur_size += GetCommandSize(cid), i += argcnt;
#ifdef DEBUG_MODULE
			static string cmdls[] = { COMMAND_NAME_LS };
			printf("%3llu: %s %llu %llu\n", cur_size - GetCommandSize(cid), cmdls[cid].c_str(), arg1, arg2);
#endif
		}
		outfile_stream.close();
		return 0;
	}
}

