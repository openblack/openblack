#include "LandBlock.h"

#include <stdio.h>

using namespace OpenBlack;

LandBlock::LandBlock()
{

}

LandBlock::~LandBlock()
{
}

void LandBlock::Draw() {
	printf("block %d\n", this->index);
}
