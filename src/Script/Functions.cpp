#include "Functions.h"

using namespace OpenBlack::Script;

Functions::FunctionPointer* Functions::GetFunctionFromName(std::string name)
{
	for (int i = 0; i < 105; i++) {
		auto func = Functions::kLandFunctionPointers[i];
		if (func.name == name) {
			return &func;
		}
	}

	return nullptr;
}
