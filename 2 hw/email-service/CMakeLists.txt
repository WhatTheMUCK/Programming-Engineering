cmake_minimum_required(VERSION 3.12)
project(email-service CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find userver (preinstalled in Docker image)
find_package(userver REQUIRED COMPONENTS core)

# Find SQLite3
find_package(SQLite3 REQUIRED)
if(NOT SQLite3_FOUND)
    message(FATAL_ERROR "SQLite3 not found")
endif()

message(STATUS "SQLite3 libraries: ${SQLITE3_LIBRARIES}")
message(STATUS "SQLite3 include dirs: ${SQLITE3_INCLUDE_DIRS}")

# JWT-CPP is header-only, we just need to include it
# It will be installed in /usr/local/include/jwt-cpp
message(STATUS "JWT-CPP will be used as header-only library")

# User Service executable
add_executable(${PROJECT_NAME}-user
    src/user/main.cpp
    src/user/handlers.cpp
    src/common/models.cpp
    src/common/database.cpp
    src/auth/jwt_auth_checker.cpp
    src/auth/jwt_auth_factory.cpp
)

target_include_directories(${PROJECT_NAME}-user PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-user PRIVATE
    userver::core
    /usr/lib/x86_64-linux-gnu/libsqlite3.so
)

# Folder Service executable
add_executable(${PROJECT_NAME}-folder
    src/folder/main.cpp
    src/folder/handlers.cpp
    src/common/models.cpp
    src/common/database.cpp
    src/auth/jwt_auth_checker.cpp
    src/auth/jwt_auth_factory.cpp
)

target_include_directories(${PROJECT_NAME}-folder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-folder PRIVATE
    userver::core
    /usr/lib/x86_64-linux-gnu/libsqlite3.so
)

# Message Service executable
add_executable(${PROJECT_NAME}-message
    src/message/main.cpp
    src/message/handlers.cpp
    src/common/models.cpp
    src/common/database.cpp
    src/auth/jwt_auth_checker.cpp
    src/auth/jwt_auth_factory.cpp
)

target_include_directories(${PROJECT_NAME}-message PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-message PRIVATE
    userver::core
    /usr/lib/x86_64-linux-gnu/libsqlite3.so
)

# Install targets
install(TARGETS ${PROJECT_NAME}-user ${PROJECT_NAME}-folder ${PROJECT_NAME}-message
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${PROJECT_NAME}
)

# Install configs
install(DIRECTORY configs/
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
    COMPONENT ${PROJECT_NAME}
)
