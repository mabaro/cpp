add_library(sourcelib-opt-deps INTERFACE)

###############################################################################

file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.hpp")
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS *.c* *.hpp)

# Make an automatic library - will be static or dynamic based on user setting
add_library(sourcelib ${SOURCE_LIST} ${HEADER_LIST})

# We need this directory, and users of our library will need it too
target_include_directories(sourcelib
	PUBLIC ${PROJECT_SOURCE_DIR}/include
)
set_target_properties(sourcelib
	PROPERTIES
		POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(sourcelib 
	PRIVATE
		doctest::doctest
        sourcelib-opt-deps
    	project_options project_warnings
)

# IDEs should put the headers in a nice place
source_group(TREE "${PROJECT_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${HEADER_LIST})
