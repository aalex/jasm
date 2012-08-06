bin_PROGRAMS = \
    jasm

AM_CFLAGS = \
    $(CLUTTERGST_CFLAGS) \
    $(LIBLO_CFLAGS) \
    -I$(top_srcdir)

LDADD = \
    $(CLUTTERGST_LIBS) \
    $(LIBLO_LIBS)

jasm_CFLAGS = $(AM_CFLAGS)
jasm_LDADD = $(LDADD)

jasm_SOURCES = \
    main.c

