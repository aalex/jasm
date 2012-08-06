# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT(jasm, [0.0.9], [alexandre@quessy.net], [jasm], [http://alexandre.quessy.net])
AC_CONFIG_MACRO_DIR([m4])
AC_MSG_NOTICE([Configuring jasm])
AM_INIT_AUTOMAKE([subdir-objects -Wno-portability])
AC_CONFIG_HEADERS([src/config.h])
AC_CANONICAL_HOST # fills $host variable

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for libraries
AC_CHECK_LIB([m],[sqrt])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h stddef.h stdlib.h string.h sys/time.h])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([gettimeofday memmove mkdir setenv sqrt strchr strpbrk uname])

AX_PTHREAD

CXXFLAGS+=" -Wall -Wextra -Wfatal-errors -Werror -Wnon-virtual-dtor "

#Set LIBLO with pkg-config
PKG_CHECK_MODULES(LIBLO, liblo, have_liblo=true, have_liblo=false)
if test "x${have_liblo}" = "xfalse" ; then
    AC_MSG_ERROR([Missing liblo])
fi

PKG_CHECK_MODULES(CLUTTERGST, clutter-gst-1.0 clutter-1.0, have_cluttergst=true, have_cluttergst=false)
if test "x${have_cluttergst}" = "xfalse" ; then
    AC_MSG_ERROR([Missing libclutter-gst-1.0 or libclutter-1.0])
fi

# Override the template file name of the generated .pc file, so that there
# is no need to rename the template file when the API version changes.
AC_CONFIG_FILES([
        Makefile 
        src/Makefile 
        ])

AC_OUTPUT

