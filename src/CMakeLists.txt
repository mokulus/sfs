set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -Wall -O2")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-g -Wall -Wextra -Wpedantic -Wconversion -Wcast-qual -Wwrite-strings -Werror)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(sfs
	sfs.cpp)

install(TARGETS sfs DESTINATION bin)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})
target_link_libraries(sfs ${CURSES_LIBRARIES})

find_package(fmt)
target_link_libraries(sfs fmt::fmt)
