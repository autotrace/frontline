SUBDIRS = intl gundo frontline gimp po
EXTRA_DIST=frontline-config.af frontline.m4 \
frontline.png frontline.desktop frontline.keys.in frontline.mime \
frontline.spec.in fl-splash.png BUGS

# -config
bin_SCRIPTS=frontline-config
BUILT_SOURCES=frontline-config
frontline-config: frontline-config.in
frontline-config.in: frontline-config.af
	autofig $<
# m4
m4datadir = $(datadir)/aclocal
m4data_DATA = frontline.m4

# pc
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= frontline.pc

# png
appicondir = $(datadir)/pixmaps
appicon_DATA = frontline.png fl-splash.png

# desktop
Graphicsdir = $(datadir)/gnome/apps/Graphics
Graphics_DATA = frontline.desktop

# Mime for gmc. This doesn't work well on nautilus/gnome-vfs.
Mimedir = $(datadir)/mime-info
Mime_DATA = frontline.keys frontline.mime

frontline.keys: frontline.keys.in Makefile
	sed -e 's^\@icondir\@^$(appicondir)^g' < $(srcdir)/frontline.keys.in > frontline.keys.tmp \
	&& mv frontline.keys.tmp frontline.keys 

dist-hook: frontline.spec
	cp frontline.spec $(distdir)

rpm: dist frontline.spec
	rpm -bb frontline.spec

ACLOCAL_AMFLAGS = -I m4