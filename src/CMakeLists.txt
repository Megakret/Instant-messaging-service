add_subdirectory(transport)
add_subdirectory(handlers)

add_library(thread_lib threads_unix.cpp)
target_include_directories(thread_lib PUBLIC ${INCLUDES})

add_library(postpone_service_lib postpone_service.cpp)
target_include_directories(postpone_service_lib PUBLIC ${INCLUDES})
target_link_libraries(postpone_service_lib PUBLIC proto-objects)

add_library(main_handler_lib main_handler.cpp)
target_include_directories(main_handler_lib PUBLIC ${INCLUDES})
target_link_libraries(main_handler_lib PUBLIC proto-objects messenger_service_lib handle_lib postpone_handler_lib thread_lib)

add_executable(main_handler_test main_handler_test.cpp)
target_link_libraries(main_handler_test PUBLIC main_handler_lib GTest::gtest_main thread_lib)
foreach(executable main_handler_test)
	target_include_directories(${executable} PUBLIC ${INCLUDES})
endforeach()
