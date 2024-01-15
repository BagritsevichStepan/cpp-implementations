project(signal)

add_executable(signal cpp_signal.h
        intrusive_list/intrusive_list.h
        intrusive_list/intrusive_list.cpp
        main.cpp)