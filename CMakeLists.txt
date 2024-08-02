check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/slicescope-examples-test-suite.cc
        )
endif()

build_lib(
    LIBNAME slicescope
    SOURCE_FILES model/slicescope.cc
                 helper/slicescope-helper.cc
                 helper/slicescope-switch-helper.cc
                 model/slicescope-switch-net-device.cc
    HEADER_FILES model/slicescope.h
                 helper/slicescope-helper.h
                 helper/slicescope-switch-helper.h
                 model/slicescope-switch-net-device.h
    LIBRARIES_TO_LINK ${libcore}
    TEST_SOURCES test/slicescope-test-suite.cc
                 ${examples_as_tests_sources}
)
