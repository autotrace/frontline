INCLUDES =					\
         -I$(top_srcdir)			\
         $(LIBART2_CFLAGS)			\
         $(GNOME_INCLUDEDIR)			\
         $(AUTOTRACE_CFLAGS)

AM_CPPFLAGS =						\
	-DG_LOG_DOMAIN=\"frontline\"			\
	-DLOCALEDIR=\""$(datadir)/locale"\"             \
        -DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"

AM_CFLAGS=-g -Wall 

lib_LIBRARIES=libfrontline.a
bin_PROGRAMS= frontline

frontlineincludedir=$(includedir)/frontline

frontlineinclude_HEADERS =			\
	frontline.h				\
	fl_opt.h				\
	fl_dialog.h				\
	fl_prog.h				\
	fl_preview.h				\
	fl_ext.h				\
	fl_fsel.h				\
	fl_ask.h

libfrontline_a_SOURCES=				\
	fl_dialog.c				\
	fl_utils.c				\
	fl_opt.c				\
	fl_opt_priv.c				\
	fl_prog.c				\
	fl_fsel.c				\
	fl_opt_undo.c				\
	at_opts_io.c				\
	at_ext.c				\
	fl_preview.c				\
	fl_splines.c				\
	fl_ext.c				\
	fl_ask.c				\
	curve.c					\
	canvas-bpath.c

noinst_HEADERS    = private.h curve.h canvas-bpath.h
frontline_SOURCES = frontline.c

# Don't use popt in gnome. Instead use libpop directly
# because popt in gnome-1.4 doesn't support FLOAT args.
frontline_LDADD= $(LIBART2_LIBS)				\
                 $(top_builddir)/frontline/libfrontline.a	\
		 $(INTLLIBS)					\
                 $(top_builddir)/gundo/libgundo.a		\
                 -lpopt						\
                 $(GNOME_LIBDIR)				\
                 $(GNOMEUI_LIBS)				\
                 $(AUTOTRACE_LIBS) 
# /home/jet/src/popt-1.6.3/.libs/libpopt.a \
#-lpopt \

