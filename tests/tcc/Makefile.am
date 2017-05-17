AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-I $(top_srcdir)/include/drm \
	-I $(top_srcdir)/libkms/ \
	-I $(top_srcdir)/tcc \
	-I $(top_srcdir)

bin_PROGRAMS =
noinst_PROGRAMS =

if HAVE_LIBKMS
if HAVE_INSTALL_TESTS
bin_PROGRAMS += \
	tcc_drm_test
else
noinst_PROGRAMS += \
	tcc_drm_test
endif
endif

tcc_drm_test_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/libkms/libkms.la \
	$(top_builddir)/tcc/libdrm_tcc.la

tcc_drm_test_SOURCES = \
	tcc_drm_test.c

