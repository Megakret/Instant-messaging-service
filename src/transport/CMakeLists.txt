add_library(pipe_transport_lib PipeTransport.cpp)
target_include_directories(pipe_transport_lib PUBLIC ${INCLUDES})
add_executable(pipe_transport_test PipeTransport_test.cpp)
target_link_libraries(pipe_transport_test pipe_transport_lib GTest::gtest_main) 
