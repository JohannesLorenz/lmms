INCLUDE(BuildPlugin)

BUILD_PLUGIN(oscplugin oscplugin.cpp oscplugin.h MOCFILES oscplugin.h
	EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")

