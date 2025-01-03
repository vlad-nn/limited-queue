find_package(GTest)
if(NOT GTest_FOUND)
    include(FetchContent)

    set(BUILD_GMOCK OFF)
    set(INSTALL_GTEST OFF)
    set(gtest_force_shared_crt ON)

    FetchContent_Declare(
        gtest
        DOWNLOAD_EXTRACT_TIMESTAMP
        URL https://github.com/google/googletest/archive/main.tar.gz
    )
    FetchContent_MakeAvailable(gtest)
endif()

add_executable(limited_queue_test test.cpp)
target_link_libraries(limited_queue_test PRIVATE GTest::gtest GTest::gtest_main)
gtest_discover_tests(limited_queue_test)