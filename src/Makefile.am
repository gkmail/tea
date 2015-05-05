if ENABLE_MEMWATCH
MEMWATCH_SOURCES=memwatch.c
else
MEMWATCH_SOURCES=
endif

lib_LTLIBRARIES=libtea.la
libtea_la_SOURCES=\
	$(MEMWATCH_SOURCES)\
	t_auto.c\
	t_re.c

noinst_DATA=template.c
