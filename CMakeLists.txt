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
                 model/slicescope-header.cc
                 model/simple-packet-sink.cc
                 model/custom-traffic-generator.cc
                 helper/slice-traffic-helper.cc
                 model/simple-queue-disc.cc
    HEADER_FILES model/slicescope.h
                 helper/slicescope-helper.h
                 helper/slicescope-switch-helper.h
                 model/slicescope-switch-net-device.h
                 model/slicescope-header.h
                 model/simple-packet-sink.h
                 model/custom-traffic-generator.h
                 helper/slice-traffic-helper.h
                 model/time-tag.h
                 model/simple-queue-disc.h
    LIBRARIES_TO_LINK ${libcore} ${libnetwork} ${libinternet}
    TEST_SOURCES test/slicescope-test-suite.cc
                 ${examples_as_tests_sources}
)
