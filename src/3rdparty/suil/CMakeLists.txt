# CMake wrapper for the suil meson build file
# based on suil/e0a874cd8199b81f1a73d321469d9013c00a6560

## START SINGLE
# CMAKE_MINIMUM_REQUIRED(VERSION 3.13)
#
# project(
#     suil
#     VERSION 0.10.26
#     LANGUAGES C CXX
# )

set(SUIL_VERSION 0.10.26)
set(SUIL_VERSION_MAJOR 0)

#
# set(QT_VERSION_MAJOR 5)
#
# find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets X11Extras)
##END SINGLE

if(CMAKE_BUILD_TYPE MATCHES "Release")
	add_compile_definitions(NDEBUG)
endif()

include(GNUInstallDirs)
set(SUIL_VERSIONED_NAME "suil-${SUIL_VERSION_MAJOR}")
set(SUIL_MODULE_DIR "${CMAKE_INSTALL_LIBDIR}/${SUIL_VERSIONED_NAME}")

if(APPLE)
	enable_language(OBJCXX)
endif()

########################
# Warning Suppressions #
########################

# warning_level = get_option('warning_level')
#
# # C
# c_suppressions = []
# if cc.get_id() in ['clang', 'emscripten']
#   if warning_level == 'everything'
#     c_suppressions += [
#       '-Wno-atomic-implicit-seq-cst',
#       '-Wno-cast-function-type-strict',
#       '-Wno-cast-qual',
#       '-Wno-declaration-after-statement',
#       '-Wno-disabled-macro-expansion',
#       '-Wno-padded',
#       '-Wno-reserved-id-macro',
#       '-Wno-unsafe-buffer-usage',
#       '-Wno-variadic-macros',
#     ]
#
#     if not meson.is_cross_build()
#       c_suppressions += ['-Wno-poison-system-directories']
#     endif
#   endif
#
#   if host_machine.system() == 'windows'
#     c_suppressions += [
#       '-Wno-deprecated-declarations',
#       '-Wno-nonportable-system-include-path',
#     ]
#   endif
#
#   if warning_level in ['everything', '3']
#     c_suppressions += ['-Wno-nullability-extension']
#   endif
#
# elif cc.get_id() == 'gcc'
#   if warning_level == 'everything'
#     c_suppressions += [
#       '-Wno-inline',
#       '-Wno-padded',
#       '-Wno-suggest-attribute=const',
#       '-Wno-suggest-attribute=pure',
#     ]
#   endif
#
# elif cc.get_id() == 'msvc'
#   c_suppressions += [
#     '/experimental:external',
#     '/external:W0',
#     '/external:anglebrackets',
#   ]
#
#   if warning_level == 'everything'
#     c_suppressions += [
#       '/wd4191', # unsafe function conversion
#       '/wd4514', # unreferenced inline function has been removed
#       '/wd4710', # function not inlined
#       '/wd4820', # padding added after construct
#       '/wd5045', # will insert Spectre mitigation for memory load
#     ]
#   endif
#
#   if warning_level in ['everything', '3', '2']
#     c_suppressions += [
#       '/wd4996', # function or variable may be unsafe
#     ]
#   endif
# endif
#
# c_suppressions = cc.get_supported_arguments(c_suppressions)
#
# # C++
# if is_variable('cpp')
#   cpp_suppressions = []
#
#   if cpp.get_id() in ['clang', 'emscripten']
#     if warning_level == 'everything'
#       cpp_suppressions += [
#         '-Wno-atomic-implicit-seq-cst',
#         '-Wno-c++98-compat',
#         '-Wno-c++98-compat-pedantic',
#         '-Wno-cast-function-type-strict',
#         '-Wno-cast-qual',
#         '-Wno-disabled-macro-expansion',
#         '-Wno-old-style-cast',
#         '-Wno-padded',
#         '-Wno-reserved-id-macro',
#         '-Wno-unsafe-buffer-usage',
#         '-Wno-variadic-macros',
#         '-Wno-zero-as-null-pointer-constant',
#       ]
#     endif
#
#     if warning_level in ['everything', '3']
#       cpp_suppressions += ['-Wno-nullability-extension']
#     endif
#
#   elif cpp.get_id() == 'gcc'
#     if warning_level == 'everything'
#       cpp_suppressions += [
#         '-Wno-arith-conversion',
#         '-Wno-cast-qual',
#         '-Wno-inline',
#         '-Wno-padded',
#         '-Wno-suggest-attribute=const',
#         '-Wno-suggest-attribute=pure',
#         '-Wno-useless-cast',
#         '-Wno-volatile',
#       ]
#     endif
#   endif
#
#   cpp_suppressions = cpp.get_supported_arguments(cpp_suppressions)
# endif
#
# # Objective C++
# if is_variable('objcpp')
#   objcpp_suppressions = []
#
#   if objcpp.get_id() in ['clang', 'emscripten']
#     if warning_level == 'everything'
#       objcpp_suppressions += [
#         '-Wno-c++98-compat-pedantic',
#         '-Wno-deprecated-declarations',
#         '-Wno-old-style-cast',
#         '-Wno-padded',
#         '-Wno-reserved-id-macro',
#         '-Wno-weak-vtables',
#         '-Wno-zero-as-null-pointer-constant',
#       ]
#
#       if not meson.is_cross_build()
#         objcpp_suppressions += ['-Wno-poison-system-directories']
#       endif
#     endif
#
#   elif objcpp.get_id() == 'gcc'
#     if warning_level == 'everything'
#       objcpp_suppressions = (
#         gcc_common_warnings + ['-Wno-direct-ivar-access']
#       )
#     endif
#   endif
#
#   objcpp_suppressions = objcpp.get_supported_arguments(objcpp_suppressions)
# endif

##########################
# Platform Configuration #
##########################

# Platform defines
add_compile_definitions(SUIL_MODULE_DIR="${CMAKE_INSTALL_PREFIX}/${SUIL_MODULE_DIR}")

# Versioned shared library handling
if(WIN32)
	# Meson default_library=both check
	if(BUILD_SHARED_LIBS AND BUILD_STATIC_LIBS)
		message(FATAL_ERROR "default_library=both is not supported on Windows")
	endif()
	set(SUIL_SOVERSION "")
else()
	set(SUIL_SOVERSION ${SUIL_VERSION_MAJOR})
endif()

################
# Dependencies #
################

#dl_dep = cc.find_library('dl', required: false)
# CMAKE_DL_LIBS

# Lv2, Qt5/6 are set by LMMS' CMakeLists

# LMMS_HAVE_LV2
if (LV2_VERSION AND LV2_VERSION VERSION_GREATER_EQUAL 1.18.4)
	SET(SUIL_HAVE_LV2)
endif()

find_package(X11)




if(APPLE AND Qt5Widgets_FOUND)
    include(CheckIncludeFileCXX)
    check_include_file_cxx(QMacCocoaViewContainer HAVE_QMACCOCOAVIEWCONTAINER)
endif()

###########
# Library #
###########

# Sources
set(SUIL_CORE_SOURCES
						${CMAKE_CURRENT_SOURCE_DIR}/suil/src/host.c
						${CMAKE_CURRENT_SOURCE_DIR}/suil/src/instance.c)
#set(C_HEADERS include/suil/suil.h)
set(SUIL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/suil/include)

# Extra compile args
#set(extra_c_args "")
#if(NOT BUILD_SHARED_LIBS)
   add_compile_definitions(SUIL_STATIC)
#endif()

# Library
add_library(${SUIL_VERSIONED_NAME} ${SUIL_CORE_SOURCES})

target_include_directories(${SUIL_VERSIONED_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${SUIL_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include>
)

# Compile definitions
target_compile_definitions(${SUIL_VERSIONED_NAME}
    PRIVATE
        SUIL_INTERNAL
)

# Dependencies
target_link_libraries(${SUIL_VERSIONED_NAME}
    PRIVATE
        ${CMAKE_DL_LIBS}
        ${LV2_LIBRARIES}
)

# Visibility (gnu_symbol_visibility: 'hidden')
set_target_properties(${SUIL_VERSIONED_NAME} PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

# Versioning
set_target_properties(${SUIL_VERSIONED_NAME} PROPERTIES
    VERSION ${SUIL_VERSION}
    SOVERSION ${SUIL_SOVERSION}
)

# macOS dylib versions
if(APPLE)
    set_target_properties(${SUIL_VERSIONED_NAME} PROPERTIES
        MACHO_CURRENT_VERSION ${SUIL_VERSION}
        MACHO_COMPATIBILITY_VERSION "${SUIL_VERSION_MAJOR}.0.0"
    )
endif()

# Install

# install(TARGETS ${SUIL_VERSIONED_NAME}
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
# )
#
# install(FILES ${C_HEADERS}
#     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/suil
# )
#

add_library(SUIL_DEP INTERFACE)

target_link_libraries(SUIL_DEP
    INTERFACE
        ${SUIL_VERSIONED_NAME}
        ${CMAKE_DL_LIBS}
        ${LV2_LIBRARIES}
)

target_include_directories(SUIL_DEP
    INTERFACE
        ${SUIL_INCLUDE_DIRS}
)

#####################
# Host Init Modules #
#####################

    add_library(suil_x11 MODULE
        ${CMAKE_CURRENT_SOURCE_DIR}/suil/src/x11.c
    )
    target_include_directories(suil_x11
        PRIVATE
            ${SUIL_INCLUDE_DIRS}
    )
	target_link_libraries(suil_x11
		PRIVATE
			${LV2_LIBRARIES}
	)
	    set_target_properties(suil_x11 PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )

###################
# Wrapper Modules #
###################


    add_library(suil_x11_in_qt${QT_VERSION_MAJOR} MODULE
        ${CMAKE_CURRENT_SOURCE_DIR}/suil/src/x11_in_qt.cpp
    )

    target_include_directories(suil_x11_in_qt${QT_VERSION_MAJOR}
        PRIVATE
            ${SUIL_INCLUDE_DIRS}
    )

	target_link_libraries(suil_x11_in_qt${QT_VERSION_MAJOR}
		PRIVATE
			${LV2_LIBRARIES}
			Qt${QT_VERSION_MAJOR}::Widgets
	)

    if(QT_VERSION_MAJOR EQUAL 5)
		target_link_libraries(suil_x11_in_qt${QT_VERSION_MAJOR}
			PRIVATE
				Qt5::X11Extras
				X11::X11
		)
	endif()

    set_target_properties(suil_x11_in_qt${QT_VERSION_MAJOR} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )

if(APPLE AND SUIL_WANT_COCOA AND ${QT_VERSION_MAJOR} EQUAL 5)

add_library(suil_cocoa_in_qt${QT_VERSION_MAJOR} MODULE
        ${CMAKE_CURRENT_SOURCE_DIR}/suil/src/cocoa_in_qt${QT_VERSION_MAJOR}.mm
    )

    target_include_directories(suil_cocoa_in_qt${QT_VERSION_MAJOR}
        PRIVATE
            ${SUIL_INCLUDE_DIRS}
    )

target_link_libraries(suil_cocoa_in_qt${QT_VERSION_MAJOR}
    PRIVATE
        ${LV2_LIBRARIES}
        Qt${QT_VERSION_MAJOR}
)

    set_target_properties(suil_cocoa_in_qt${QT_VERSION_MAJOR} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )

endif()

#set (SUIL_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/suil/include")
#set (SUIL_LIBRARY_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
#set (SUIL_LIBRARIES "suil-0")
#set (SUIL_LIBRARIES "suil_x11_in_qt${QT_VERSION_MAJOR}")


# if qt5_dep.found() and qt5_x11_dep.found() and x11_dep.found()
#   shared_module(
#     'suil_x11_in_qt5',
#     files('src/x11_in_qt.cpp'),
#     cpp_args: cpp_suppressions + platform_defines,
#     dependencies: [lv2_dep, qt5_dep, qt5_x11_dep, x11_dep],
#     gnu_symbol_visibility: 'hidden',
#     include_directories: SUIL_INCLUDE_DIRS,
#     install: true,
#     install_dir: suil_module_dir,
#   )
# endif
# #
# if host_machine.system() == 'darwin'
#   if qt5_dep.found() and not get_option('cocoa').disabled()
#     cocoa_suppressions = [
#       '-Wno-deprecated-declarations',
#     ]
#
#     shared_module(
#       'suil_cocoa_in_qt5',
#       files('src/cocoa_in_qt5.mm'),
#       dependencies: [lv2_dep, qt5_dep],
#       gnu_symbol_visibility: 'hidden',
#       include_directories: SUIL_INCLUDE_DIRS,
#       install: true,
#       install_dir: suil_module_dir,
#       objcpp_args: cocoa_suppressions + objcpp_suppressions + platform_defines,
#     )
#   endif
# endif
# #
# if qt6_dep.found()
#   shared_module(
#     'suil_x11_in_qt6',
#     files('src/x11_in_qt.cpp'),
#     cpp_args: cpp_suppressions + platform_defines,
#     dependencies: [lv2_dep, qt6_dep],
#     gnu_symbol_visibility: 'hidden',
#     include_directories: SUIL_INCLUDE_DIRS,
#     install: true,
#     install_dir: suil_module_dir,
#   )
# endif
#
# #########
# # Tests #
# #########
#
# if not get_option('tests').disabled()
#   subdir('test/headers')
# endif

