add_subdirectory(transport)
add_subdirectory(handlers)

add_library(main_handler_lib main_handler.cpp)
target_include_directories(main_handler_lib PUBLIC ${INCLUDES})
target_link_libraries(main_handler_lib PUBLIC proto-objects messenger_service_lib handle_lib)

add_executable(main_handler_test main_handler_test.cpp)
target_link_libraries(main_handler_test PUBLIC main_handler_lib GTest::gtest_main)
foreach(executable main_handler_test)
	target_include_directories(${executable} PUBLIC ${INCLUDES})
endforeach()
