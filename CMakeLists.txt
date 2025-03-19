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
    SOURCE_FILES helper/slicescope-switch-helper.cc
                 model/slicescope-switch-net-device.cc
                 model/slicescope-header.cc
                 model/custom-packet-sink.cc
                 model/custom-traffic-generator.cc
                 helper/slice-helper.cc
                 model/custom-queue-disc.cc
                 helper/topology-helper.cc
                 helper/linear-topology-helper.cc
                 helper/fat-tree-topology-helper.cc
                 model/slice.cc
                 model/metadata-tag.cc
    HEADER_FILES helper/slicescope-switch-helper.h
                 model/slicescope-switch-net-device.h
                 model/slicescope-header.h
                 model/custom-packet-sink.h
                 model/custom-traffic-generator.h
                 helper/slice-helper.h
                 model/time-tag.h
                 model/custom-queue-disc.h
                 helper/topology-helper.h
                 helper/linear-topology-helper.h
                    helper/fat-tree-topology-helper.h
                 model/slice.h
                 model/metadata-tag.h
    LIBRARIES_TO_LINK ${libcore} ${libnetwork} ${libinternet} ${libcsma} ${libbridge} ${libnetwork} ${libpoint-to-point}
    TEST_SOURCES test/slicescope-test-suite.cc
                 ${examples_as_tests_sources}
)
