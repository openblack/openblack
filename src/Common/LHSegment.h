#pragma once
#ifndef OPENBLACK_LHSEGMENT_H
#define OPENBLACK_LHSEGMENT_H

#include <stdint.h>

namespace OpenBlack
{
	struct LHSegment
	{
		char Name[32];
		uint32_t SegmentSize;
		void* SegmentData;
	};
}

#endif
