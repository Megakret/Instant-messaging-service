add_library(PipeTransport_lib PipeTransport.cpp)
target_include_directories(PipeTransport_lib PUBLIC ${INCLUDES})

add_executable(PipeTransport_test PipeTransport_test.cpp)
target_include_directories(PipeTransport_test PUBLIC ${INCLUDES})
target_link_libraries(PipeTransport_test PipeTransport_lib GTest::gtest_main) 
