#pragma once

#include "gloconst.h"
#include "pretreat.h"

namespace Interpreter {
	int Vasm_Init(string path);

	int Vasm_BuildOutfile(vector<Token>& tklist);
}
