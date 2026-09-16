add_executable(interpolation_lab_tests
    AnalysisSettingsTests.cpp
    FadeInterpolatorTests.cpp
    InterpolatorCatalogTests.cpp
    OptimalTransportTests.cpp
    SpectralTransportTests.cpp
    WeightsTests.cpp)

target_link_libraries(interpolation_lab_tests
    PRIVATE
        interpolation_lab_dsp
        Catch2::Catch2WithMain
        interpolation_lab_warnings)

include(Catch)
catch_discover_tests(interpolation_lab_tests)
