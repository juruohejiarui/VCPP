#pragma once
#include "gloconst.h"
#include "vcppcpl.h"

namespace Interpreter {
	struct OutFileInfo {
		string Path;
		ulong ExposeCount;
		pair<string, ulong>* ExposeContent = nullptr;
		ulong RelyCount;
		string* RelyContent = nullptr;
		ulong ExternCount;
		string* ExternContent = nullptr;
		ulong ExecSize;
		ulong GlomemSize;
		ulong StringCount;
		string* StringContent = nullptr;
		vbyte HasMain;
		ulong MainAddr;
		vbyte* ExecContent = nullptr;
		ulong NodeCount;
		CplNode** Nodes = nullptr;
		~OutFileInfo();
	};
	struct VLibraryInfo : OutFileInfo {
		string DefinitionContent;

		//If the project directly relies on this VLibrary
		bool IsDirectRely;
	};

	void ProjectBuilder_Init();

	//Load the vc file build a CplTree of definition, and update the idensystem
	int ProjectBuilder_LoadVcpp(string &path);
	//Load the vlib file, build a CplTree of declaration, and update the idensystem
	int ProjectBuilder_LoadVlib(string &path, bool is_direct = false);
	//Load the vo file
	int ProjectBuilder_LoadOutFile(string &path);
	//Load the vdef file, build a CplTree of declaration, and update the idensystem
	int ProjectBuilder_LoadVdef(string &path);
	
	int ProjectBuilder_BuildVexe(string &path);

	int ProjectBuilder_BuildVlib(string &path);
} 