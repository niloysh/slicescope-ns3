file(GLOB EXAMPLE_SOURCES *.cc)

foreach(EXAMPLE_FILE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
    build_lib_example(
        NAME ${EXAMPLE_NAME}
        SOURCE_FILES ${EXAMPLE_FILE}
        LIBRARIES_TO_LINK ${libslicescope} ${libcore} ${libnetwork} ${libinternet} ${libpoint-to-point}
    )
endforeach()