AM_CPPFLAGS = \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/tcc \
	-I$(top_srcdir)

AM_CFLAGS = $(WARN_CFLAGS)

LDADD = \
	../../tcc/libdrm_tcc.la \
	../../libdrm.la

noinst_PROGRAMS = \
	tcc_drm_test
