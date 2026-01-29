cmake_minimum_required(VERSION 3.30)
project(tgBot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.12.0 REQUIRED)

if(POLICY CMP0167)
   cmake_policy(SET CMP0167 OLD)
endif()

find_package(Boost 1.80 REQUIRED)
if(TARGET Boost::system)
    set(BOOST_SYSTEM_TARGET Boost::system)
    message(STATUS "Using Boost::system target")
else()
    set(BOOST_SYSTEM_TARGET "")
    message(STATUS "Boost::system not available, using header-only")
endif()

add_executable(tgBot src/main.cpp src/config.cpp src/topicWatchdog.cpp)

target_compile_options(tgBot PRIVATE -Wall)
target_compile_definitions(tgBot PRIVATE HAVE_CURL)
target_include_directories(tgBot PRIVATE /usr/local/include include)
target_link_libraries(tgBot 
    PRIVATE 
        /usr/local/lib/libTgBot.a 
        Threads::Threads
        OpenSSL::SSL 
        OpenSSL::Crypto
        CURL::libcurl
        #BOOST_SYSTEM_TARGET
        nlohmann_json::nlohmann_json
)
