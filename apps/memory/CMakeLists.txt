project(memory)

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
      base-lib
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

add_custom_target(${PROJECT_NAME}_run
  COMMAND ${PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "run generated app in ${CMAKE_CURRENT_SOURCE_DIR}"
)
