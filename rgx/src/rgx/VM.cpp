#include "rgx/VM.h"
#include "rgx/ISA.h"

#include <mn/Defer.h>

namespace rgx
{
	using namespace mn;

	struct Thread
	{
		size_t ip;
		const char* dp;
	};

	inline static uint8_t
	pop_byte(const Buf<uint8_t>& bytes, size_t& ip)
	{
		return bytes[ip++];
	}

	inline static int
	pop_int(const Buf<uint8_t>& bytes, size_t& ip)
	{
		assert(ip + sizeof(int) <= bytes.count);
		int res = *(int*)(bytes.ptr+ip);
		ip += sizeof(int);
		return res;
	}


	//API
	Result
	match(const Buf<uint8_t>& bytes, const char* str)
	{
		auto threads = buf_new<Thread>();
		mn_defer(buf_free(threads));

		buf_push(threads, Thread{0, str});
		for(;buf_empty(threads) == false;)
		{
			bool halted = false;
			Thread t = buf_top(threads);
			buf_pop(threads);

			for(;;)
			{
				if(t.ip == bytes.count)
					break;

				auto op = ISA(pop_byte(bytes, t.ip));
				if(op == ISA_BYTE)
				{
					if(t.dp == nullptr || *t.dp != char(pop_byte(bytes, t.ip)))
						break;
					++t.dp;
				}
				else if(op == ISA_ANY)
				{
					if(t.dp == nullptr)
						break;
					++t.dp;
				}
				else if(op == ISA_SPLIT)
				{
					int offset1 = pop_int(bytes, t.ip);
					int offset2 = pop_int(bytes, t.ip);

					Thread t2 = t;
					t2.ip += offset2;

					t.ip += offset1;
					buf_push(threads, t2);
				}
				else if(op == ISA_JUMP)
				{
					t.ip += pop_int(bytes, t.ip);
				}
				else if(op == ISA_SET || op == ISA_NSET)
				{
					int count = pop_int(bytes, t.ip);
					bool match = false;
					for(size_t i = 0; i < size_t(count); ++i)
					{
						auto local_op = ISA(pop_byte(bytes, t.ip));
						if(local_op == ISA_RANGE)
						{
							char a = char(pop_byte(bytes, t.ip));
							char z = char(pop_byte(bytes, t.ip));
							match |= (*t.dp >= a && *t.dp <= z);
						}
						else
						{
							match |= *t.dp == local_op;
						}
					}
					if(op == ISA_SET)
					{
						if(match) ++t.dp;
						else break;
					}
					else if(op == ISA_NSET)
					{
						if(match == false) ++t.dp;
						else break;
					}
				}
				else if(op == ISA_HALT)
				{
					halted = true;
					break;
				}
			}

			if (halted)
				return { t.dp, true };
		}

		return { str, false };
	}
}