# a macro to get the libs/cflags for libfrontline
# Copyed from gdk-pixbuf.m4

dnl AM_PATH_FRONTLINE([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test to see if libfrontline is installed, and define FRONTLINE_CFLAGS, LIBS
dnl
AC_DEFUN(AM_PATH_FRONTLINE,
[dnl
dnl Get the cflags and libraries from the frontline-config script
dnl
AC_ARG_WITH(frontline-prefix,[  --with-frontline-prefix=PFX   Prefix where Frontline is installed (optional)],
            frontline_prefix="$withval", frontline_prefix="")
AC_ARG_WITH(frontline-exec-prefix,[  --with-frontline-exec-prefix=PFX Exec prefix where Frontline is installed (optional)],
            frontline_exec_prefix="$withval", frontline_exec_prefix="")
AC_ARG_ENABLE(frontlinetest, [  --disable-frontlinetest       Do not try to compile and run a test Frontline program],
		    , enable_frontlinetest=yes)

  if test x$frontline_exec_prefix != x ; then
     frontline_args="$frontline_args --exec_prefix=$frontline_exec_prefix"
     if test x${FRONTLINE_CONFIG+set} != xset ; then
        FRONTLINE_CONFIG=$frontline_exec_prefix/bin/frontline-config
     fi
  fi
  if test x$frontline_prefix != x ; then
     frontline_args="$frontline_args --prefix=$frontline_prefix"
     if test x${FRONTLINE_CONFIG+set} != xset ; then
        FRONTLINE_CONFIG=$frontline_prefix/bin/frontline-config
     fi
  fi

  AC_PATH_PROG(FRONTLINE_CONFIG, frontline-config, no)
  min_frontline_version=ifelse([$1], ,0.2.2,$1)
  AC_MSG_CHECKING(for FRONTLINE - version >= $min_frontline_version)
  no_frontline=""
  if test "$FRONTLINE_CONFIG" = "no" ; then
    no_frontline=yes
  else
    FRONTLINE_CFLAGS=`$FRONTLINE_CONFIG $frontline_args --cflags`
    FRONTLINE_LIBS=`$FRONTLINE_CONFIG $frontline_args --libs`

    frontline_major_version=`$FRONTLINE_CONFIG $frontline_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    frontline_minor_version=`$FRONTLINE_CONFIG $frontline_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    frontline_micro_version=`$FRONTLINE_CONFIG $frontline_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_frontlinetest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $FRONTLINE_CFLAGS"
      LIBS="$FRONTLINE_LIBS $LIBS"
dnl
dnl Now check if the installed FRONTLINE is sufficiently new. (Also sanity
dnl checks the results of frontline-config to some extent
dnl
      rm -f conf.frontlinetest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <frontline/frontline.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.frontlinetest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_frontline_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_frontline_version");
     exit(1);
   }

   if (($frontline_major_version > major) ||
      (($frontline_major_version == major) && ($frontline_minor_version > minor)) ||
      (($frontline_major_version == major) && ($frontline_minor_version == minor) && ($frontline_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'frontline-config --version' returned %d.%d.%d, but the minimum version\n", $frontline_major_version, $frontline_minor_version, $frontline_micro_version);
      printf("*** of FRONTLINE required is %d.%d.%d. If frontline-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If frontline-config was wrong, set the environment variable FRONTLINE_CONFIG\n");
      printf("*** to point to the correct copy of frontline-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}
],, no_frontline=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_frontline" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$FRONTLINE_CONFIG" = "no" ; then
       echo "*** The frontline-config script installed by FRONTLINE could not be found"
       echo "*** If FRONTLINE was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the FRONTLINE_CONFIG environment variable to the"
       echo "*** full path to frontline-config."
     else
       if test -f conf.frontlinetest ; then
        :
       else
          echo "*** Could not run FRONTLINE test program, checking why..."
          CFLAGS="$CFLAGS $FRONTLINE_CFLAGS"
          LIBS="$LIBS $FRONTLINE_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <frontline/frontline.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding FRONTLINE or finding the wrong"
          echo "*** version of FRONTLINE. If it is not finding FRONTLINE, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means FRONTLINE was incorrectly installed"
          echo "*** or that you have moved FRONTLINE since it was installed. In the latter case, you"
          echo "*** may want to edit the frontline-config script: $FRONTLINE_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     FRONTLINE_CFLAGS=""
     FRONTLINE_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(FRONTLINE_CFLAGS)
  AC_SUBST(FRONTLINE_LIBS)
  rm -f conf.frontlinetest
])