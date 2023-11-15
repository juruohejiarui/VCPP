#include "pretreat.h"

namespace Interpreter {
	extern const string TokenTypeStr[];
	string Pretreat_HeaderPathRoot;
	
	void PrintToken(Token& Token) {
		int id = 0;
		if (Token.Type >= TokenType::label) id = (int)Token.Type - (int)TokenType::label + VCTokenCount;
		else id = (int)Token.Type;

		PrintLog(TokenTypeStr[id], FOREGROUND_RED | FOREGROUND_INTENSITY);
		PrintLog(" " + Token.String, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		PrintLog(" " + to_string(Token.Ulong), FOREGROUND_GREEN);

		PrintLog(" Line = ", FOREGROUND_WHITE);
		PrintLog(to_string(Token.Line) + "\n", FOREGROUND_INTENSITY | FOREGROUND_BLUE);
	}

	Token::Token() {
		this->Type = TokenType::Empty;
		this->Char = 0;
		this->Float = 0.0;
		this->String.clear();
		this->Ulong = this->Line = 0;
	}


	inline bool is_number(char ch) { return '0' <= ch && ch <= '9'; }
	inline bool is_letter(char ch) { return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_'; }

	//会对读取到的字符串进行处理
	static int read_str(string &src, int st, int &ed, Token &res) {
		res.Type = TokenType::String; res.String.clear();
		for (ed = st + 1; src[ed] != '\"' && ed < src.size(); ed++)
			res.String.push_back(src[ed]);
		if (ed == src.size()) return -1;
		return 0;
	}

	static Token long_term_token;
	static int CurrentLineId;

	void Pretreat_Init() {
		CurrentLineId = 1;
	}

	int GetAsmTokenList(vector<Token>& list, string& str) {
		string iden;
		Token tk;
		auto is_idench = [](char ch) { return is_letter(ch) || is_number(ch) || ch == '_' ||ch == '@' || ch == '.'; };
		for (int l = 0, r = 0; l < str.size(); l = r) {
			char fir_ch = str[l];
			tk.Type = TokenType::Empty; tk.String.clear(); tk.Ulong = 0, tk.Line = CurrentLineId;
			if (fir_ch == '\n') r++, CurrentLineId++;
			else if (is_idench(fir_ch) && !is_number(fir_ch)) {
				iden.clear();
				for (; r < str.size() && is_idench(str[r]); r++) iden.push_back(str[r]);
				int id = GetCommandIndex(iden);
				if (id == -1) tk.Type = TokenType::Identifier, tk.String = iden;
				else tk.Type = (TokenType)(id + (1 << 10));
			} else if (is_number(fir_ch)) {
				for (; r < str.size() && is_number(str[r]); r++) tk.Ulong = (tk.Ulong << 3) + (tk.Ulong << 1) + (str[r] - '0');
				tk.Type = TokenType::Integer;
			} else if (fir_ch == ';') {
				for (; r < str.size() && str[r] != '\n'; r++);
				r++; 
			} else if (fir_ch == '\"') {
				int status = read_str(str, l, r, tk);
				r++;
				if (status == -1) return -1;
			} else if (fir_ch == '#') { //预处理指令
				iden.clear(); r++;
				for (; r < str.size() && is_idench(str[r]); r++) iden.push_back(str[r]);
				if (iden == "EXPOSE" || iden == "EXTERN") {
					//先设置类型
					tk.Type = (iden == "EXPOSE" ? TokenType::Expose : TokenType::Extern);
					//然后读取一行的标识符
					for (; r < str.size() && str[r] != '\n'; r++) {
						//跳过空格和逗号
						if (str[r] == ' ' || str[r] == ',') continue;

						if (is_number(str[r])) {
							PrintError("Number can't be the first char of a identifier");
							return -1;
						} else if (is_letter(str[r])) { //读取一个标识符
							iden.clear();
							for (; r < str.size() && is_idench(str[r]); r++)
								iden.push_back(str[r]);
							r--;
							tk.String = iden;
							list.emplace_back(tk);
						} else {
							PrintError("Invalid identifier name");
							return -1;
						}
					}
				} else if (iden == "GLOMEM") {
					tk.Type = TokenType::Glomem;
					list.emplace_back(tk);
				} else if (iden == "STRING") {
					tk.Type = TokenType::StringRegion;
					list.emplace_back(tk);
				} else if (iden == "RELY") {
					tk.Type = TokenType::Rely;
					list.emplace_back(tk);
				}
				tk.Type = TokenType::Empty;
			} else r++;
			if (tk.Type != TokenType::Empty) list.emplace_back(tk);
		}
		return 0;
	}

	static string keyword_list[] = {
		"var", "func", "class", "namespace",
		"if", "else", "switch", "case", "while", "for", "continue", "break", "return", 
		"private", "protected", "public", "super", "using", 
	};
	static TokenType GetKeywordToken(string& str) {
		for (int i = 0; i < (int)KeywordTokenTypeEnd - (int)KeywordTokenTypeStart + 1; i++) 
			if (str == keyword_list[i]) return (TokenType)(i + (int)KeywordTokenTypeStart);
		return TokenType::Empty;
	}
	static inline int CharToInt(char ch) { return is_number(ch) ? ch - '0' : ch - 'a'; }

	static int GetVCTokenList(vector<Token> &list, string &str); 

	static int LoadVHead(vector<Token>& list, string path) {
		string str;
		ifstream ifs(path, ios::in);
		if (ifs.is_open()) {
			while (ifs.good()) {
				getline(ifs, str);
				GetVCTokenList(list, str);
			}
		} else {
			PrintError("Couldn't open file : " + path);
			return -1;
		}
		return 0;
	}

	static int GetVCTokenList(vector<Token>& list, string& str) {
		Token tk;
		// ignore the prefix of tab and space
		int st = 0;
		while (str[st] == ' ' || str[st] == '\t') st++;
		for (int l = st, r = st; l < str.size(); l = r) {
			char &fir_ch = str[l];
			tk = Token(); tk.Line = CurrentLineId;
			if (fir_ch == '#') {
				string pt_iden;
				for (r = l + 1; r < str.size() && (is_letter(str[r]) || is_number(str[r])); r++) 
					pt_iden.push_back(str[r]);
				if (pt_iden == "include") {
					//读取一个地址
					while (str[r] != '\"') r++;
					int res = read_str(str, r, r, tk);
					if (res == -1) return -1;
					tk.Type = TokenType::Empty;
					r++;
					if (LoadVHead(list, tk.String) == -1) return -1;
				} else if (pt_iden == "vasmbegin") {
					long_term_token.Type = TokenType::VasmRegion;
					long_term_token.String.clear();
					while (str[r] != '\n' && r < str.size()) r++;
					if (str[r] == '\n') CurrentLineId++;
					r++;
				} else if (pt_iden == "vasmend") {
					tk = long_term_token, long_term_token = Token();
					while (str[r] != '\n' && r < str.size()) r++;
					if (str[r] == '\n') CurrentLineId++;
					r++;
				} else {
					PrintError("Unknown pretreat symbol : #" + pt_iden);
					return -1;
				}
			} else if (long_term_token.Type != TokenType::Empty) {
				//将一行的字符读进去
				if (long_term_token.Type == TokenType::VasmRegion) {
					for (; str[r] != '\n' && r < str.size(); r++) long_term_token.String.push_back(str[r]);
					if (str[r] == '\n') CurrentLineId++;
					long_term_token.String.push_back('\n');
					r++;
				}
				continue;
			} else if (is_letter(fir_ch)) {
				for (; r < str.size() && (is_letter(str[r]) || is_number(str[r])); r++) tk.String.push_back(str[r]);
				TokenType kwtk = GetKeywordToken(tk.String);
				if (kwtk != TokenType::Empty) tk.Type = kwtk;
				else tk.Type = TokenType::Identifier;
			} else if (is_number(fir_ch)) {
				tk.Ulong = 0; tk.Type = TokenType::Integer;
				int shift = 10;
				if (fir_ch == '0') {
					//是一个十六进制
					if (str[l + 1] == 'x') r = l + 2, shift = 16;
					else r = l + 1, shift = 8;
				}
				for (; r < str.size() && (is_number(str[r]) || (str[r] >= 'a' && str[r] <= 'z')); r++)
					tk.Ulong = tk.Ulong * shift + CharToInt(str[r]);
				if (str[r] == '.') {
					tk.Type = TokenType::Float;
					tk.Float = tk.Ulong;
					r++;
					double d_shift = 1.0 / shift;
					for (; r < str.size() && (is_number(str[r]) || (str[r] >= 'a' && str[r] <= 'z')); r++, d_shift /= shift)
						tk.Float += CharToInt(str[r]) * d_shift;
				}
			} else if (fir_ch == '\"') {
				int status = read_str(str, l, r, tk); r++;
				if (status == -1) return -1;
			} else if (fir_ch == '\'') {
				char ch = str[++r];
				tk.Type = TokenType::Char;
				if (ch == '\\') {
					ch = str[++r];
					switch(ch) {
						case 'n': tk.Char = '\n'; break;
						case 'r': tk.Char = '\r'; break;
						case 't': tk.Char = '\t'; break;
						case '0': tk.Char = '\0'; break;
						case 'b': tk.Char = '\b'; break;
						default: tk.Char = ch; break;
					}
				} else tk.Char = ch;
				r += 2;
			} else if (fir_ch == ';') r++, tk.Type = TokenType::ExpressionEnd;
			else if (fir_ch == '!') {
				if (str[l + 1] == '=') r += 2, tk.Type = TokenType::Neq;
				else r++, tk.Type = TokenType::LogicNot;
			} else if (fir_ch == '%') r++, tk.Type = TokenType::Mod;
			else if (fir_ch == '^') r++, tk.Type = TokenType::BitXor;
			else if (fir_ch == '&') {
				if (str[l + 1] == '&') r += 2, tk.Type = TokenType::LogicAnd;
				else r++, tk.Type = TokenType::BitAnd;
			} else if (fir_ch == '*') r++, tk.Type = TokenType::Mul;
			else if (fir_ch == '(') r++, tk.Type = TokenType::SBracketL;
			else if (fir_ch == ')') r++, tk.Type = TokenType::SBracketR;
			else if (fir_ch == '-') {
				if (str[l + 1] == '-') r += 2, tk.Type = TokenType::PDec;
				else r++, tk.Type = TokenType::Minus;
			} else if (fir_ch == '+') {
				if (str[l + 1] == '+') r += 2, tk.Type = TokenType::PInc;
				else r++, tk.Type = TokenType::Add;
			}
			else if (fir_ch == '=') {
				if (str[l + 1] == '=') r += 2, tk.Type = TokenType::Equ;
				else if (str[l + 1] == '>') r += 2, tk.Type = TokenType::As;
				else r++, tk.Type = TokenType::Assign;
			} else if (fir_ch == '{') r++, tk.Type = TokenType::LBracketL;
			else if (fir_ch == '[') r++, tk.Type = TokenType::MBracketL;
			else if (fir_ch == '}') r++, tk.Type = TokenType::LBracketR;
			else if (fir_ch == ']') r++, tk.Type = TokenType::MBracketR;
			else if (fir_ch == '|') {
				if (str[l + 1] == '|') r += 2, tk.Type = TokenType::LogicOr;
				else r++, tk.Type = TokenType::BitOr;
			} else if (fir_ch == '~') r++, tk.Type = TokenType::BitNot;
			else if (fir_ch == '<') {
				if (str[l + 1] == '<') r += 2, tk.Type = TokenType::Lmov;
				else if (str[l + 1] == '=') r += 2, tk.Type = TokenType::Le;
				else r++, tk.Type = TokenType::Ls;
			} else if (fir_ch == '>') {
				if (str[l + 1] == '>') r += 2, tk.Type = TokenType::Rmov;
				else if (str[l + 1] == '=') r += 2, tk.Type = TokenType::Ge;
				else r++, tk.Type = TokenType::Gt;
			} else if (fir_ch == ',') r++, tk.Type = TokenType::Comma;
			else if (fir_ch == '.') r++, tk.Type = TokenType::CallMember;
			else if (fir_ch == '/') r++, tk.Type = TokenType::Divison;
			else if (fir_ch == '$') r++, tk.Type = TokenType::New;
			else if (fir_ch == ':') {
				if (str[l + 1] == ':') r += 2, tk.Type = TokenType::Region;
				else r++, PrintError("Unsupported operator \":\"", CurrentLineId);
			}
			else if (fir_ch == '\n') {
				r++, CurrentLineId++;
				// Skip all the space character following the '\n'
				while (r < str.size() && (str[r] == ' ' || str[r] == '\t')) r++;
			} else r++;
			if (tk.Type != TokenType::Empty) list.emplace_back(tk);
		}
		return 0;
	}

	int Pretreat_GetTokenList(vector<Token>& list, string& str, bool is_asm_style) {
		if (is_asm_style) return GetAsmTokenList(list, str);
		else return GetVCTokenList(list, str);
	}
}