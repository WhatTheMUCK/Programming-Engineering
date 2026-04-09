cmake_minimum_required(VERSION 3.14)
project(email-service CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable PostgreSQL support, disable other features we don't need
set(USERVER_FEATURE_POSTGRESQL ON CACHE BOOL "" FORCE)
set(USERVER_FEATURE_PATCH_LIBPQ OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_MONGODB OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_REDIS OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_CLICKHOUSE OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_GRPC OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_RABBITMQ OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_MYSQL OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_ROCKS OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_YDB OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_TESTSUITE OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_FORMAT OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_CHAOTIC OFF CACHE BOOL "" FORCE)

# Try to find preinstalled userver first, otherwise fetch from source
find_package(userver QUIET COMPONENTS core postgresql)
if(NOT userver_FOUND)
    message(STATUS "userver not found, fetching from source via CPM...")
    include(FetchContent)
    FetchContent_Declare(
        userver
        GIT_REPOSITORY https://github.com/userver-framework/userver.git
        GIT_TAG v2.6
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(userver)
endif()

# Find PostgreSQL client library
find_package(PostgreSQL REQUIRED)
if(NOT PostgreSQL_FOUND)
    message(FATAL_ERROR "PostgreSQL client library (libpq) not found")
endif()

message(STATUS "PostgreSQL libraries: ${PostgreSQL_LIBRARIES}")
message(STATUS "PostgreSQL include dirs: ${PostgreSQL_INCLUDE_DIRS}")

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
    ${PostgreSQL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-user PRIVATE
    userver::core
    userver::postgresql
    ${PostgreSQL_LIBRARIES}
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
    ${PostgreSQL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-folder PRIVATE
    userver::core
    userver::postgresql
    ${PostgreSQL_LIBRARIES}
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
    ${PostgreSQL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}-message PRIVATE
    userver::core
    userver::postgresql
    ${PostgreSQL_LIBRARIES}
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
