## Process this file with automake to generate Makefile.in
GENERATE_TEXI = $(AM_V_GEN)$(SED) -n \
	-e '/^ \*\*\// { s/.*//; p }' \
	-e ' /^ \*\* / { s/^ \*\* //; p }' \
	-e '/^ \*\*/ { s/^ \*\*//; p }' $< > $@
GENERATED_TEXI = log.h.texi lua_api.h.texi
DISTCLEANFILES = $(GENERATED_TEXI)

info_TEXINFOS = mudcore.texi
mudcore_TEXINFOS = $(GENERATED_TEXI)

log.h.texi:     $(top_srcdir)/src/log.h;     $(GENERATE_TEXI)
lua_api.h.texi: $(top_srcdir)/src/lua_api.h; $(GENERATE_TEXI)
