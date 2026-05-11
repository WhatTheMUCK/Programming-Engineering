cmake_minimum_required(VERSION 3.14)
project(email-service CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use FetchContent to get userver - this is the recommended way for userver v2.6
include(FetchContent)

# Fetch userver from GitHub
FetchContent_Declare(
    userver
    GIT_REPOSITORY https://github.com/userver-framework/userver.git
    GIT_TAG v2.6
    GIT_SHALLOW TRUE
)

# Set userver options
set(USERVER_FEATURE_MONGODB ON CACHE BOOL "" FORCE)
set(USERVER_FEATURE_POSTGRESQL OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_REDIS OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_CLICKHOUSE OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_KAFKA OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_YDB OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_RABBITMQ ON CACHE BOOL "" FORCE)
set(USERVER_FEATURE_TESTSUITE OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_GRPC OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_FORMAT OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_CHAOTIC OFF CACHE BOOL "" FORCE)

# Make userver available - this creates the userver targets directly
FetchContent_MakeAvailable(userver)

# Debug: Print userver source directory
message(STATUS "[INTERESTING PART] userver_SOURCE_DIR: ${userver_SOURCE_DIR}")
message(STATUS "[INTERESTING PART] userver_BINARY_DIR: ${userver_BINARY_DIR}")
message(STATUS "[INTERESTING PART] CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "[INTERESTING PART] CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

# Check if userver directories exist
if(EXISTS "${userver_SOURCE_DIR}/universal/include")
    message(STATUS "[INTERESTING PART] userver universal include EXISTS")
else()
    message(STATUS "[INTERESTING PART] userver universal include DOES NOT EXIST")
endif()
if(EXISTS "${userver_SOURCE_DIR}/core/include")
    message(STATUS "[INTERESTING PART] userver core include EXISTS")
else()
    message(STATUS "[INTERESTING PART] userver core include DOES NOT EXIST")
endif()
if(EXISTS "${userver_SOURCE_DIR}/mongo/include")
    message(STATUS "[INTERESTING PART] userver mongo include EXISTS")
else()
    message(STATUS "[INTERESTING PART] userver mongo include DOES NOT EXIST")
endif()
if(EXISTS "${userver_SOURCE_DIR}/rabbitmq/include")
    message(STATUS "[INTERESTING PART] userver rabbitmq include EXISTS")
    file(GLOB RABBITMQ_HEADERS "${userver_SOURCE_DIR}/rabbitmq/include/userver/rabbitmq/*.hpp")
    message(STATUS "[INTERESTING PART] rabbitmq headers: ${RABBITMQ_HEADERS}")
    
    # Check for the main rabbitmq.hpp header
    if(EXISTS "${userver_SOURCE_DIR}/rabbitmq/include/userver/rabbitmq.hpp")
        message(STATUS "[INTERESTING PART] userver/rabbitmq.hpp EXISTS")
    else()
        message(STATUS "[INTERESTING PART] userver/rabbitmq.hpp DOES NOT EXIST")
    endif()
else()
    message(STATUS "[INTERESTING PART] userver rabbitmq include DOES NOT EXIST")
endif()

# Find MongoDB C driver using pkg-config
include(FindPkgConfig)
pkg_check_modules(MONGOC REQUIRED libmongoc-1.0)
pkg_check_modules(BSON REQUIRED libbson-1.0)
pkg_check_modules(MONGOCXX REQUIRED libmongocxx)
pkg_check_modules(BSONCXX REQUIRED libbsoncxx)

# MongoDB C++ driver paths
set(MONGOCXX_INCLUDE_DIRS /usr/local/include/mongocxx/v_noabi)
set(BSONCXX_INCLUDE_DIRS /usr/local/include/bsoncxx/v_noabi)

# JWT-CPP is header-only, we just need to include it
# It will be installed in /usr/local/include/jwt-cpp
message(STATUS "JWT-CPP will be used as header-only library")

# User Service executable
add_executable(${PROJECT_NAME}-user
    src/user/main.cpp
    src/user/handlers.cpp
    src/common/models.cpp
    src/common/database.cpp
    src/common/metrics_handler.cpp
    src/common/read_model_handler.cpp
    src/common/read_model_cache.cpp
    src/auth/jwt_auth_checker.cpp
    src/auth/jwt_auth_factory.cpp
)

target_include_directories(${PROJECT_NAME}-user PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MONGOCXX_INCLUDE_DIRS}
    ${BSONCXX_INCLUDE_DIRS}
    ${MONGOC_INCLUDE_DIRS}
    ${BSON_INCLUDE_DIRS}
    ${userver_SOURCE_DIR}/universal/include
    ${userver_SOURCE_DIR}/core/include
    ${userver_SOURCE_DIR}/mongo/include
    ${userver_SOURCE_DIR}/rabbitmq/include
)

message(STATUS "[INTERESTING PART] ${PROJECT_NAME}-user include dirs:")
get_target_property(USER_INCLUDE_DIRS ${PROJECT_NAME}-user INCLUDE_DIRECTORIES)
message(STATUS "[INTERESTING PART] ${USER_INCLUDE_DIRS}")

target_link_libraries(${PROJECT_NAME}-user PRIVATE
    userver::universal
    userver::core
    userver::mongo
    userver::rabbitmq
    ${MONGOCXX_LIBRARIES}
    ${BSONCXX_LIBRARIES}
    ${MONGOC_LIBRARIES}
    ${BSON_LIBRARIES}
)

# Folder Service executable
add_executable(${PROJECT_NAME}-folder
    src/folder/main.cpp
    src/folder/handlers.cpp
    src/common/models.cpp
    src/common/database.cpp
    src/common/read_model_handler.cpp
    src/common/read_model_cache.cpp
    src/auth/jwt_auth_checker.cpp
    src/auth/jwt_auth_factory.cpp
)

target_include_directories(${PROJECT_NAME}-folder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MONGOCXX_INCLUDE_DIRS}
    ${BSONCXX_INCLUDE_DIRS}
    ${MONGOC_INCLUDE_DIRS}
    ${BSON_INCLUDE_DIRS}
    ${userver_SOURCE_DIR}/universal/include
    ${userver_SOURCE_DIR}/core/include
    ${userver_SOURCE_DIR}/mongo/include
    ${userver_SOURCE_DIR}/rabbitmq/include
)

target_link_libraries(${PROJECT_NAME}-folder PRIVATE
    userver::universal
    userver::core
    userver::mongo
    userver::rabbitmq
    ${MONGOCXX_LIBRARIES}
    ${BSONCXX_LIBRARIES}
    ${MONGOC_LIBRARIES}
    ${BSON_LIBRARIES}
)

# Message Service executable
add_executable(${PROJECT_NAME}-message
    src/message/main.cpp
    src/message/handlers.cpp
    src/common/models.cpp
    src/common/database.cpp
    src/common/read_model_handler.cpp
    src/common/read_model_cache.cpp
    src/auth/jwt_auth_checker.cpp
    src/auth/jwt_auth_factory.cpp
)

target_include_directories(${PROJECT_NAME}-message PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MONGOCXX_INCLUDE_DIRS}
    ${BSONCXX_INCLUDE_DIRS}
    ${MONGOC_INCLUDE_DIRS}
    ${BSON_INCLUDE_DIRS}
    ${userver_SOURCE_DIR}/universal/include
    ${userver_SOURCE_DIR}/core/include
    ${userver_SOURCE_DIR}/mongo/include
    ${userver_SOURCE_DIR}/rabbitmq/include
)

target_link_libraries(${PROJECT_NAME}-message PRIVATE
    userver::universal
    userver::core
    userver::mongo
    userver::rabbitmq
    ${MONGOCXX_LIBRARIES}
    ${BSONCXX_LIBRARIES}
    ${MONGOC_LIBRARIES}
    ${BSON_LIBRARIES}
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
