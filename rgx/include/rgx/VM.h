#pragma once

#include "rgx/Exports.h"

#include <mn/Buf.h>

#include <stdint.h>

namespace rgx
{
	struct Result
	{
		const char* it;
		bool matched;
	};

	RGX_EXPORT Result
	match(const mn::Buf<uint8_t>& bytes, const char* str);
}