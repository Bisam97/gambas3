COMPONENT = gb.form.htmlview
include $(top_srcdir)/component.am

EXTRA_DIST=gumbo/LICENSE litehtml/README.md litehtml/LICENSE

noinst_LTLIBRARIES = libgumbo.la
gblib_LTLIBRARIES = gb.form.htmlview.la

libgumbo_la_LIBADD =
libgumbo_la_LDFLAGS = -module @LD_FLAGS@
libgumbo_la_CFLAGS = -std=c99 -I$(top_srcdir)/share $(AM_CFLAGS_OPT)

libgumbo_la_SOURCES = \
	gumbo/attribute.c \
	gumbo/attribute.h \
	gumbo/char_ref.c \
	gumbo/char_ref.h \
	gumbo/char_ref.rl \
	gumbo/error.c \
	gumbo/error.h \
	gumbo/gumbo.h \
	gumbo/insertion_mode.h \
	gumbo/parser.c \
	gumbo/parser.h \
	gumbo/string_buffer.c \
	gumbo/string_buffer.h \
	gumbo/string_piece.c \
	gumbo/string_piece.h \
	gumbo/tag_enum.h \
	gumbo/tag_gperf.h \
	gumbo/tag_sizes.h \
	gumbo/tag_strings.h \
	gumbo/tag.c \
	gumbo/token_type.h \
	gumbo/tokenizer_states.h \
	gumbo/tokenizer.c \
	gumbo/tokenizer.h \
	gumbo/utf8.c \
	gumbo/utf8.h \
	gumbo/util.c \
	gumbo/util.h \
	gumbo/vector.c \
	gumbo/vector.h

gb_form_htmlview_la_LIBADD = libgumbo.la
gb_form_htmlview_la_LDFLAGS = -module @LD_FLAGS@
gb_form_htmlview_la_CXXFLAGS = $(GB_CXXFLAGS_STD_CPP11) -I$(top_srcdir)/share -I$(srcdir)/gumbo $(AM_CXX_FLAGS)
gb_form_htmlview_la_CPPFLAGS =

gb_form_htmlview_la_SOURCES = \
	litehtml/background.h \
	litehtml/borders.h \
	litehtml/codepoint.h \
	litehtml/css_length.h \
	litehtml/css_margins.h \
	litehtml/css_offsets.h \
	litehtml/css_position.h \
	litehtml/css_properties.h \
	litehtml/css_selector.h \
	litehtml/document_container.h \
	litehtml/document.h \
	litehtml/el_anchor.h \
	litehtml/el_base.h \
	litehtml/el_before_after.h \
	litehtml/el_body.h \
	litehtml/el_break.h \
	litehtml/el_cdata.h \
	litehtml/el_comment.h \
	litehtml/el_div.h \
	litehtml/el_font.h \
	litehtml/el_image.h \
	litehtml/el_link.h \
	litehtml/el_para.h \
	litehtml/el_script.h \
	litehtml/el_space.h \
	litehtml/el_style.h \
	litehtml/el_table.h \
	litehtml/el_td.h \
	litehtml/el_text.h \
	litehtml/el_title.h \
	litehtml/el_tr.h \
	litehtml/element.h \
	litehtml/flex_item.h \
	litehtml/flex_line.h \
	litehtml/formatting_context.h \
	litehtml/html_tag.h \
	litehtml/html.h \
	litehtml/iterators.h \
	litehtml/line_box.h \
	litehtml/master_css.h \
	litehtml/media_query.h \
	litehtml/num_cvt.h \
	litehtml/os_types.h \
	litehmtl/render_block_context.h \
	litehtml/render_block.h \
	litehtml/render_flex.h \
	litehtml/render_image.h \
	litehtml/render_inline_context.h \
	litehtml/render_inline.h \
	litehmtl/render_item.h \
	litehtml/render_table.h \
	litehtml/string_id.h \
	litehtml/style.h \
	litehtml/stylesheet.h \
	litehtml/table.h \
	litehtml/tstring_view.h \
	litehtml/types.h \
	litehtml/url.h \
	litehtml/url_path.h \
	litehtml/utf8_strings.h \
	litehtml/web_color.h \
	litehtml/codepoint.cpp \
	litehtml/css_borders.cpp \
	litehtml/css_length.cpp \
	litehtml/css_properties.cpp \
	litehtml/css_selector.cpp \
	litehtml/document_container.cpp \
	litehtml/document.cpp \
	litehtml/el_anchor.cpp \
	litehtml/el_base.cpp \
	litehtml/el_before_after.cpp \
	litehtml/el_body.cpp \
	litehtml/el_break.cpp \
	litehtml/el_cdata.cpp \
	litehtml/el_comment.cpp \
	litehtml/el_div.cpp \
	litehtml/el_font.cpp \
	litehtml/el_image.cpp \
	litehtml/el_link.cpp \
	litehtml/el_para.cpp \
	litehtml/el_script.cpp \
	litehtml/el_space.cpp \
	litehtml/el_style.cpp \
	litehtml/el_table.cpp \
	litehtml/el_td.cpp \
	litehtml/el_text.cpp \
	litehtml/el_title.cpp \
	litehtml/el_tr.cpp \
	litehtml/element.cpp \
	litehtml/flex_item.cpp \
	litehtml/flex_line.cpp \
	litehtml/formatting_context.cpp \
	litehtml/html_tag.cpp \
	litehtml/html.cpp \
	litehtml/iterators.cpp \
	litehtml/line_box.cpp \
	litehtml/media_query.cpp \
	litehtml/num_cvt.cpp \
	litehtml/render_block_context.cpp \
	litehtml/render_block.cpp \
	litehtml/render_flex.cpp \
	litehtml/render_image.cpp \
	litehtml/render_inline_context.cpp \
	litehtml/render_item.cpp \
	litehtml/render_table.cpp \
	litehtml/string_id.cpp \
	litehtml/strtod.cpp \
	litehtml/style.cpp \
	litehtml/stylesheet.cpp \
	litehtml/table.cpp \
	litehtml/tstring_view.cpp \
	litehtml/url_path.cpp \
	litehtml/url.cpp \
	litehtml/utf8_strings.cpp \
	litehtml/web_color.cpp \
	\
	main.cpp main.h \
	c_htmldocument.cpp c_htmldocument.h
