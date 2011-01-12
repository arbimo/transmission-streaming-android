ACLOCAL_AMFLAGS = -I m4

if BUILD_CLI
  CLI_DIR = cli
endif
if BUILD_DAEMON
if !WIN32
  DAEMON_DIR = daemon
endif
endif

SUBDIRS = \
  doc \
  third-party \
  libtransmission \
  $(DAEMON_DIR) \
  $(CLI_DIR) \
  $(GTK_DIR) \
  $(MAC_DIR) \
  web

EXTRA_DIST = \
  qt \
  NEWS \
  AUTHORS \
  COPYING \
  README \
  autogen.sh \
  update-version-h.sh \
  Transmission.xcodeproj/project.pbxproj

dist-hook:
	rm -rf `find $(distdir)/qt -name .svn`


DISTCLEANFILES = \
  intltool-extract \
  intltool-merge \
  intltool-update 
