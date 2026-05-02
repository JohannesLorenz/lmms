# CMake wrapper for the suil meson build file
# based on suil/e0a874cd8199b81f1a73d321469d9013c00a6560

set(SUIL_VERSION 0.10.26)
set(SUIL_VERSION_MAJOR 0)

if(CMAKE_BUILD_TYPE MATCHES "Release")
	add_compile_definitions(NDEBUG)
endif()

include(GNUInstallDirs)
set(SUIL_VERSIONED_NAME "suil-${SUIL_VERSION_MAJOR}")
set(SUIL_MODULE_DIR "${CMAKE_INSTALL_LIBDIR}/${SUIL_VERSIONED_NAME}")  # "lib/suil-0"

if(APPLE)
	enable_language(OBJCXX)
endif()

########################
# Warning Suppressions #
########################

set(c_suppressions "")
set(cpp_suppressions "")
set(objcpp_suppressions "")

# Detect compiler
if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR
   CMAKE_C_COMPILER_ID STREQUAL "Emscripten")

    list(APPEND c_suppressions
        -Wall
        -Wextra
        -Weverything
        -Wno-atomic-implicit-seq-cst
        -Wno-cast-function-type-strict
        -Wno-cast-qual
        -Wno-declaration-after-statement
        -Wno-disabled-macro-expansion
        -Wno-padded
        -Wno-reserved-id-macro
        -Wno-unsafe-buffer-usage
        -Wno-variadic-macros
        -Wno-nullability-extension
    )

    if(NOT CMAKE_CROSSCOMPILING)
        list(APPEND c_suppressions
            -Wno-poison-system-directories
        )
    endif()

    if(WIN32)
        list(APPEND c_suppressions
            -Wno-deprecated-declarations
            -Wno-nonportable-system-include-path
        )
    endif()

    list(APPEND cpp_suppressions
        -Wall
        -Wextra
        -Weverything
        -Wno-atomic-implicit-seq-cst
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-cast-function-type-strict
        -Wno-cast-qual
        -Wno-disabled-macro-expansion
        -Wno-old-style-cast
        -Wno-padded
        -Wno-reserved-id-macro
        -Wno-unsafe-buffer-usage
        -Wno-variadic-macros
        -Wno-zero-as-null-pointer-constant
        -Wno-nullability-extension
    )


    list(APPEND objcpp_suppressions
        -Wall
        -Wextra
        -Weverything
        -Wno-c++98-compat-pedantic
        -Wno-deprecated-declarations
        -Wno-old-style-cast
        -Wno-padded
        -Wno-reserved-id-macro
        -Wno-weak-vtables
        -Wno-zero-as-null-pointer-constant
    )

    if(NOT CMAKE_CROSSCOMPILING)
        list(APPEND objcpp_suppressions
            -Wno-poison-system-directories
        )
    endif()

elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")

    list(APPEND c_suppressions
        -Wall
        -Wextra
        -Wno-inline
        -Wno-padded
        -Wno-suggest-attribute=const
        -Wno-suggest-attribute=pure
    )

    list(APPEND cpp_suppressions
        -Wall
        -Wextra
        -Wno-arith-conversion
        -Wno-cast-qual
        -Wno-inline
        -Wno-padded
        -Wno-suggest-attribute=const
        -Wno-suggest-attribute=pure
        -Wno-useless-cast
        -Wno-volatile
    )

    list(APPEND objcpp_suppressions
        -Wno-direct-ivar-access
    )

elseif(MSVC)

    list(APPEND c_suppressions
        /W4
        /permissive-
        /Wall
        /experimental:external
        /external:W0
        /external:anglebrackets
    )

    list(APPEND cpp_suppressions
        /W4
        /permissive-
        /Wall
        /wd4191  # unsafe function conversion
        /wd4514  # unreferenced inline function removed
        /wd4710  # function not inlined
        /wd4820  # padding added
        /wd5045  # Spectre mitigation
        /wd4996  # unsafe function/variable
    )

endif()

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

function(filter_supported_flags out_var lang)
    set(result "")

    foreach(flag IN LISTS ARGN)
        string(REGEX REPLACE "[^A-Za-z0-9]" "_" flag_var "${flag}")
        set(test_var "HAS_${lang}_${flag_var}")

        # Only this tiny part differs
        if(lang STREQUAL "C")
            check_c_compiler_flag("${flag}" ${test_var})
        elseif(lang STREQUAL "CXX" OR lang STREQUAL "OBJCXX")
            check_cxx_compiler_flag("${flag}" ${test_var})
        else()
            message(FATAL_ERROR "Unsupported language: ${lang}")
        endif()

        # Shared logic (only written once)
        if(${test_var})
            list(APPEND result "${flag}")
        endif()
    endforeach()

    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

filter_supported_flags(c_suppressions_filtered C ${c_suppressions})
filter_supported_flags(cpp_suppressions_filtered CXX ${cpp_suppressions})
filter_supported_flags(objcpp_suppressions_filtered OBJCXX ${objcpp_suppressions})


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

# Lv2, Qt5/6 are set by LMMS' CMakeLists

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
set(SUIL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/suil/include)
set(SUIL_CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/suil/src/host.c
					  ${CMAKE_CURRENT_SOURCE_DIR}/suil/src/instance.c)
set(C_HEADERS ${SUIL_INCLUDE_DIRS}/suil/suil.h)

# Always compile static in LMMS context
add_compile_definitions(SUIL_STATIC)

# Library
add_library(${SUIL_VERSIONED_NAME} ${SUIL_CORE_SOURCES})
target_compile_options(${SUIL_VERSIONED_NAME} PRIVATE ${c_suppressions_filtered})

target_include_directories(${SUIL_VERSIONED_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${SUIL_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(${SUIL_VERSIONED_NAME}
    PRIVATE SUIL_INTERNAL)
target_link_libraries(${SUIL_VERSIONED_NAME}
    PRIVATE ${CMAKE_DL_LIBS} ${LV2_LIBRARIES})
set_target_properties(${SUIL_VERSIONED_NAME} PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES)

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

###########
# Install #
###########

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

add_library(suil_x11
    MODULE
	    ${CMAKE_CURRENT_SOURCE_DIR}/suil/src/x11.c
)
install(TARGETS suil_x11 LIBRARY DESTINATION ${SUIL_MODULE_DIR})
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

install(TARGETS suil_x11_in_qt${QT_VERSION_MAJOR}
	LIBRARY DESTINATION ${SUIL_MODULE_DIR})

target_compile_options(suil_x11_in_qt${QT_VERSION_MAJOR} PRIVATE ${cpp_suppressions_filtered})

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

    install(TARGETS suil_cocoa_in_qt${QT_VERSION_MAJOR}
	    LIBRARY DESTINATION ${SUIL_MODULE_DIR})

    target_compile_options(suil_cocoa_in_qt${QT_VERSION_MAJOR} PRIVATE ${objcpp_suppressions_filtered})

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
