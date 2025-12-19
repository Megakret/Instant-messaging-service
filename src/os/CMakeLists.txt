add_library(mutex_lib mutex.cpp)
target_include_directories(mutex_lib PUBLIC ${INCLUDES})

add_library(thread_lib threads_unix.cpp)
target_include_directories(thread_lib PUBLIC ${INCLUDES})
