SET(INC_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_PROJECT_NAME}cpp)
INSTALL(FILES libcaer.hpp network.hpp DESTINATION ${INC_INSTALL_DIR})
INSTALL(DIRECTORY events DESTINATION ${INC_INSTALL_DIR} FILES_MATCHING PATTERN "*.hpp")
INSTALL(DIRECTORY devices DESTINATION ${INC_INSTALL_DIR} FILES_MATCHING PATTERN "*.hpp")
