if HAVE_GIMP

INCLUDES  =					\
	-I$(top_srcdir)				\
        $(LIBART2_CFLAGS)			\
	$(GIMP_CFLAGS)				\
        $(GNOME_INCLUDEDIR)			\
        $(AUTOTRACE_CFLAGS)

AM_CPPFLAGS = -DG_LOG_DOMAIN=\"trace-plugin\"		\
	      -DLOCALEDIR=\""$(datadir)/locale"\"

AM_CFLAGS   = -g -Wall

libexecdir = $(GIMP_PLUGIN_DIR)/plug-ins
libexec_PROGRAMS = trace
trace_SOURCES    = trace.c
trace_LDADD      = $(LIBART2_LIBS)\
	$(top_builddir)/frontline/libfrontline.a	\
	$(INTLLIBS)					\
	$(top_builddir)/gundo/libgundo.a		\
	-lpopt						\
	$(GNOME_LIBDIR)					\
	$(GNOMEUI_LIBS)					\
	$(GIMP_LIBS)					\
	$(AUTOTRACE_LIBS) 
endif
