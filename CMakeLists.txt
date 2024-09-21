cmake_minimum_required(VERSION 3.25.1)


find_package(Git QUIET)

if(GIT_FOUND)
  message(STATUS "Updating VCPKG submodule")
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  RESULT_VARIABLE GIT_SUBMOD_RESULT)

  if(GIT_SUBMOD_RESULT EQUAL "0")
    message(STATUS "Successfully updated VCPKG submodule")
  else()
    message(FATAL_ERROR "Failed to update VCPKG submodule: '${GIT_SUBMOD_RESULT}'")
  endif()
endif()


project(pokatto_prestige_bot VERSION 1.0 DESCRIPTION "Pokatto Prestige Bot")  


# Bot project
add_executable(${PROJECT_NAME}
               src/main.cc
               src/bot/pokatto_prestige_bot.cc
               src/bot/pokatto_prestige_bot.h
               src/bot/pokatto_prestige/pokatto_prestige.cc
               src/bot/pokatto_prestige/pokatto_prestige.h
               src/bot/pokatto_prestige/pokatto/pokatto_data.cc
               src/bot/pokatto_prestige/pokatto/pokatto_data.h
               src/bot/settings/settings.cc
               src/bot/settings/settings.h
               src/logger/logger.cc
               src/logger/logger.h
               src/logger/logger_factory.cc
               src/logger/logger_factory.h)


target_include_directories(${PROJECT_NAME} PRIVATE
                           src
                           src/bot)


find_package(argparse CONFIG REQUIRED)                                              # Arguments parsing
find_package(dpp CONFIG REQUIRED)                                                   # Interfacing with Discord
find_package(fmt CONFIG REQUIRED)                                                   # Format logging
find_package(nlohmann_json CONFIG REQUIRED)                                         # Settings and data storage
find_package(Opus CONFIG REQUIRED)                                                  # DPP dependency
find_package(OpenSSL REQUIRED)                                                      # DPP dependency
find_package(spdlog CONFIG REQUIRED)                                                # Logging
find_package(ZLIB REQUIRED)                                                         # DPP dependency

if(UNIX)
find_package(unofficial-sodium CONFIG REQUIRED)                                     # DPP dependency
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
                      argparse::argparse
                      dpp::dpp
                      fmt::fmt-header-only
                      nlohmann_json::nlohmann_json
                      OpenSSL::SSL
                      Opus::opus
                      spdlog::spdlog_header_only
                      ZLIB::ZLIB)

if(UNIX)
target_link_libraries(${PROJECT_NAME} PRIVATE unofficial-sodium::sodium)
endif()


set_target_properties(${PROJECT_NAME} PROPERTIES
                      CXX_STANDARD 23
                      CXX_STANDARD_REQUIRED ON)


if(NOT EXISTS "${CMAKE_BINARY_DIR}/settings/settings.json")
  configure_file(sample/settings.json "${CMAKE_BINARY_DIR}/settings/settings.json" COPYONLY)
endif()