# Tests need to be added as executables first

add_executable(app_test testlib.cpp)
target_compile_features(app_test
	PRIVATE
)

# Should be linked to the main library, as well as the Catch2 testing library
target_link_libraries(app_test
    PRIVATE
        sourcelib
        doctest::doctest
        project_options project_warnings
)

# If you register a test, then ctest and make test will run it.
# You can also run examples and check the output, as well.
add_test(NAME app_test COMMAND app_test) # Command can be a target