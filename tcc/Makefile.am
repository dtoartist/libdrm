AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir) \
	$(PTHREADSTUBS_CFLAGS) \
	-I$(top_srcdir)/include/drm

libdrm_tcc_la_LTLIBRARIES = libdrm_tcc.la
libdrm_tcc_ladir = $(libdir)
libdrm_tcc_la_LDFLAGS = -version-number 1:0:0 -no-undefined
libdrm_tcc_la_LIBADD = ../libdrm.la @PTHREADSTUBS_LIBS@

libdrm_tcc_la_SOURCES = \
	tcc_drm.c

libdrm_tcccommonincludedir = ${includedir}/tcc
libdrm_tcccommoninclude_HEADERS = tcc_drm.h

libdrm_tccincludedir = ${includedir}/libdrm
libdrm_tccinclude_HEADERS = tcc_drmif.h

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdrm_tcc.pc

TESTS = tcc-symbol-check
EXTRA_DIST = $(TESTS)
