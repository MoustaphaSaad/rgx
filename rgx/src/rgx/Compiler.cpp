#include "rgx/Compiler.h"
#include "rgx/ISA.h"

#include <mn/Defer.h>

#include <assert.h>

namespace rgx
{
	using namespace mn;

	enum OPERATOR
	{
		//this operators are ordered by presedence from lower to higher
		OPERATOR_OR = 0,
		OPERATOR_CONCAT = 1,
		OPERATOR_PLUS = 2,
		OPERATOR_PLUS_NON_GREEDY = 3,
		OPERATOR_STAR = 4,
		OPERATOR_STAR_NON_GREEDY = 5,
		OPERATOR_OPTIONAL = 6,
		OPERATOR_OPTIONAL_NON_GREEDY = 7,
		OPERATOR_OPEN_PAREN = -1,
		OPERATOR_CLOSE_PAREN = -2,
	};

	struct Compiler
	{
		Buf<Buf<uint8_t>> operands;
		Buf<OPERATOR> operators;

		bool ignore;
		bool recommend_concat;
	};

	inline static Compiler
	compiler_new()
	{
		Compiler self{};
		self.operands = buf_new<Buf<uint8_t>>();
		self.operators = buf_new<OPERATOR>();
		return self;
	}

	inline static void
	compiler_free(Compiler& self)
	{
		destruct(self.operands);
		buf_free(self.operators);
	}

	inline static void
	destruct(Compiler& self)
	{
		compiler_free(self);
	}

	union Int
	{
		int oint;
		uint8_t obytes[sizeof(int)];
	};

	inline static void
	push_int(Buf<uint8_t>& self, int offset)
	{
		Int v{};
		v.oint = offset;
		for(size_t i = 0; i < sizeof(int); ++i)
			buf_push(self, v.obytes[i]);
	}

	inline static void
	patch_int_at(Buf<uint8_t>& self, size_t ix, int offset)
	{
		Int v{};
		v.oint = offset;
		for(size_t i = 0; i < sizeof(int); ++i)
			self[i + ix] = v.obytes[i];
	}

	inline static bool
	compiler_concat(Compiler& self)
	{
		if(self.operands.count < 2)
			return false;

		auto B = buf_top(self.operands);
		mn_defer(buf_free(B));
		buf_pop(self.operands);

		auto& A = buf_top(self.operands);
		buf_concat(A, B);
		return true;
	}

	inline static bool
	compiler_or(Compiler& self)
	{
		if(self.operands.count < 2)
			return false;

		auto B = buf_top(self.operands);
		mn_defer(buf_free(B));
		buf_pop(self.operands);

		auto A = buf_top(self.operands);
		mn_defer(buf_free(A));
		buf_pop(self.operands);

		auto C = buf_with_capacity<uint8_t>(B.count + A.count + 14);
		buf_push(C, ISA_SPLIT);
		push_int(C, 0);
		//+5 for the jump instruction
		push_int(C, int(A.count + 5));

		buf_concat(C, A);

		buf_push(C, ISA_JUMP);
		push_int(C, int(B.count));

		buf_concat(C, B);

		buf_push(self.operands, C);
		return true;
	}

	inline static bool
	compiler_star(Compiler& self, bool greedy)
	{
		if(self.operands.count < 1)
			return false;

		auto A = buf_top(self.operands);
		mn_defer(buf_free(A));
		buf_pop(self.operands);

		auto C = buf_with_capacity<uint8_t>(A.count + 14);
		buf_push(C, ISA_SPLIT);
		if(greedy)
		{
			push_int(C, 0);
			//+5 for the jump instruction
			push_int(C, int(A.count + 5));
		}
		else
		{
			//+5 for the jump instruction
			push_int(C, int(A.count + 5));
			push_int(C, 0);
		}
		

		buf_concat(C, A);

		buf_push(C, ISA_JUMP);
		//+14 to go back to split
		push_int(C, -1 * int(A.count + 14));

		buf_push(self.operands, C);
		return true;
	}

	inline static bool
	compiler_plus(Compiler& self, bool greedy)
	{
		if(self.operands.count < 1)
			return false;

		auto& A = buf_top(self.operands);
		int offset = -1 * int(A.count + 9);

		buf_push(A, ISA_SPLIT);
		if(greedy)
		{
			//+9 for the split instruction
			push_int(A, offset);
			push_int(A, 0);
		}
		else
		{
			push_int(A, 0);
			//+9 for the split instruction
			push_int(A, offset);
		}
		return true;
	}

	inline static bool
	compiler_optional(Compiler& self, bool greedy)
	{
		if(self.operands.count < 1)
			return false;

		auto A = buf_top(self.operands);
		mn_defer(buf_free(A));
		buf_pop(self.operands);

		auto C = buf_with_capacity<uint8_t>(A.count + 9);

		buf_push(C, ISA_SPLIT);
		if(greedy)
		{
			push_int(C, 0);
			push_int(C, int(A.count));
		}
		else
		{
			push_int(C, int(A.count));
			push_int(C, 0);
		}

		buf_concat(C, A);

		buf_push(self.operands, C);
		return true;
	}

	inline static bool
	compiler_eval(Compiler& self)
	{
		if(buf_empty(self.operators))
			return false;

		OPERATOR op = buf_top(self.operators);
		buf_pop(self.operators);

		switch(op)
		{
		case OPERATOR_CONCAT: return compiler_concat(self);
		case OPERATOR_OR: return compiler_or(self);
		case OPERATOR_STAR: return compiler_star(self, true);
		case OPERATOR_STAR_NON_GREEDY: return compiler_star(self, false);
		case OPERATOR_PLUS: return compiler_plus(self, true);
		case OPERATOR_PLUS_NON_GREEDY: return compiler_plus(self, false);
		case OPERATOR_OPTIONAL: return compiler_optional(self, true);
		case OPERATOR_OPTIONAL_NON_GREEDY: return compiler_optional(self, false);
		default: return false;
		}
	}

	inline static bool
	compiler_op(Compiler& self, OPERATOR op)
	{
		for(;buf_empty(self.operators) == false && buf_top(self.operators) >= op;)
			if(compiler_eval(self) == false)
				return false;
		buf_push(self.operators, op);
		return true;
	}

	inline static bool
	compiler_maybe_concat(Compiler& self)
	{
		if(self.recommend_concat)
		{
			if(compiler_op(self, OPERATOR_CONCAT) == false)
				return false;
			self.recommend_concat = false;
		}
		return true;
	}

	inline static bool
	compiler_run(Compiler& self, const Str& expr)
	{
		for(auto it = begin(expr); it != end(expr); ++it)
		{
			char c = *it;
			
			//if found \ and we are not in ignore state then set the ignore flag
			if(c == '\\' && self.ignore == false)
			{
				self.ignore = true;
			}
			//if found | and we are not in ignore state then add the or op
			else if(c == '|' && self.ignore == false)
			{
				if(compiler_op(self, OPERATOR_OR) == false)
					return false;
				
				self.ignore = false;
				self.recommend_concat = false;
			}
			//if found * and we are not in ignore state then add star op
			else if(c == '*' && self.ignore == false)
			{
				OPERATOR op = OPERATOR_STAR;
				if(it + 1 != end(expr) && it[1] == '?')
				{
					op = OPERATOR_STAR_NON_GREEDY;
					++it;
				}

				if(compiler_op(self, op) == false)
					return false;

				self.ignore = false;
				self.recommend_concat = true;
			}
			//if found + and we are not in ignore state then add plus op
			else if(c == '+' && self.ignore == false)
			{
				OPERATOR op = OPERATOR_PLUS;
				if(it + 1 != end(expr) && it[1] == '?')
				{
					op = OPERATOR_PLUS_NON_GREEDY;
					++it;
				}

				if(compiler_op(self, op) == false)
					return false;

				self.ignore = false;
				self.recommend_concat = true;
			}
			//if found ? and we are not in ignore state then add optional op
			else if(c == '?' && self.ignore == false)
			{
				OPERATOR op = OPERATOR_OPTIONAL;
				if(it + 1 != end(expr) && it[1] == '?')
				{
					op = OPERATOR_OPTIONAL_NON_GREEDY;
					++it;
				}

				if(compiler_op(self, op) == false)
					return false;

				self.ignore = false;
				self.recommend_concat = true;
			}
			//if found . and we are not in ignore state then add any byte
			else if(c == '.' && self.ignore == false)
			{
				if(compiler_maybe_concat(self) == false)
					return false;
				
				buf_push(self.operands, buf_lit<uint8_t>({ ISA_ANY }));

				self.ignore = false;
				self.recommend_concat = true;
			}
			//if found ( and we are not in ignore state then add OPEN_PAREN operator
			else if(c == '(' && self.ignore == false)
			{
				if(compiler_maybe_concat(self) == false)
					return false;

				buf_push(self.operators, OPERATOR_OPEN_PAREN);
				self.ignore = false;
				self.recommend_concat = false;
			}
			//if found ) and we are not in ignore state then flush until ( is found
			else if(c == ')' && self.ignore == false)
			{
				for(;buf_empty(self.operators) == false && buf_top(self.operators) != OPERATOR_OPEN_PAREN;)
					if(compiler_eval(self) == false)
						return false;
				//pop open parent
				buf_pop(self.operators);
				self.ignore = false;
				self.recommend_concat = true;
			}
			//if found [ and we are not in ignore state then
			else if(c == '[' && self.ignore == false)
			{
				if(compiler_maybe_concat(self) == false)
					return false;

				auto C = buf_new<uint8_t>();
				buf_push(C, ISA_SET);
				push_int(C, 0);

				if (it != end(expr) && it[1] == '^')
				{
					C[0] = ISA_NSET;
					++it;
					c = *it;
				}

				bool local_ignore = false;
				char prev_c = c;
				int component_count = 0;
				for(++it; it != end(expr); ++it)
				{
					c = *it;
					if(c == '\\' && local_ignore == false)
					{
						local_ignore = true;
					}
					else if(c == ']' && local_ignore == false)
					{
						break;
					}
					else if(c == '-' && local_ignore == false)
					{
						//pop the prev char
						--component_count;
						buf_pop(C);

						++it;
						if(it == end(expr))
						{
							buf_free(C);
							return false;
						}
						char next_c = *it;

						if(next_c < prev_c)
						{
							buf_free(C);
							return false;
						}

						buf_push(C, ISA_RANGE);
						buf_push(C, prev_c);
						buf_push(C, next_c);
						++component_count;
						local_ignore = false;
					}
					else
					{
						buf_push(C, c);
						++component_count;
						local_ignore = false;
					}
					
					prev_c = c;
				}

				if(it == end(expr))
				{
					buf_free(C);
					return false;
				}
				c = *it;

				if(c != ']')
				{
					buf_free(C);
					return false;
				}

				patch_int_at(C, 1, component_count);
				self.ignore = false;
				self.recommend_concat = true;
				buf_push(self.operands, C);
			}
			//if no regex specific byte found then this is a simple byte
			else
			{
				if(compiler_maybe_concat(self) == false)
					return false;

				buf_push(self.operands, buf_lit<uint8_t>({ISA_BYTE, uint8_t(c)}));
				self.ignore = false;
				self.recommend_concat = true;
			}
		}

		for(;buf_empty(self.operators) == false;)
			if(compiler_eval(self) == false)
				return false;

		return true;
	}


	//API
	Buf<uint8_t>
	compile(const Str& expr, Allocator allocator)
	{
		auto c = compiler_new();
		mn_defer(compiler_free(c));

		auto res = buf_with_allocator<uint8_t>(allocator);

		if(compiler_run(c, expr) == false)
			return res;

		buf_concat(res, buf_top(c.operands));
		buf_push(res, ISA_HALT);
		return res;
	}
}