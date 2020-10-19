add_library(base-lib base.cpp base.hpp)
target_include_directories(base-lib
	PUBLIC
        .
)
target_compile_definitions(base-lib 
    PUBLIC
)
target_link_libraries(base-lib
	PRIVATE
		project_options
        	project_warnings
	PUBLIC
		doctest::doctest
)


add_subdirectory(memory)
add_subdirectory(literals)
