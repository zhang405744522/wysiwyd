# dopyright: (C) 2014 WYSIWYD Consortium
# Authors: Hector Barron-Gonzalez
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

project(awareTouch)

file(GLOB source_files src/*.cpp)
file(GLOB header_files include/*.h)
set(doc_files ${PROJECT_NAME}.xml)
source_group("Source Files" FILES ${source_files})
source_group("Header Files" FILES ${header_files})
source_group("DOC Files" FILES ${doc_files})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${wrdac_INCLUDE_DIRS}
                    ${YARP_INCLUDE_DIRS}
                    ${ICUB_INCLUDE_DIRS} )
add_executable(${PROJECT_NAME} ${source_files} ${header_files} ${doc_files})
target_link_libraries(${PROJECT_NAME} ${ICUB_LIBRARIES} wrdac ${YARP_LIBRARIES})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER systemHaptic)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
yarp_install(FILES ${doc_files} DESTINATION ${WYSIWYD_MODULES_INSTALL_DIR})
