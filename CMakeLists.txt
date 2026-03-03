cmake_minimum_required(VERSION 3.10)
project(tgBot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_USE_STATIC_LIBS ON)
set(OPENSSL_USE_STATIC_LIBS TRUE)
set(CURL_USE_STATIC_LIBS TRUE)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

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

add_executable(tgBot src/main.cpp src/settings.cpp src/topicWatchdog.cpp src/mqttService.cpp src/tgService.cpp src/application.cpp src/adminController.cpp src/mapper.cpp src/graphiteService.cpp)

target_compile_options(tgBot PRIVATE -Wall)
target_compile_definitions(tgBot PRIVATE HAVE_CURL)
target_include_directories(tgBot PRIVATE include)

add_subdirectory(libs/tgbot-cpp)
add_subdirectory(libs/json)
add_subdirectory(libs/mqtt5)

target_link_libraries(tgBot 
    PRIVATE 
        boost_mqtt5
        TgBot
        Threads::Threads
        OpenSSL::SSL 
        OpenSSL::Crypto
        CURL::libcurl
        #BOOST_SYSTEM_TARGET
        nlohmann_json
)
