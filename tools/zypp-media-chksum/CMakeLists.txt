PROJECT( zypp-media-chksum C CXX )

set (CMAKE_CXX_STANDARD 17)
SET (CMAKE_CXX_EXTENSIONS OFF)

FIND_PACKAGE(Protobuf REQUIRED)

SET( SOURCES
  main.cc
)

add_executable( ${PROJECT_NAME} ${SOURCES} )
target_link_libraries( ${PROJECT_NAME} zypp-media )
target_link_libraries( ${PROJECT_NAME} zypp-core )
target_link_libraries( ${PROJECT_NAME} zypp-protobuf )
target_link_libraries( ${PROJECT_NAME} ${PROTOBUF_LITE_LIBRARIES} )

IF ( INSTALL_NG_BINARIES )
  INSTALL( TARGETS ${PROJECT_NAME} DESTINATION "${ZYPP_LIBEXEC_INSTALL_DIR}/workers" )
ENDIF()

set_target_properties( ${PROJECT_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools/workers"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools/workers"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools/workers"
)
