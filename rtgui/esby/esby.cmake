# esby fork: favorites, tool moves (up/down/left/right), trash and useful tabs, tt* tools.
# included by rtgui/CMakeLists.txt, right after the NONCLISOURCEFILES list.

set(ESBYSOURCEFILES
    esby/environment.cc
    esby/esbypreferences.cc
    esby/toolpanelcoordesby.cc
    esby/movabletoolpanel.cc
    esby/toolvboxdef.cc
    esby/variable.cc
    esby/ttdep.cc
    esby/ttfavoritecolorer.cc
    esby/ttisoprofiler.cc
    esby/ttlenscorrector.cc
    esby/ttpanelcolorer.cc
    esby/ttsaver.cc
    esby/tttabhider.cc
    esby/tttweaker.cc
    esby/ttudlrhider.cc
    esby/ttvardisplayer.cc
    )

list(APPEND NONCLISOURCEFILES ${ESBYSOURCEFILES})

# the esby options are a member of Options (options.cc), which is also part of the CLI executable
list(APPEND CLISOURCEFILES esby/esbysettings.cc)
list(APPEND NONCLISOURCEFILES esby/esbysettings.cc)

# translation file of the esby keys, loaded after the upstream language files (see esbyTranslationFile())
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/esby/languages/default" DESTINATION "${DATADIR}/esby/languages")

# the existing #include directives are kept unchanged:
# - upstream files include the esby headers by name (ex: "movabletoolpanel.h"),
# - esby files include the rtgui headers by name (ex: "toolpanel.h") or with "../rtengine/...".
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/esby)
