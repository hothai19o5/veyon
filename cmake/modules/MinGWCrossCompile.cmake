# this one is important
set(CMAKE_SYSTEM_NAME Windows)
#this one not so much
set(CMAKE_SYSTEM_VERSION 1)

# Check if MXE_PATH is provided (either as a CMake variable or environment variable)
if(NOT MXE_PATH AND DEFINED ENV{MXE_PATH})
	set(MXE_PATH "$ENV{MXE_PATH}")
endif()

if(MXE_PATH)
	set(MINGW_PREFIX ${MXE_PATH}/usr/${MINGW_TARGET}/)
	set(MINGW_TOOL_PREFIX ${MXE_PATH}/usr/bin/${MINGW_TARGET}-)
else()
	set(MINGW_PREFIX /usr/${MINGW_TARGET}/)
	set(MINGW_TOOL_PREFIX /usr/bin/${MINGW_TARGET}-)
endif()

# where is the target environment 
set(CMAKE_FIND_ROOT_PATH	${MINGW_PREFIX})
set(CMAKE_INSTALL_PREFIX	${MINGW_PREFIX})



# specify the cross compiler
set(CMAKE_C_COMPILER		${MINGW_TOOL_PREFIX}gcc)
set(CMAKE_CXX_COMPILER		${MINGW_TOOL_PREFIX}g++)

# specify location of some tools
set(STRIP					${MINGW_TOOL_PREFIX}strip)

set(QT_BINARY_DIR			${MINGW_PREFIX}/bin)

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_INCLUDE_PATH ${MINGW_PREFIX}/include)
