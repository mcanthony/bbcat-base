
AUTOMAKE_OPTIONS = foreign

include doxygen.am

SUBDIRS = src

base_DATA = share/licences.txt

basedir = $(prefix)/share/$(PACKAGE)

EXTRA_DIST = README COPYING autogen.sh $(base_DATA)

pkgconfig_DATA = bbcat-base-$(BBCAT_BASE_MAJORMINOR).pc

bbcat-base-$(BBCAT_BASE_MAJORMINOR).pc: bbcat-base.pc
	cp bbcat-base.pc bbcat-base-$(BBCAT_BASE_MAJORMINOR).pc

bbcat-base-$(BBCAT_BASE_MAJORMINOR)-uninstalled.pc: bbcat-base-uninstalled.pc
	cp bbcat-base-uninstalled.pc bbcat-base-$(BBCAT_BASE_MAJORMINOR)-uninstalled.pc

BUILT_SOURCES = bbcat-base-$(BBCAT_BASE_MAJORMINOR)-uninstalled.pc

CLEANFILES = bbcat-base-$(BBCAT_BASE_MAJORMINOR).pc bbcat-base-$(BBCAT_BASE_MAJORMINOR)-uninstalled.pc src/version.h

ACLOCAL_FLAGS = -I m4
