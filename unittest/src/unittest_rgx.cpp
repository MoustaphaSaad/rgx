#include <doctest/doctest.h>

#include <rgx/Compiler.h>
#include <rgx/VM.h>

#include <mn/Str.h>
#include <mn/IO.h>
#include <utf8proc/utf8proc.h>

using namespace rgx;
using namespace mn;

inline static bool
matched(const Buf<uint8_t>& prog, const char* s)
{
	auto str = str_lit(s);
	auto [it, ok] = match(prog, begin(str));
	return ok;
}

inline static bool
matched_substr(const Buf<uint8_t>& prog, size_t count, const char* s)
{
	auto str = str_lit(s);
	auto [it, ok] = match(prog, begin(str));
	CHECK(it == begin(str) + count);
	return ok;
}

TEST_CASE("simple concat")
{
	auto prog = compile("abc");

	CHECK(matched(prog, "abc") == true);
	CHECK(matched(prog, "acb") == false);
	CHECK(matched(prog, "") == false);

	buf_free(prog);
}

TEST_CASE("simple or")
{
	auto prog = compile("ab(c|d)");

	CHECK(matched(prog, "abc") == true);
	CHECK(matched(prog, "abd") == true);
	CHECK(matched(prog, "ab") == false);
	CHECK(matched(prog, "") == false);

	buf_free(prog);
}

TEST_CASE("simple star")
{
	auto prog = compile("abc*");

	CHECK(matched(prog, "abc") == true);
	CHECK(matched(prog, "abd") == true);
	CHECK(matched(prog, "ab") == true);
	CHECK(matched_substr(prog, 9, "abccccccc") == true);
	CHECK(matched(prog, "") == false);

	buf_free(prog);
}

TEST_CASE("set star")
{
	auto prog = compile("[a-z]*");

	CHECK(matched(prog, "abc") == true);
	CHECK(matched(prog, "123") == true);
	CHECK(matched(prog, "ab") == true);
	CHECK(matched(prog, "DSFabccccccc") == true);
	CHECK(matched(prog, "") == true);

	buf_free(prog);
}

TEST_CASE("set plus")
{
	auto prog = compile("[a-z]+");

	CHECK(matched(prog, "abc") == true);
	CHECK(matched(prog, "123") == false);
	CHECK(matched(prog, "ab") == true);
	CHECK(matched(prog, "DSFabccccccc") == false);
	CHECK(matched(prog, "") == false);

	buf_free(prog);
}

TEST_CASE("C id")
{
	auto prog = compile("[a-zA-Z_][a-zA-Z0-9_]*");

	CHECK(matched(prog, "abc") == true);
	CHECK(matched(prog, "abc_def_123") == true);
	CHECK(matched(prog, "123") == false);
	CHECK(matched(prog, "ab") == true);
	CHECK(matched(prog, "DSFabccccccc") == true);
	CHECK(matched(prog, "") == false);

	buf_free(prog);
}

TEST_CASE("Email regex")
{
	auto prog = compile("[a-z0-9!#$%&'*+/=?^_`{|}~\\-]+(\\.[a-z0-9!#$%&'*+/=?^_`{|}~\\-]+)*@([a-z0-9]([a-z0-9\\-]*[a-z0-9])?\\.)+[a-z0-9]([a-z0-9\\-]*[a-z0-9])?");

	CHECK(matched(prog, "moustapha.saad.abdelhamed@gmail.com") == true);
	CHECK(matched(prog, "mostafa") == false);
	CHECK(matched(prog, "moustapha.saad.abdelhamed@gmail") == false);
	CHECK(matched(prog, "moustapha.saad.abdelhamed@.com") == false);
	CHECK(matched(prog, "@gmail.com") == false);

	buf_free(prog);
}

TEST_CASE("quoted string")
{
	auto prog = compile("\"([^\\\"]|\\.)*\"");

	CHECK(matched(prog, "\"\"") == true);
	CHECK(matched(prog, "\"my name is \\\"mostafa\\\"\"") == true);
	CHECK(matched(prog, "moustapha.saad.abdelhamed@gmail") == false);
	CHECK(matched(prog, "") == false);

	buf_free(prog);
}

TEST_CASE("arabic")
{
	auto prog = compile("أبجد*");

	CHECK(matched(prog, "أبجد") == true);
	CHECK(matched(prog, "أد") == false);
	CHECK(matched(prog, "أبجددددددد") == true);

	buf_free(prog);
}
