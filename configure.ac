AC_PREREQ([2.57])

m4_define([SPICE_MAJOR], 2)
m4_define([SPICE_MINOR], 0)
m4_define([SPICE_MICRO], 0)
m4_define([SPICE_COMPILE], 5)

AC_INIT(spice, [SPICE_MAJOR.SPICE_MINOR.SPICE_MICRO.SPICE_COMPILE], [], spice)

AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR(.)

AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip subdir-objects])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CC_C99
if test x"$ac_cv_prog_cc_c99" = xno; then
    AC_MSG_ERROR([C99 compiler is required.])
fi
AC_PROG_CXX
AC_PROG_INSTALL
AC_CANONICAL_HOST
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
AC_C_BIGENDIAN
AC_PATH_PROGS(PYTHON, python2 python)

AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([execinfo.h])
AC_FUNC_ALLOCA

SPICE_LT_VERSION=m4_format("%d:%d:%d", 1, 0, 2)
AC_SUBST(SPICE_LT_VERSION)

# Check for the CPU we are using
#
AC_MSG_CHECKING(for x86 or x86-64 platform)
case $host_cpu in
  i386|i486|i586|i686|i786|k6|k7|arm|armv7)
	variant=32
        ;;
  x86_64)
	variant=64
	;;
  *)
	AC_MSG_RESULT(no)
	echo Only x86 and x86-64 are supported
	exit 1
esac
AC_MSG_RESULT($variant bit)
AM_CONDITIONAL([X86_64], [test "$variant" = 64])

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    os_win32=yes
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])

case $host in
  *-*-linux*)
    os_linux=yes
    ;;
esac

dnl =========================================================================
dnl Check OS target

AC_MSG_CHECKING([for some Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
if test "$platform_win32" = yes; then
  red_target=windows
else
  red_target=x11
fi

AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")
AM_CONDITIONAL(OS_UNIX, test "$os_win32" != "yes")
AM_CONDITIONAL(OS_LINUX, test "$os_linux" = "yes")

dnl =========================================================================
dnl Chek optional features
AC_ARG_ENABLE(tunnel,
[  --enable-tunnel         Enable network redirection],,
[enable_tunnel="no"])
AS_IF([test x"$enable_tunnel" != "xno"], [enable_tunnel="yes"])
AM_CONDITIONAL(SUPPORT_TUNNEL, test "x$enable_tunnel" != "xno")
if test "x$enable_tunnel" != "xno"; then
   AC_DEFINE(USE_TUNNEL, [1], [Define if supporting tunnel proxying])
fi

AC_ARG_ENABLE(gui,
[  --enable-gui            Enable start dialog with CEGUI],,
[enable_gui="no"])
AS_IF([test x"$enable_gui" != "xno"], [enable_gui="yes"])
AM_CONDITIONAL(SUPPORT_GUI, test "x$enable_gui" != "xno")

AC_ARG_ENABLE(opengl,
[  --enable-opengl         Enable opengl requirement / support (not recommended)],,
[enable_opengl="no"])
AS_IF([test x"$enable_opengl" != "xno"], [enable_opengl="yes"])
AM_CONDITIONAL(SUPPORT_GL, test "x$enable_opengl" = "xyes")

AC_ARG_ENABLE(smartcard,
[  --enable-smartcard      Enable network redirection],,
[enable_smartcard="no"])
AS_IF([test x"$enable_smartcard" != "xno"], [enable_smartcard="yes"])
AM_CONDITIONAL(SUPPORT_SMARTCARD, test "x$enable_smartcard" != "xno")
if test "x$enable_smartcard" = "xyes"; then
   AC_DEFINE(USE_SMARTCARD, [1], [Define if supporting smartcard proxying])
fi

AC_ARG_ENABLE(portredir,
[  --enable-portredir      Enable serial and parallel devices redirection],,
[enable_portredir="no"])
AS_IF([test x"$enable_portredir" != "xno"], [enable_portredir="yes"])
AM_CONDITIONAL(SUPPORT_PORTREDIR, test "x$enable_portredir" != "xno")
if test "x$enable_portredir" = "xyes"; then
   AC_DEFINE(USE_PORTREDIR, [1], [Define if supporting portredir redirection])
fi

AC_ARG_ENABLE(usbredir,
[  --enable-usbredir       Enable usb devices redirection],,
[enable_usbredir="no"])
AS_IF([test x"$enable_usbredir" != "xno"], [enable_usbredir="yes"])
AM_CONDITIONAL(SUPPORT_USBREDIR, test "x$enable_usbredir" != "xno")
if test "x$enable_usbredir" = "xyes"; then
   AC_DEFINE(USE_USBREDIR, [1], [Define if supporting usbredir redirection])
fi

AC_ARG_ENABLE(devredir,
[  --enable-devredir       Enable special devices redirection],,
[enable_devredir="no"])
AS_IF([test x"$enable_devredir" != "xno"], [enable_devredir="yes"])
AM_CONDITIONAL(SUPPORT_DEVREDIR, test "x$enable_devredir" != "xno")
if test "x$enable_devredir" = "xyes"; then
   AC_DEFINE(USE_DEVREDIR, [1], [Define if supporting devredir redirection])
fi

AC_ARG_ENABLE(gstreamer,
[  --enable-gstreamer      Enable gstreamer],,
[enable_gstreamer="no"])
AS_IF([test x"$enable_gstreamer" != "xno"], [enable_gstreamer="yes"])
AM_CONDITIONAL(SUPPORT_GSTREAMER, test "x$enable_gstreamer" != "xno")
if test "x$enable_gstreamer" = "xyes"; then
   AC_DEFINE(USE_GSTREAMER, [1], [Define if supporting gstreamer])
fi

AC_ARG_ENABLE(videoofflineredir,
[  --enable-videoofflineredir Enable offline video redir],,
[enable_videoofflineredir="no"])
AS_IF([test x"$enable_videoofflineredir" != "xno"], [$enable_videoofflineredir="yes"])
AM_CONDITIONAL(SUPPORT_OFFLINE_VIDEO_REDIR, test "x$enable_videoofflineredir" != "xno")
if test "x$enable_videoofflineredir" = "xyes"; then
   AC_DEFINE(OFFLINE_VIDEO_REDIR, [1], [Define if supporting samsung hardware h264 decoder])
fi

AC_ARG_ENABLE(videoonlineredir,
[  --enable-videoonlineredir  Enable online video redir],,
[enable_videoonlineredir="no"])
AS_IF([test x"$enable_videoonlineredir" != "xno"], [$enable_videonlineredir="yes"])
AM_CONDITIONAL(SUPPORT_ONLINE_VIDEO_REDIR, test "x$enable_videoonlineredir" != "xno")
if test "x$enable_videoonlineredir" = "xyes"; then
   AC_DEFINE(ONLINE_VIDEO_REDIR, [1], [Define if supporting video online redirect])
fi

AC_ARG_ENABLE(cloudclass,
[  --enable-cloudclass         Enable cloudclass],,
[enable_cloudclass="no"])
AS_IF([test x"$enable_cloudclass" != "xno"], [$enable_cloudclass="yes"])
AM_CONDITIONAL(SUPPORT_CLOUD_CLASS, test "x$enable_cloudclass" != "xno")
if test "x$enable_cloudclass" = "xyes"; then
   AC_DEFINE(CLOUD_CLASS, [1], [Define if supporting samsung hardware h264 decoder])
fi

AC_ARG_ENABLE(client,
[  --enable-client         Enable spice client],,
[enable_client="yes"])
AS_IF([test x"$enable_client" != "xno"], [enable_client="yes"])
AM_CONDITIONAL(SUPPORT_CLIENT, test "x$enable_client" = "xyes")

dnl =========================================================================
dnl Check deps

PKG_CHECK_MODULES(PROTOCOL, spice-protocol >= 0.9.1)
AC_SUBST(PROTOCOL_CFLAGS)

AC_CHECK_LIBM
AC_SUBST(LIBM)

AC_CHECK_LIB(rt, clock_gettime,
   AC_DEFINE(HAVE_CLOCK_GETTIME, 1, [Defined if we have clock_gettime()])
   LIBRT=-lrt
   )
AC_SUBST(LIBRT)

if test "$red_target" = "windows"; then
	SPICE_NONPKGCONFIG_LIBS+=" -lpthreadGC2 -lversion -lmsimg32 $LIBM"
else
	SPICE_NONPKGCONFIG_LIBS+=" -pthread $LIBM $LIBRT"
fi

dnl The client needs a yield function
AC_MSG_CHECKING(for posix yield function)
for yield_func in pthread_yield pthread_yield_np sched_yield \
					thr_yield; do
    spice_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $SPICE_NONPKGCONFIG_LIBS"
    AC_TRY_LINK([#include <pthread.h>],
		[$yield_func()],
		[posix_yield_func="$yield_func"
		break])
    CPPFLAGS="$spice_save_CPPFLAGS"
done
if test x"$posix_yield_func" = xnone; then
    AC_MSG_ERROR([No posix yield function found])
else
    AC_MSG_RESULT($posix_yield_func)
    posix_yield_func="$posix_yield_func()"
fi
AC_DEFINE_UNQUOTED(POSIX_YIELD_FUNC,$posix_yield_func,[The POSIX RT yield function])

SPICE_REQUIRES=""

if test "x$enable_gui" = "xyes"; then
    PKG_CHECK_MODULES(CEGUI06, CEGUI-0.6 >= 0.6.0 CEGUI-0.6 < 0.7.0,
    [
        AC_SUBST(CEGUI06_CFLAGS)
        AC_SUBST(CEGUI06_LIBS)
        AC_DEFINE([USE_GUI], [1], [Define to build with CEGUI support])
    ],
    [
        PKG_CHECK_MODULES(CEGUI, CEGUI >= 0.6.0 CEGUI < 0.7.0)
        AC_SUBST(CEGUI_CFLAGS)
        AC_SUBST(CEGUI_LIBS)
        AC_DEFINE([USE_GUI], [1], [Define to build with CEGUI support])
    ])
fi

if test "x$enable_tunnel" = "xyes"; then
	PKG_CHECK_MODULES(SLIRP, slirp)
	AC_SUBST(SLIRP_CFLAGS)
	AC_SUBST(SLIRP_LIBS)
	SPICE_REQUIRES+=" slirp"
	AC_DEFINE([HAVE_SLIRP], [], [Define if we have slirp])
fi

if test "x$enable_smartcard" = "xyes"; then
    PKG_CHECK_MODULES(CAC_CARD, libcacard >= 0.1.2)
    SMARTCARD_LIBS="$CAC_CARD_LIBS"
    SMARTCARD_CFLAGS="$CAC_CARD_CFLAGS"
    AC_SUBST(SMARTCARD_LIBS)
    AC_SUBST(SMARTCARD_CFLAGS)
fi

if test "x$enable_usbredir" = "xyes"; then
	PKG_CHECK_MODULES([USBREDIR],
                      [gudev-1.0 libusb-1.0 >= 1.0.9 libusbredirhost >= 0.4.2 libusbredirparser >= 0.4],
                      [have_usbredir=yes],
                      [have_usbredir=no])
	if test "x$have_usbredir" = "xno" && test "x$enable_usbredir" = "xyes"; then
		AC_MSG_ERROR([usbredir support explicitly requested, but some required packages are not available])
	fi
	if test "x$have_usbredir" = "xyes"; then
		AC_DEFINE(USE_USBREDIR, [1], [Define if supporting usbredir proxying])
	fi	
fi


PKG_CHECK_MODULES(PIXMAN, pixman-1 >= 0.17.7)
AC_SUBST(PIXMAN_CFLAGS)
AC_SUBST(PIXMAN_LIBS)
SPICE_REQUIRES+=" pixman-1 >= 0.17.7"

PKG_CHECK_MODULES(CELT051, celt051 >= 0.5.1.1)
AC_SUBST(CELT051_CFLAGS)
AC_SUBST(CELT051_LIBS)
AC_SUBST(CELT051_LIBDIR)
SPICE_REQUIRES+=" celt051 >= 0.5.1.1"

if test ! -e client/generated_marshallers.cpp; then
AC_MSG_CHECKING([for pyparsing python module])
echo "import pyparsing" | python - >/dev/null 2>&1
if test $? -ne 0 ; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([pyparsing python module is required to compile this package])
fi
AC_MSG_RESULT([found])
fi

if test "$os_linux" = yes; then
	PKG_CHECK_MODULES(ALSA, alsa)
	AC_SUBST(ALSA_CFLAGS)
	AC_SUBST(ALSA_LIBS)
fi

PKG_CHECK_MODULES(SSL, openssl)
AC_SUBST(SSL_CFLAGS)
AC_SUBST(SSL_LIBS)
SPICE_REQUIRES+=" openssl"

# These are commented out because the gl libraries on RHEL 5 do not have pkgconfig files
#
# PKG_CHECK_MODULES(GL, gl glu)
# AC_SUBST(GL_CFLAGS)
# AC_SUBST(GL_LIBS)
# SPICE_REQUIRES+=" gl glu"

if test "x$enable_opengl" = "xyes"; then
   AC_CHECK_LIB(GL, glBlendFunc, GL_LIBS="$GL_LIBS -lGL", enable_opengl=no)
   AC_CHECK_LIB(GLU, gluSphere, GL_LIBS="$GL_LIBS -lGLU", enable_opengl=no)
   AC_DEFINE([USE_OPENGL], [1], [Define to build with OpenGL support])
   AC_DEFINE([GL_GLEXT_PROTOTYPES], [], [Enable GLExt prototypes])

   if test "x$enable_opengl" = "xno"; then
      AC_MSG_ERROR([GL libraries not available])
   fi
fi

AC_SUBST(GL_CFLAGS)
AC_SUBST(GL_LIBS)
SPICE_NONPKGCONFIG_LIBS+=" $GL_LIBS"

if test "$red_target" = "x11"; then
	PKG_CHECK_MODULES(XRANDR, xrandr)
	PKG_CHECK_MODULES(XFIXES, xfixes)
	AC_SUBST(XRANDR_CFLAGS)
	AC_SUBST(XRANDR_LIBS)

	PKG_CHECK_MODULES(XRANDR12,
	        xrandr >= 1.2,
	        have_xrandr12=yes,
	        have_xrandr12=no)
else
	have_xrandr12=no
fi

AM_CONDITIONAL([HAVE_XRANDR12], [test "x$have_xrandr12" = "xyes"])
if test "x$have_xrandr12" = "xyes" ; then
  AC_DEFINE([HAVE_XRANDR12], [], [Define if we have XRANDR 12])
fi

if test "$red_target" = "x11"; then
	PKG_CHECK_MODULES(MISC_X, x11 xext xrender)
	AC_SUBST(MISC_X_CFLAGS)
	AC_SUBST(MISC_X_LIBS)
fi

# Add parameter for (partial) static linkage of spice client.
# this is used to achive single binary package for all (?) distros.
AC_ARG_ENABLE(static-linkage,
[  --enable-static-linkage will generate spice client binary with static linkage to external libraries ],,
[enable_static_linkage="no"])
AS_IF([test x"$enable_static_linkage" != "xno"],
[SPICEC_STATIC_LINKAGE_BSTATIC=["-Wl,-Bstatic"]])

AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
    AC_MSG_CHECKING([for jpeglib.h])
    AC_TRY_CPP(
[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <jpeglib.h>],
        JPEG_LIBS='-ljpeg'
        AC_MSG_RESULT($jpeg_ok),
	AC_MSG_ERROR([jpeglib.h not found])),
    AC_MSG_ERROR([libjpeg not found]))
AC_SUBST(JPEG_LIBS)

AC_CHECK_LIB(z, deflate, Z_LIBS='-lz', AC_MSG_ERROR([zlib not found]))
AC_SUBST(Z_LIBS)

dnl Cyrus SASL, check from gtk-vnc
AC_ARG_WITH([sasl],
  [AS_HELP_STRING([--with-sasl],
    [use cyrus SASL for authentication @<:@default=check@:>@])],
  [],
  [with_sasl=check])

SASL_CFLAGS=
SASL_LIBS=
enable_sasl=no
if test "x$with_sasl" != "xno"; then
  if test "x$with_sasl" != "xyes" -a "x$with_sasl" != "xcheck"; then
    SASL_CFLAGS="-I$with_sasl"
    SASL_LIBS="-L$with_sasl"
  fi
  fail=0
  old_cflags="$CFLAGS"
  old_libs="$LIBS"
  CFLAGS="$CFLAGS $SASL_CFLAGS"
  LIBS="$LIBS $SASL_LIBS"
  AC_CHECK_HEADER([sasl/sasl.h],[],[
    if test "x$with_sasl" != "xcheck" ; then
        with_sasl=no
    else
        fail=1
    fi])
  if test "x$with_sasl" != "xno" ; then
    AC_CHECK_LIB([sasl2], [sasl_client_init],[with_sasl2=yes],[with_sasl2=no])
  fi
  if test "x$with_sasl2" = "xno" -a "x$with_sasl" != "xno" ; then
    AC_CHECK_LIB([sasl], [sasl_client_init],[with_sasl=yes],[with_sasl=no])
  fi
  if test "x$with_sasl2" = "xyes"; then
    SASL_LIBS="$SASL_LIBS -lsasl2"
  elif test "x$with_sasl" = "xyes"; then
    SASL_LIBS="$SASL_LIBS -lsasl"
  else
    AC_MSG_ERROR([Missing required Cyrus SASL development package])
  fi
  CFLAGS="$old_cflags"
  LIBS="$old_libs"
  if test "x$with_sasl2" = "xyes" -o "x$with_sasl" = "xyes" ; then
    AC_DEFINE_UNQUOTED([HAVE_SASL], 1,
      [whether Cyrus SASL is available for authentication])
    enable_sasl=yes
  fi
fi
AM_CONDITIONAL([HAVE_SASL], [test "x$with_sasl2" = "xyes" -o "x$with_sasl" = "xyes"])
AC_SUBST([SASL_CFLAGS])
AC_SUBST([SASL_LIBS])

dnl ===========================================================================
dnl check compiler flags

AC_DEFUN([SPICE_CC_TRY_FLAG], [
  AC_MSG_CHECKING([whether $CC supports $1])

  spice_save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $1"

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([ ])], [spice_cc_flag=yes], [spice_cc_flag=no])
  CFLAGS="$spice_save_CFLAGS"

  if test "x$spice_cc_flag" = "xyes"; then
    ifelse([$2], , :, [$2])
  else
    ifelse([$3], , :, [$3])
  fi
  AC_MSG_RESULT([$spice_cc_flag])
])


dnl Use lots of warning flags with with gcc and compatible compilers

dnl Note: if you change the following variable, the cache is automatically
dnl skipped and all flags rechecked.  So there's no need to do anything
dnl else.  If for any reason you need to force a recheck, just change
dnl MAYBE_WARN in an ignorable way (like adding whitespace)

dnl MAYBE_WARN="-Wall -Wno-sign-compare -Werror -Wno-deprecated-declarations"

if test "$red_target" = "windows"; then
	MAYBE_WARN="-Wall -Wno-sign-compare -Wno-deprecated-declarations"
else
	MAYBE_WARN="-Wall -Wno-sign-compare -Wno-deprecated-declarations"
fi

AC_ARG_ENABLE(werror,
AC_HELP_STRING([--enable-werror], [Use -Werror (if supported)]),
set_werror="$enableval",[
if test -f $srcdir/GITVERSION; then
	is_git_version=true
	set_werror=yes
else
	set_werror=no
fi
])


# invalidate cached value if MAYBE_WARN has changed
if test "x$spice_cv_warn_maybe" != "x$MAYBE_WARN"; then
	unset spice_cv_warn_cflags
fi
AC_CACHE_CHECK([for supported warning flags], spice_cv_warn_cflags, [
	echo
	WARN_CFLAGS=""

	# Some warning options are not supported by all versions of
	# gcc, so test all desired options against the current
	# compiler.
	#
	# Note that there are some order dependencies
	# here. Specifically, an option that disables a warning will
	# have no net effect if a later option then enables that
	# warnings, (perhaps implicitly). So we put some grouped
	# options (-Wall and -Wextra) up front and the -Wno options
	# last.

	for W in $MAYBE_WARN; do
		SPICE_CC_TRY_FLAG([$W], [WARN_CFLAGS="$WARN_CFLAGS $W"])
	done
	if test "$set_werror" != "no"; then
		SPICE_CC_TRY_FLAG(["-Werror"], [WARN_CFLAGS="$WARN_CFLAGS -Werror"])
	fi

	spice_cv_warn_cflags=$WARN_CFLAGS
	spice_cv_warn_maybe=$MAYBE_WARN

	AC_MSG_CHECKING([which warning flags were supported])])
WARN_CFLAGS="$spice_cv_warn_cflags"
SPICE_CFLAGS="$SPICE_CFLAGS $WARN_CFLAGS"

# use ximage.h for win32 build if it is found (no package for mingw32 atm)
if test $os_win32 == "yes" ; then
    AC_MSG_CHECKING([for cximage])
    AC_TRY_CPP(
[#include <ximage.h>
int main()
{
 return CXIMAGE_FORMAT_RAW;
)}
],[
 AC_MSG_RESULT([found])
 have_cximage="yes"
]
,[AC_MSG_RESULT([missing])
 CXIMAGE_CFLAGS='-DDISABLE_CXIMAGE'
 have_cximage="no"
 ]
)
fi
AC_SUBST(CXIMAGE_CFLAGS)

# check for windres for mingw32 builds
if test $os_win32 == "yes" ; then
    AC_CHECK_PROGS(WINDRES,i686-pc-mingw32-windres mingw-windres mingw32-windres,false)
    AC_SUBST(WINDRES)
fi

AC_SUBST(WARN_CFLAGS)
AC_SUBST(CFLAGS_CFLAGS)

dnl =========================================================================
dnl -fvisibility stuff

have_gcc4=no
AC_MSG_CHECKING(for -fvisibility)
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#if defined(__GNUC__) && (__GNUC__ >= 4)
#else
error Need GCC 4.0 for visibility
#endif
int main () { return 0; }
])], have_gcc4=yes)

if test "x$have_gcc4" = "xyes" -a ! "$os_win32" = "yes" ; then
   VISIBILITY_HIDDEN_CFLAGS="-fvisibility=hidden"
fi
AC_MSG_RESULT($have_gcc4)
AC_SUBST(VISIBILITY_HIDDEN_CFLAGS)

AC_SUBST(SPICE_REQUIRES)
AC_SUBST(SPICE_NONPKGCONFIG_CFLAGS)
AC_SUBST(SPICE_NONPKGCONFIG_LIBS)

AC_SUBST([SPICEC_STATIC_LINKAGE_BSTATIC])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_OUTPUT([
Makefile
spice-server.pc
common/Makefile
common/win/Makefile
common/win/my_getopt-1.5/Makefile
python_modules/Makefile
server/Makefile
server/tests/Makefile
client/Makefile
])

dnl ==========================================================================
echo "

        Spice $VERSION
        ==============

        prefix:                   ${prefix}
        c compiler:               ${CC}
        c++ compiler:             ${CXX}

        Build Spice client:       ${enable_client}

        Have XRANDR 1.2:          ${have_xrandr12}

        Support tunneling:        ${enable_tunnel}

        Red target:               ${red_target}

        OpenGL:                   ${enable_opengl}

        GUI:                      ${enable_gui}

        VideoOfflineRedir:        ${enable_videoofflineredir}
       
        VideoOnlineRedir:         ${enable_videoonlineredir}

        CloudClass:               ${enable_cloudclass}

        Smartcard:                ${enable_smartcard}

        Portredir:                ${enable_portredir}

        Usbredir:                 ${enable_usbredir}

        Devredir:                 ${enable_devredir}

        gstreamer support:        ${enable_gstreamer}

        SASL support:             ${enable_sasl}
"

if test $os_win32 == "yes" ; then
echo \
"        Copy & Paste Images:     ${have_cximage}
"
fi

echo \
"        Now type 'make' to build $PACKAGE
"
