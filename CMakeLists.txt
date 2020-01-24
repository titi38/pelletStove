#####################  CMakeLists for pelletStove build   ###########################
#  Written by T.DESCOMBES  (thierry.descombes@gmail.com)                      #
#                                                                             #
#  To create an Xcode project:                                                #
#         cmake -G Xcode .                                                    #
###############              cmake version          ###########################
cmake_minimum_required(VERSION 2.6)

####################         Project name           ###########################
project(PELLET)

#################            version variables           ######################
set(PELLET_VERSION "2.0.0")
EXEC_PROGRAM("/bin/date +%s" OUTPUT_VARIABLE PELLET_BUILD_DATE)

#####################            Minimal flags           ######################
IF(${UNIX})
  set (CMAKE_CXX_FLAGS "-DLINUX -Wall -Wno-unused -fexceptions -fPIC -D_REENTRANT -std=c++11 -g3 -Wall -O3 -DPELLET_SOFTWARE_VERSION=\"\\\"${PELLET_VERSION}\\\"\" -DPELLET_BUILD_DATE=\"${PELLET_BUILD_DATE}\"")
ENDIF(${UNIX})

IF(${APPLE})
  set (CMAKE_CXX_FLAGS "-D__darwin__ -D__x86__ -fPIC -fno-common -D_REENTRANT -std=c++0x -g3 -Wall -O3 -DPELLET_SOFTWARE_VERSION=\"\\\"${PELLET_VERSION}\\\"\" -DPELLET_BUILD_DATE=\"${PELLET_BUILD_DATE}\"")
ENDIF(${APPLE})

if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

#########   Check the compiler and set the compile and link flags  ###########
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake.modules/")


find_package (Threads)

###############        OpenSSL include/lib dir        ########################
find_package(OpenSSL REQUIRED)
include_directories (${OPENSSL_INCLUDE_DIR})

###############        libnavajo include/lib dir        ######################
#include(config/cmake/FindLibnavajo.cmake)
find_package(Libnavajo REQUIRED)
include_directories(${LIBNAVAJO_INCLUDE_DIRS})

##################          cmake Module directory       #####################

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/../cmake.modules/")
set (PROJECT_SOURCE_DIR src)

#######################          /lib dir        #############################

#find_package(PostgreSQL REQUIRED)
#set (PostgreSQL_TYPE_INCLUDE_DIR "/usr/include/postgresql")
#include_directories(${PostgreSQL_INCLUDE_DIRS})

find_package(Libnavajo REQUIRED)
include_directories(${LIBNAVAJO_INCLUDE_DIRS})

find_package(OpenSSL REQUIRED) 
include_directories (${OPENSSL_INCLUDE_DIR})

find_package(ZLIB REQUIRED)

find_package(wiringPi REQUIRED)
include_directories(${WIRINGPI_INCLUDE_DIRS})

include_directories ( ${PROJECT_SOURCE_DIR}/rapidjson/include )

include_directories ( 
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/ButtonControl
  ${PROJECT_SOURCE_DIR}/DhtReader
  ${PROJECT_SOURCE_DIR}/OpenWeatherClient
  ${PROJECT_SOURCE_DIR}/PelletService
  ${PROJECT_SOURCE_DIR}/LcdReader
  ${PROJECT_SOURCE_DIR}/Gauge
  ${PROJECT_SOURCE_DIR}/AutoMode
  )
  
###################             Sources files           #######################

file(GLOB sources_cpp 
  ${PROJECT_SOURCE_DIR}/
  ${PROJECT_SOURCE_DIR}/ButtonControl/ButtonControl.cc
  ${PROJECT_SOURCE_DIR}/DhtReader/DhtReader.cc
  ${PROJECT_SOURCE_DIR}/OpenWeatherClient/OpenWeatherClient.cc
  ${PROJECT_SOURCE_DIR}/PelletService/PelletService.cc
  ${PROJECT_SOURCE_DIR}/LcdReader/LcdReader.cc
  ${PROJECT_SOURCE_DIR}/Gauge/Gauge.cc
  ${PROJECT_SOURCE_DIR}/AutoMode/AutoMode.cc
  #  ${PROJECT_SOURCE_DIR}/LogWebBuffer/LogWebBuffer.cc
  )
  
###################          Output directory          ########################
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

#################          HTML Precompilation          #######################
#set(HTML_REPOSITORY ${PROJECT_SOURCE_DIR}/HtmlRepository/html)
#HTML_PRECOMPIL_REPO("${HTML_REPOSITORY}")


#########################  Binary generation ##################################
set_source_files_properties(${LIBNAVAJO_precompiledFile}
                            PROPERTIES GENERATED TRUE)

add_executable( pelletStove ${LIBNAVAJO_precompiledFile} ${sources_cpp} )

target_link_libraries(pelletStove ${OPENSSL_LIBRARIES})
target_link_libraries(pelletStove ${LIBNAVAJO_LIBRARIES})
#target_link_libraries(pelletStove ${PostgreSQL_LIBRARIES})
target_link_libraries(pelletStove ${ZLIB_LIBRARIES})
target_link_libraries(pelletStove ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(pelletStove ${WIRINGPI_LIBRARIES})

######################### document file generation ############################

find_package(Doxygen)
if(DOXYGEN_FOUND)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)

