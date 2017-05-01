INCLUDE(BuildPlugin)

BUILD_PLUGIN(oscinstrument oscinstrument.cpp oscinstrument.h
	MOCFILES oscinstrument.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")

