if HAVE_GIMP

INCLUDES  =					\
	-I$(top_srcdir)				\
	$(GIMP_CFLAGS)				\
        $(GNOME_INCLUDEDIR)			\
        $(AUTOTRACE_CFLAGS)

AM_CPPFLAGS = -DG_LOG_DOMAIN=\"trace-plugin\"
AM_CFLAGS   = -g -Wall

libexecdir = $(GIMP_PLUGIN_DIR)/plug-ins
libexec_PROGRAMS = trace
trace_SOURCES    = trace.c
trace_LDADD      = \
	$(top_builddir)/frontline/libfrontline.a	\
	$(top_builddir)/gundo/libgundo.a		\
	-lpopt						\
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(GIMP_LIBS)					\
	$(AUTOTRACE_LIBS) 
endif
