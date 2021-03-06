[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

uidir = $(datadir)/[+NameHLower+]/ui
ui_DATA = [+NameHLower+].ui

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$([+NameCUpper+]_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

VALAFLAGS = [+IF (not (= (get "PackageModule2") ""))+] --pkg [+(string-substitute (get "PackageModule2") " " " --pkg ")+] [+ENDIF+] \
	--pkg gtk+-3.0

bin_PROGRAMS = [+NameHLower+]

[+NameCLower+]_SOURCES = \
	[+NameHLower+].vala

[+NameCLower+]_LDFLAGS = \
	-Wl,--export-dynamic

[+NameCLower+]_LDADD = $([+NameCUpper+]_LIBS)

EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(datadir)/[+NameHLower+]
