#pragma once

#include <stdint.h>

namespace rgx
{
	enum ISA: uint8_t
	{
		//matches a single byte
		//ISA_BYTE, 'a'
		ISA_BYTE,
		//matches multiple bytes
		//ISA_MATCH, 7, 'm', 'o', 's', 't', 'a', 'f', 'a'
		ISA_MATCH,
		//matchs any single byte
		//ISA_ANY
		ISA_ANY,
		//splits the execution into 2 threads for or operations
		//ISA_SPLIT, offset1(4byte), offset2(4byte)
		ISA_SPLIT,
		//changes the ip to ip + offset(4byte)
		//ISA_JUMP, offset
		ISA_JUMP,
		//matches a set operator
		//ISA_SET, count(4byte), Component(ISA_RANGE|ISA_BYTE)
		ISA_SET,
		ISA_NSET, //negative set check if letter is not in set
		//Specify a range of bytes
		//ISA_RANGE, 'a', 'z'
		ISA_RANGE,
		//stops the execution with success
		ISA_HALT,
	};
}