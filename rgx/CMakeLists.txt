cmake_minimum_required(VERSION 3.9)

# list the header files
set(HEADER_FILES
	include/rgx/ISA.h
	include/rgx/Compiler.h
	include/rgx/VM.h
)

# list the source files
set(SOURCE_FILES
	src/rgx/Compiler.cpp
	src/rgx/VM.cpp
)

# add library target
add_library(rgx
	${HEADER_FILES}
	${SOURCE_FILES}
)

# zay will link to mn
target_link_libraries(rgx
	PUBLIC
		MoustaphaSaad::mn
)

add_library(MoustaphaSaad::rgx ALIAS rgx)

# make it reflect the same structure as the one on disk
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADER_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

# let's enable warnings as errors
if(WIN32)
	target_compile_options(rgx
		PRIVATE
			/WX /W4
	)
elseif(UNIX)
	target_compile_options(rgx
		PRIVATE
			-Wall -Werror
	)
endif()

# enable C++17
# disable any compiler specifc extensions
# add d suffix in debug mode
target_compile_features(rgx PUBLIC cxx_std_17)
set_target_properties(rgx PROPERTIES
	CXX_EXTENSIONS OFF
	DEBUG_POSTFIX d
)

# define debug macro
target_compile_definitions(rgx PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

# generate exports header file
include(GenerateExportHeader)
generate_export_header(rgx
	EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/rgx/Exports.h
)

# list include directories
target_include_directories(rgx
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)