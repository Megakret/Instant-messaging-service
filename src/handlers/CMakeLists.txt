foreach(example user)
  add_library(${example}_lib ${example}.cpp)
	target_include_directories(${example}_lib PUBLIC ${INCLUDES})
	target_link_libraries(${example}_lib PUBLIC proto-objects)
endforeach()

add_library(messenger_service_lib messenger_service.cpp)
target_include_directories(messenger_service_lib PUBLIC ${INCLUDES})
target_link_libraries(messenger_service_lib PUBLIC user_lib proto-objects)

add_library(postpone_handler_lib postpone_handler.cpp)
target_include_directories(postpone_handler_lib PUBLIC ${INCLUDES})
target_link_libraries(postpone_handler_lib PUBLIC postpone_service_lib proto-objects)

add_library(handle_lib handle.cpp)
target_include_directories(handle_lib PUBLIC ${INCLUDES})
target_link_libraries(handle_lib PUBLIC pipe_transport_lib)

