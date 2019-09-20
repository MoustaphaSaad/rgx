cmake_minimum_required(VERSION 3.9)

# list source files
set(SOURCE_FILES
	src/unittest_rgx.cpp
	src/unittest_main.cpp
)

# add executable target
add_executable(rgx_unittest
	${SOURCE_FILES}
)

target_link_libraries(rgx_unittest
	PRIVATE
		MoustaphaSaad::rgx
)

# add doctest folder
target_include_directories(rgx_unittest
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/doctest
)
