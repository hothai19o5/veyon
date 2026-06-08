set(WINDOWS_INSTALL_FILES "veyon-${VEYON_WINDOWS_ARCH}-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_BUILD}")

set(DLLDIR "${MINGW_PREFIX}/bin")
set(DLLDIR_LIB "${MINGW_PREFIX}/lib")

if(QT_MAJOR_VERSION EQUAL 5)
	set(DLLDIR_QT "${MINGW_PREFIX}/qt5/bin")
	set(QT_PLUGINDIR "${MINGW_PREFIX}/qt5/plugins")
	set(DLL_QCA "${DLLDIR_QT}/libqca-qt5.dll")
	set(QCA_CRYPTO_DIR "${QT_PLUGINDIR}/crypto")
	set(QT_DLLS
		${DLLDIR_QT}/Qt5Core.dll
		${DLLDIR_QT}/Qt5Gui.dll
		${DLLDIR_QT}/Qt5Svg.dll
		${DLLDIR_QT}/Qt5Widgets.dll
		${DLLDIR_QT}/Qt5Network.dll
		${DLLDIR_QT}/Qt5Concurrent.dll)
else()
	set(DLLDIR_QT "${DLLDIR}")
	set(QT_PLUGINDIR "${MINGW_PREFIX}/plugins")
	set(DLL_QCA "${DLLDIR}/libqca*.dll")
	set(QCA_CRYPTO_DIR "${DLLDIR_QCA}/qca-qt6/crypto")
	set(QT_DLLS
		${DLLDIR_QT}/Qt6Core.dll
		${DLLDIR_QT}/Qt6Core5Compat.dll
		${DLLDIR_QT}/Qt6Gui.dll
		${DLLDIR_QT}/Qt6Svg.dll
		${DLLDIR_QT}/Qt6Widgets.dll
		${DLLDIR_QT}/Qt6Network.dll
		${DLLDIR_QT}/Qt6Concurrent.dll
		${DLLDIR_QT}/Qt6HttpServer.dll
		${DLLDIR_QT}/Qt6WebSockets.dll)
endif()

if(WITH_TRANSLATIONS)
	set(COPY_TRANSLATIONS_COMMAND cp translations/*qm ${WINDOWS_INSTALL_FILES}/translations/)
else()
	set(COPY_TRANSLATIONS_COMMAND ${CMAKE_COMMAND} -E true)
endif()

if(WITH_LDAP)
	set(COPY_LDAP_DLLS_COMMAND cp ${DLLDIR}/libsasl*.dll ${DLLDIR}/libldap*.dll ${DLLDIR}/liblber*.dll ${WINDOWS_INSTALL_FILES})
else()
	set(COPY_LDAP_DLLS_COMMAND ${CMAKE_COMMAND} -E true)
endif()

if(WITH_BUILTIN_LIBVNC)
	set(COPY_LIBVNC_DLLS_COMMAND ${CMAKE_COMMAND} -E true)
else()
	set(COPY_LIBVNC_DLLS_COMMAND cp ${DLLDIR}/libvncclient*.dll ${DLLDIR}/libvncserver*.dll ${WINDOWS_INSTALL_FILES})
endif()

if(EXISTS "${DLLDIR}/libjpeg-turbo")
	set(DLLDIR_JPEG "${DLLDIR}/libjpeg-turbo")
else()
	set(DLLDIR_JPEG "${DLLDIR}")
endif()

set(COPY_RUNTIME_DLLS_COMMAND cp
	${DLLDIR}/icudt*.dll
	${DLLDIR}/icuin*.dll
	${DLLDIR}/icuuc*.dll
	${DLLDIR}/libbrotlicommon*.dll
	${DLLDIR}/libbrotlidec*.dll
	${DLLDIR}/libbz2*.dll
	${DLLDIR}/libfreetype*.dll
	${DLLDIR}/libglib*.dll
	${DLLDIR}/libharfbuzz-0.dll
	${DLLDIR}/libiconv*.dll
	${DLLDIR}/libintl*.dll
	${DLLDIR}/libjpeg*.dll
	${DLLDIR}/libpcre2-16*.dll
	${DLLDIR}/libpcre2-8*.dll
	${DLLDIR}/libzstd*.dll
	${WINDOWS_INSTALL_FILES})

find_program(UNIX2DOS_EXECUTABLE unix2dos)
if(UNIX2DOS_EXECUTABLE)
	set(CONVERT_TEXT_FILES_COMMAND ${UNIX2DOS_EXECUTABLE} ${WINDOWS_INSTALL_FILES}/*.TXT)
else()
	set(CONVERT_TEXT_FILES_COMMAND ${CMAKE_COMMAND} -E true)
endif()

if(NOT WITH_LDAP)
	set(REMOVE_DISABLED_LDAP_PLUGIN_COMMAND rm -f ${WINDOWS_INSTALL_FILES}/plugins/ldap.dll ${WINDOWS_INSTALL_FILES}/plugins/libkldap-light.dll)
else()
	set(REMOVE_DISABLED_LDAP_PLUGIN_COMMAND ${CMAKE_COMMAND} -E true)
endif()
if(NOT WITH_WEBAPI)
	set(REMOVE_DISABLED_WEBAPI_PLUGIN_COMMAND rm -f ${WINDOWS_INSTALL_FILES}/plugins/webapi.dll ${WINDOWS_INSTALL_FILES}/plugins/libqthttpserver.dll)
else()
	set(REMOVE_DISABLED_WEBAPI_PLUGIN_COMMAND ${CMAKE_COMMAND} -E true)
endif()

# Check if MXE_PATH is provided (either as a CMake variable or environment variable)
if(NOT MXE_PATH AND DEFINED ENV{MXE_PATH})
	set(MXE_PATH "$ENV{MXE_PATH}")
endif()

string(REGEX MATCH "^[^.]+" GCC_VERSION_MAJOR ${CMAKE_CXX_COMPILER_VERSION})

if(NOT DLLDIR_GCC)
	if(MXE_PATH)
		# Search in MINGW_PREFIX/bin first (MXE shared builds copy them there)
		if(EXISTS "${MINGW_PREFIX}/bin/libstdc++-6.dll")
			set(DLLDIR_GCC "${MINGW_PREFIX}/bin")
		else()
			# Otherwise look in MXE's internal gcc directory
			set(DLLDIR_GCC "${MXE_PATH}/usr/lib/gcc/${MINGW_TARGET}/${GCC_VERSION_MAJOR}")
			if(NOT EXISTS "${DLLDIR_GCC}/libstdc++-6.dll")
				set(DLLDIR_GCC "${MINGW_PREFIX}/bin")
			endif()
		endif()
	else()
		set(DLLDIR_GCC "/usr/lib/gcc/${MINGW_TARGET}/${GCC_VERSION_MAJOR}-posix")
	endif()
endif()

# For MXE shared target, zlib and winpthread DLLs are under MINGW_PREFIX/bin instead of MINGW_PREFIX/lib
if(MXE_PATH)
	set(DLLDIR_ZLIB "${DLLDIR}")
	set(DLLDIR_PTHREAD "${DLLDIR}")
	if(EXISTS "${DLLDIR}/qca-qt6/crypto/libqca-ossl.dll")
		set(DLLDIR_QCA "${DLLDIR}")
	else()
		set(DLLDIR_QCA "${DLLDIR_LIB}")
	endif()
else()
	set(DLLDIR_ZLIB "${DLLDIR_LIB}")
	set(DLLDIR_PTHREAD "${DLLDIR_LIB}")
	set(DLLDIR_QCA "${DLLDIR_LIB}")
endif()

if(VEYON_BUILD_WIN64)
	set(DLL_GCC "libgcc_s_seh-1.dll")
	set(DLL_DDENGINE "ddengine64.dll")
else()
	set(DLL_GCC "libgcc_s_dw2-1.dll")
	set(DLL_DDENGINE "ddengine.dll")
endif()

add_custom_target(windows-binaries
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config $<CONFIGURATION>
	COMMAND rm -rf ${WINDOWS_INSTALL_FILES}*
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/interception
	COMMAND cp ${CMAKE_SOURCE_DIR}/3rdparty/interception/* ${WINDOWS_INSTALL_FILES}/interception
	COMMAND cp ${CMAKE_SOURCE_DIR}/3rdparty/ddengine/${DLL_DDENGINE} ${WINDOWS_INSTALL_FILES}
	COMMAND cp core/veyon-core.dll ${WINDOWS_INSTALL_FILES}
	COMMAND find . -mindepth 2 -name 'veyon-*.exe' ! -path './${WINDOWS_INSTALL_FILES}/*' -exec cp '{}' ${WINDOWS_INSTALL_FILES}/ '\;'
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/plugins
	COMMAND find plugins/ -name '*.dll' -exec cp '{}' ${WINDOWS_INSTALL_FILES}/plugins/ '\;'
	COMMAND ${REMOVE_DISABLED_LDAP_PLUGIN_COMMAND}
	COMMAND ${REMOVE_DISABLED_WEBAPI_PLUGIN_COMMAND}
	COMMAND find ${WINDOWS_INSTALL_FILES}/plugins -maxdepth 1 -name 'lib*.dll' -exec mv '{}' ${WINDOWS_INSTALL_FILES}/ '\;'
	COMMAND find ${WINDOWS_INSTALL_FILES}/plugins -maxdepth 1 -name 'vnchooks.dll' -exec mv '{}' ${WINDOWS_INSTALL_FILES}/ '\;'
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/translations
	COMMAND ${COPY_TRANSLATIONS_COMMAND}
	COMMAND cp ${DLLDIR_JPEG}/libjpeg*.dll ${WINDOWS_INSTALL_FILES}
	COMMAND ${COPY_RUNTIME_DLLS_COMMAND}
	COMMAND cp ${DLLDIR}/libpng*.dll ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${DLLDIR}/libcrypto*.dll ${DLLDIR}/libssl*.dll ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${DLL_QCA} ${WINDOWS_INSTALL_FILES}
	COMMAND ${COPY_LDAP_DLLS_COMMAND}
	COMMAND cp ${DLLDIR}/interception.dll ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${DLLDIR}/liblzo*.dll ${WINDOWS_INSTALL_FILES}
	COMMAND ${COPY_LIBVNC_DLLS_COMMAND}
	COMMAND cp ${DLLDIR_ZLIB}/zlib*.dll ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${DLLDIR_PTHREAD}/libwinpthread*.dll ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${DLLDIR_GCC}/libstdc++-6.dll ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${DLLDIR_GCC}/libssp-0.dll ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${DLLDIR_GCC}/${DLL_GCC} ${WINDOWS_INSTALL_FILES}
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/crypto
	COMMAND cp ${QCA_CRYPTO_DIR}/libqca-ossl.dll ${WINDOWS_INSTALL_FILES}/crypto
	COMMAND cp ${QT_DLLS} ${WINDOWS_INSTALL_FILES}
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/imageformats
	COMMAND cp ${QT_PLUGINDIR}/imageformats/qjpeg.dll ${QT_PLUGINDIR}/imageformats/qsvg.dll ${WINDOWS_INSTALL_FILES}/imageformats
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/iconengines
	COMMAND cp ${QT_PLUGINDIR}/iconengines/qsvgicon.dll ${WINDOWS_INSTALL_FILES}/iconengines
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/platforms
	COMMAND cp ${QT_PLUGINDIR}/platforms/qwindows.dll ${WINDOWS_INSTALL_FILES}/platforms
	COMMAND mkdir -p ${WINDOWS_INSTALL_FILES}/styles
	COMMAND cp ${QT_PLUGINDIR}/styles/*.dll ${WINDOWS_INSTALL_FILES}/styles
	COMMAND ${MINGW_TOOL_PREFIX}strip ${WINDOWS_INSTALL_FILES}/*.dll ${WINDOWS_INSTALL_FILES}/*.exe ${WINDOWS_INSTALL_FILES}/plugins/*.dll ${WINDOWS_INSTALL_FILES}/imageformats/*.dll ${WINDOWS_INSTALL_FILES}/iconengines/*.dll ${WINDOWS_INSTALL_FILES}/platforms/*.dll ${WINDOWS_INSTALL_FILES}/styles/*.dll ${WINDOWS_INSTALL_FILES}/crypto/*.dll
	COMMAND cp ${CMAKE_SOURCE_DIR}/COPYING ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${CMAKE_SOURCE_DIR}/COPYING ${WINDOWS_INSTALL_FILES}/LICENSE.TXT
	COMMAND cp ${CMAKE_SOURCE_DIR}/README.md ${WINDOWS_INSTALL_FILES}/README.TXT
	COMMAND ${CONVERT_TEXT_FILES_COMMAND}
	COMMAND cp -ra ${CMAKE_SOURCE_DIR}/nsis ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${CMAKE_BINARY_DIR}/nsis/veyon.nsi ${WINDOWS_INSTALL_FILES}
	COMMAND find ${WINDOWS_INSTALL_FILES} -ls
)

add_custom_target(create-windows-installer
	COMMAND makensis ${WINDOWS_INSTALL_FILES}/veyon.nsi
	COMMAND mv ${WINDOWS_INSTALL_FILES}/edumonitor-*setup.exe .
	COMMAND rm -rf ${WINDOWS_INSTALL_FILES}
	DEPENDS windows-binaries
)

add_custom_target(prepare-dev-nsi
	COMMAND sed -i ${WINDOWS_INSTALL_FILES}/veyon.nsi -e "s,/SOLID lzma,zlib,g"
	DEPENDS windows-binaries)

add_custom_target(dev-nsi
	DEPENDS prepare-dev-nsi create-windows-installer
)
