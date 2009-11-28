MACRO(INIT_VIDEO_FILTER_GTK  lib  _srcsGtk )

IF (DO_GTK)
	IF (GETTEXT_FOUND)
		ADD_DEFINITIONS("-DHAVE_GETTEXT")
		INCLUDE_DIRECTORIES(${GETTEXT_INCLUDE_DIR})
	ENDIF (GETTEXT_FOUND)

        ADD_DEFINITIONS(${GTK_CFLAGS})

        ADD_LIBRARY(${lib} SHARED ${ARGN} ${_srcsGtk})
        INCLUDE_DIRECTORIES(${AVIDEMUX_TOP_SOURCE_DIR}/avidemux/gtk/ADM_UIs/include/)
        TARGET_LINK_LIBRARIES( ${lib} ADM_UIGtk6 ADM_render6_gtk)
        TARGET_LINK_LIBRARIES(${lib} ${GTK_LDFLAGS} ${GTHREAD_LDFLAGS})

        IF (GETTEXT_FOUND)
            TARGET_LINK_LIBRARIES(${lib} ${GETTEXT_LIBRARY_DIR})
        ENDIF(GETTEXT_FOUND)
		ADD_TARGET_CFLAGS(${lib} "-DADM_UI_TYPE_BUILD=2")
        INIT_VIDEO_FILTER_INTERNAL(${lib})
        INSTALL_VIDEO_FILTER_INTERNAL(${lib})
ENDIF (DO_GTK)
ENDMACRO(INIT_VIDEO_FILTER_GTK)
