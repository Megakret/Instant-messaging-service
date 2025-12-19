add_executable(client client.cpp)
target_include_directories(client PUBLIC ${INCLUDES})
target_link_libraries(client pipe_transport_lib ncurses proto-objects thread_lib mutex_lib) 
