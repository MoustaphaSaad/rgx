#pragma once

#include "rgx/Exports.h"

#include <mn/Buf.h>
#include <mn/Str.h>

#include <stdint.h>

namespace rgx
{
	RGX_EXPORT mn::Buf<uint8_t>
	compile(const mn::Str& expr, mn::Allocator allocator = mn::allocator_top());

	inline static mn::Buf<uint8_t>
	compile(const char* expr, mn::Allocator allocator = mn::allocator_top())
	{
		return compile(mn::str_lit(expr), allocator);
	}
}