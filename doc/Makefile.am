## Process this file with automake to generate Makefile.in
AM_MAKEINFOHTMLFLAGS = --no-headers --no-split
GENERATE_TEXI = $(AM_V_GEN)$(SED) -n \
	-e '/^ \*\*\// { s/.*//; p; }' \
	-e '/^ \*\* / { s/^ \*\* //; p; }' \
	-e '/^ \*\*/ { s/^ \*\*//; p; }' $< > $@
GENERATED_TEXI = \
	arg_parse.h.texi \
	buffer.h.texi \
	descriptor.h.texi \
	io.h.texi \
	log.h.texi \
	lua_api.h.texi \
	lua_args.h.texi \
	lua_descriptor.h.texi \
	lua_log.h.texi \
	lua_zmq.h.texi \
	options.h.texi \
	socket.h.texi \
	timeval.h.texi
DISTCLEANFILES = $(GENERATED_TEXI)

info_TEXINFOS = mudcore.texi
mudcore_TEXINFOS = \
	$(GENERATED_TEXI) \
	mudcore.args.texi \
	mudcore.descriptor.texi \
	mudcore.log.texi \
	mudcore.timer.texi \
	mudcore.zmq.texi

arg_parse.h.texi:      $(top_srcdir)/src/arg_parse.h;      $(GENERATE_TEXI)
buffer.h.texi:         $(top_srcdir)/src/buffer.h;         $(GENERATE_TEXI)
descriptor.h.texi:     $(top_srcdir)/src/descriptor.h;     $(GENERATE_TEXI)
io.h.texi:             $(top_srcdir)/src/io.h;             $(GENERATE_TEXI)
log.h.texi:            $(top_srcdir)/src/log.h;            $(GENERATE_TEXI)
lua_api.h.texi:        $(top_srcdir)/src/lua_api.h;        $(GENERATE_TEXI)
lua_args.h.texi:       $(top_srcdir)/src/lua_args.h;       $(GENERATE_TEXI)
lua_descriptor.h.texi: $(top_srcdir)/src/lua_descriptor.h; $(GENERATE_TEXI)
lua_log.h.texi:        $(top_srcdir)/src/lua_log.h;        $(GENERATE_TEXI)
lua_zmq.h.texi:        $(top_srcdir)/src/lua_zmq.h;        $(GENERATE_TEXI)
options.h.texi:        $(top_srcdir)/src/options.h;        $(GENERATE_TEXI)
socket.h.texi:         $(top_srcdir)/src/socket.h;         $(GENERATE_TEXI)
timeval.h.texi:        $(top_srcdir)/src/timeval.h;        $(GENERATE_TEXI)
