include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# JUCE 9.0.1 (commit e18f7f5). Archive fetch avoids cloning the full git history.
FetchContent_Declare(
    JUCE
    URL https://github.com/juce-framework/JUCE/archive/e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(JUCE)

if(BUILD_TESTING)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.8.1
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()
