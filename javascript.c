/*	$Id$ */
/*
 * Copyright (c) 2017 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "config.h"

#include <sys/queue.h>

#if HAVE_ERR
# include <err.h>
#endif
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extern.h"

static	const char *types[FTYPE__MAX] = {
	"number", /* FTYPE_BIT */
	"number", /* FTYPE_DATE */
	"number", /* FTYPE_EPOCH */
	"number", /* FTYPE_INT */
	"double", /* FTYPE_REAL */
	NULL, /* FTYPE_BLOB */
	"string", /* FTYPE_TEXT */
	"string", /* FTYPE_PASSWORD */
	"string", /* FTYPE_EMAIL */
	NULL, /* FTYPE_STRUCT */
	"number", /* FTYPE_ENUM */
	"number", /* FTYPE_BITFIELD */
};

static void
gen_jsdoc_field(const struct field *f)
{

	if (FIELD_NOEXPORT & f->flags || FTYPE_BLOB == f->type)
		return;

	if (FIELD_NULL & f->flags) {
		print_commentv(2, COMMENT_JS_FRAG,
			"<li>%s-has-%s: \"hide\" class "
			"removed if %s not null, otherwise "
			"\"hide\" class is added</li>",
			f->parent->name, f->name, f->name);
		print_commentv(2, COMMENT_JS_FRAG,
			"<li>%s-no-%s: \"hide\" class "
			"added if %s not null, otherwise "
			"\"hide\" class is removed</li>",
			f->parent->name, f->name, f->name);
	} 

	if (FTYPE_STRUCT == f->type) {
		print_commentv(2, COMMENT_JS_FRAG,
			"<li>%s-%s-obj: invoke [fillInner]{@link "
			"%s#fillInner} with %s data%s</li>",
			f->parent->name, f->name, 
			f->ref->tstrct, f->name,
			FIELD_NULL & f->flags ? 
			" (if non-null)" : "");
	} else {
		print_commentv(2, COMMENT_JS_FRAG,
			"<li>%s-%s-text: replace contents "
			"with %s data%s</li>",
			f->parent->name, f->name, f->name,
			FIELD_NULL & f->flags ? 
			" (if non-null)" : "");
		print_commentv(2, COMMENT_JS_FRAG,
			"<li>%s-%s-value: replace \"value\" "
			"attribute with %s data%s</li>",
			f->parent->name, f->name, f->name,
			FIELD_NULL & f->flags ? 
			" (if non-null)" : "");
	}
}

static void
gen_js_field(const struct field *f)
{
	char	*buf = NULL;
	int	 rc;

	if (FIELD_NOEXPORT & f->flags)
		return;
	if (FTYPE_STRUCT == f->type) {
		rc = asprintf(&buf, "new %s(o.%s)", 
			f->ref->tstrct, f->name);
		if (rc < 0)
			err(EXIT_FAILURE, NULL);
	}

	printf("\t\t\t_fillfield(e, '%s', '%s', custom, o.%s, "
		"inc, %s, %s, %s);\n",
		f->parent->name, f->name, f->name,
		FIELD_NULL & f->flags ? "true" : "false",
		FTYPE_BLOB == f->type ? "true" : "false",
		NULL == buf ? "null" : buf);
	free(buf);
}

void
gen_javascript(const struct config *cfg, int tsc)
{
	const struct strct  *s;
	const struct field  *f;
	const struct bitf   *bf;
	const struct bitidx *bi;
	const struct enm    *e;
	const struct eitem  *ei;
	const char	    *ns = "kwebapp";

	/*
	 * Begin with the methods we'll use throughout the js file.
	 * This includes _attr, which sets an attribute of an element; 
	 * _attrcl, which sets the attribute of all elements of a given
	 * className under a root; _repl, which is like _attr except
	 * in setting the element's child as a text node (after clearing
	 * it); _replcl, which is like _attrcl but for _repl; _hide,
	 * which adds the "hide" class to an element; and _show, which
	 * removes the "hide" class.
	 * These use _elemList, which is like getElementsByClassName
	 * except that it also considers the root and returns an array,
	 * not an HTMLCollection.
	 */

	if (tsc)
		printf("namespace %s {\n", ns);
	else
		printf("var %s = (function(root) {\n"
		       "\t'use strict';\n"
		       "\n", ns);

	if (tsc) 
		puts("\tfunction _attr(e: HTMLElement|null, "
			"attr: string, text: string): void");
	else
		puts("\tfunction _attr(e, attr, text)");

	puts("\t{\n"
	     "\t\tif (null !== e)\n"
	     "\t\t\te.setAttribute(attr, text);\n"
	     "\t}\n"
	     "");

	if (tsc)
		puts("\tfunction _attrcl(e: HTMLElement|null, "
			"attr: string, name: string, "
			"text: string, inc: boolean): void\n"
	  	     "\t{\n"
		     "\t\tlet list: HTMLElement[];\n"
		     "\t\tlet i: number;\n"
		     "");
	else
		puts("\tfunction _attrcl(e, attr, name, text, inc)\n"
		     "\t{\n"
		     "\t\tvar list, i;\n"
		     "");

	puts("\t\tif (null === e)\n"
	     "\t\t\treturn;\n"
	     "\t\tlist = _elemList(e, name, inc);\n"
	     "\t\tfor (i = 0; i < list.length; i++)\n"
	     "\t\t\t_attr(list[i], attr, text);\n"
	     "\t}\n"
	     "");
	
	if (tsc)
		puts("\tfunction _elemList(e: HTMLElement, "
			"cls: string, inc: boolean): "
			"HTMLElement[]\n"
		     "\t{\n"
		     "\t\tlet a: HTMLElement[] = [];\n"
		     "\t\tlet list: NodeListOf<Element>;\n"
		     "\t\tlet i: number;\n"
		     "");
	else
		puts("\tfunction _elemList(e, cls, inc)\n"
		     "\t{\n"
	             "\t\tvar a = [], list, i;\n"
		     "");

	printf("\t\tif (null === e)\n"
	       "\t\t\treturn(a);\n"
	       "\t\tlist = e.getElementsByClassName(cls);\n"
	       "\t\tfor (i = 0; i < list.length; i++)\n"
	       "\t\t\ta.push(%slist[i]);\n"
	       "\t\tif (inc && e.classList.contains(cls))\n"
	       "\t\t\ta.push(e);\n"
	       "\t\treturn(a);\n"
	       "\t}\n"
	       "\n", tsc ? "<HTMLElement>" : "");

	if (tsc)
		puts("\tfunction _repl(e: HTMLElement, "
			"text: string): void");
	else
		puts("\tfunction _repl(e, text)");

	puts("\t{\n"
	     "\t\tif (null === e)\n"
	     "\t\t\treturn;\n"
	     "\t\twhile (e.firstChild)\n"
	     "\t\t\te.removeChild(e.firstChild);\n"
	     "\t\te.appendChild(document.createTextNode(text));\n"
	     "\t}\n"
	     "");

	if (tsc)
		puts("\tfunction _fillfield(e: HTMLElement, "
			"strct: string, name: string,\n"
		     "\t\tfuncs, obj, inc: boolean, cannull: boolean,\n"
		     "\t\tisblob: boolean, sub): void\n"
		     "\t{\n"
		     "\t\tlet i: number;\n"
		     "\t\tlet fname: string;\n"
		     "");
	else
		puts("\tfunction _fillfield(e, strct, name, funcs, "
			"obj, inc, cannull, isblob, sub)\n"
		     "\t{\n"
	      	     "\t\tvar i, fname = strct + '-' + name;\n"
		     "");

	puts("\t\t/* First handle the custom callback. */\n"
	     "\t\tif (typeof funcs !== 'undefined' && \n"
	     "\t\t    null !== funcs && fname in funcs) {\n"
	     "\t\t\tif (funcs[fname] instanceof Array) {\n"
	     "\t\t\t\tfor (i = 0; i < funcs[fname].length; i++)\n"
	     "\t\t\t\t\tfuncs[fname][i](e, fname, obj);\n"
	     "\t\t\t} else {\n"
	     "\t\t\t\tfuncs[fname](e, fname, obj);\n"
	     "\t\t\t}\n"
	     "\t\t}\n"
	     "\t\t/* Now handle our has/no null situation. */\n"
	     "\t\tif (cannull) {\n"
	     "\t\t\tif (null === obj) {\n"
	     "\t\t\t\t_hidecl(e, strct + '-has-' + name, inc);\n"
	     "\t\t\t\t_showcl(e, strct + '-no-' + name, inc);\n"
	     "\t\t\t} else {\n"
	     "\t\t\t\t_showcl(e, strct + '-has-' + name, inc);\n"
	     "\t\t\t\t_hidecl(e, strct + '-no-' + name, inc);\n"
	     "\t\t\t}\n"
	     "\t\t}\n"
	     "\t\t/* Don't account for blobs any more. */\n"
	     "\t\tif (isblob)\n"
	     "\t\t\treturn;\n"
	     "\t\t/* Don't process null values that can be null. */\n"
	     "\t\tif (cannull && null === obj)\n"
	     "\t\t\treturn;\n"
	     "\t\t/* Non-null non-structs. */\n"
	     "\t\tif (null !== sub) {\n"
	     "\t\t\tvar list = _elemList(e, fname + '-obj', inc);\n"
	     "\t\t\tfor (i = 0; i < list.length; i++) {\n"
	     "\t\t\t\tsub.fillInner(list[i], funcs);\n"
	     "\t\t\t}\n"
	     "\t\t} else {\n"
	     "\t\t\t_replcl(e, fname + '-text', obj, inc);\n"
	     "\t\t\t_attrcl(e, 'value', fname + '-value', obj, inc);\n"
	     "\t\t}\n"
	     "\t}\n"
	     "");

	if (tsc)
		puts("\tfunction _replcl(e: HTMLElement, name: string, "
			"text: string, inc: boolean): void\n"
		     "\t{\n"
		     "\t\tlet list: HTMLElement[];\n"
		     "\t\tlet i: number;\n"
		     "");
	else
		puts("\tfunction _replcl(e, name, text, inc)\n"
		     "\t{\n"
	     	     "\t\tvar list, i;\n"
		     "");

	puts("\t\tif (null === e)\n"
	     "\t\t\treturn;\n"
	     "\t\tlist = _elemList(e, name, inc);\n"
	     "\t\tfor (i = 0; i < list.length; i++)\n"
	     "\t\t\t_repl(list[i], text);\n"
	     "\t}\n"
	     "");

	if (tsc)
		puts("\tfunction _classadd(e: HTMLElement|null, "
			"name: string): HTMLElement|null\n");
	else
		puts("\tfunction _classadd(e, name)");

	puts("\t{\n"
	     "\t\tif (null === e)\n"
	     "\t\t\treturn(null);\n"
	     "\t\tif ( ! e.classList.contains(name))\n"
	     "\t\t\te.classList.add(name);\n"
	     "\t\treturn(e);\n"
	     "\t}\n"
	     "");

	if (tsc)
		puts("\tfunction _classaddcl(e: HTMLElement, "
			"name: string, cls: string, inc: boolean): "
			"void\n"
		     "\t{\n"
		     "\t\tlet list: HTMLElement[];\n"
		     "\t\tlet i: number;\n"
		     "");
	else
		puts("\tfunction _classaddcl(e, name, cls, inc)\n"
		     "\t{\n"
	     	     "\t\tvar list, i;\n"
		     "");

	puts("\t\tif (null === e)\n"
	     "\t\t\treturn;\n"
	     "\t\tlist = _elemList(e, name, inc);\n"
	     "\t\tfor (i = 0; i < list.length; i++)\n"
	     "\t\t\t_classadd(list[i], cls);\n"
	     "\t}\n"
	     "");

	if (tsc)
		puts("\tfunction _hide(e: HTMLElement): "
			"HTMLElement|null");
	else
		puts("\tfunction _hide(e)");

	puts("\t{\n"
	     "\t\tif (null === e)\n"
	     "\t\t\treturn(null);\n"
	     "\t\tif ( ! e.classList.contains('hide'))\n"
	     "\t\t\te.classList.add('hide');\n"
	     "\t\treturn(e);\n"
	     "\t}\n"
	     "");

	if (tsc)
		puts("\tfunction _hidecl(e: HTMLElement, "
			"name: string, inc: boolean): void\n"
		     "\t{\n"
		     "\t\tlet list: HTMLElement[];\n"
		     "\t\tlet i: number;\n"
		     "");
	else
		puts("\tfunction _hidecl(e, name, inc)\n"
		     "\t{\n"
	     	     "\t\tvar list, i;\n"
		     "");

	puts("\t\tif (null === e)\n"
	     "\t\t\treturn;\n"
	     "\t\tlist = _elemList(e, name, inc);\n"
	     "\t\tfor (i = 0; i < list.length; i++)\n"
	     "\t\t\t_hide(list[i]);\n"
	     "\t}\n"
	     "");

	if (tsc)
		puts("\tfunction _show(e: HTMLElement): "
			"HTMLElement|null");
	else
		puts("\tfunction _show(e)");

	puts("\t{\n"
	     "\t\tif (null === e)\n"
	     "\t\t\treturn(null);\n"
	     "\t\tif (e.classList.contains('hide'))\n"
	     "\t\t\te.classList.remove('hide');\n"
	     "\t\treturn(e);\n"
	     "\t}\n"
	     "");

	if (tsc)
		puts("\tfunction _showcl(e: HTMLElement, "
			"name: string, inc: boolean): void\n"
		     "\t{\n"
		     "\t\tlet list: HTMLElement[];\n"
		     "\t\tlet i: number;\n"
		     "");
	else
		puts("\tfunction _showcl(e, name, inc)\n"
		     "\t{\n"
	     	     "\t\tvar list, i;\n"
		     "");

	puts("\t\tif (null === e)\n"
	     "\t\t\treturn;\n"
	     "\t\tlist = _elemList(e, name, inc);\n"
	     "\t\tfor (i = 0; i < list.length; i++)\n"
	     "\t\t\t_show(list[i]);\n"
	     "\t}\n"
	     "");

	if (tsc)
		TAILQ_FOREACH(s, &cfg->sq, entries) {
			printf("\tinterface %sData\n"
			       "\t{\n", s->name);
			TAILQ_FOREACH(f, &s->fq, entries)
				if (FTYPE_STRUCT == f->type)
					printf("\t\t%s: %sData;\n",
						f->name, 
						f->ref->tstrct);
				else if (NULL != types[f->type])
					printf("\t\t%s: %s;\n",
						f->name,
						types[f->type]);
			puts("\t}\n");
		}

	/*
	 * This is pretty straightforward.
	 * Each structure is an object initialised by either an object
	 * from the server or an array of objects.
	 * Each object has the "fill" and "fillArray" methods.
	 * These use the internal _fill method, which accepts both the
	 * object (or array) and the element to be filled.
	 */

	TAILQ_FOREACH(s, &cfg->sq, entries) {
		print_commentv(1, COMMENT_JS,
			"%s%s%s\n"
			"This constructor accepts the \"%s\" objects "
			"or array of objects serialises into a "
			"DOM tree.\n"
			"@param {(Object|Object[])} obj - The %s "
			"object or array of objects.\n"
			"@class %s",
			NULL == s->doc ? "" : "\n",
			NULL == s->doc ? "" : s->doc,
			NULL == s->doc ? "" : "<br />\n",
			s->name, s->name, s->name);
		if (tsc)
			printf("\tclass %s {\n"
			       "\t\tobj: %sData|%sData[];\n"
			       "\t\tconstructor(o: %sData|%sData[]) {\n"
			       "\t\t\tthis.obj = o;\n"
			       "\t\t}\n"
			       "\n", s->name, s->name, 
			       s->name, s->name, s->name);
		else
			printf("\tfunction %s(obj)\n"
			       "\t{\n"
			       "\t\tthis.obj = obj;\n"
			       "\n",
			       s->name);

		print_commentv(2, COMMENT_JS_FRAG_OPEN,
			"Fill in a \"%s\" object at the given "
			"element in the DOM tree.\n"
			"If the object was initialised with an "
			"array, the first element is used.\n"
			"Elements within (and including) \"e\" having "
			"the following classes are manipulated as "
			"follows:", s->name);
		print_commentt(2, COMMENT_JS_FRAG, "<ul>");
		TAILQ_FOREACH(f, &s->fq, entries)
			gen_jsdoc_field(f);
		print_commentt(2, COMMENT_JS_FRAG, "</ul>");
		print_commentv(2, COMMENT_JS_FRAG_CLOSE,
			"@param {Object} e - The DOM element.\n"
			"@param {Object} custom - A dictionary "
			"of functions keyed by structure and field "
			"name (e.g., \"foo\" structure, \"bar\" "
			"field would be \"foo-bar\"). "
			"The value is a function for custom "
			"handling that accepts the \"e\" value, "
			"the name of the structure-field, and the "
			"value of the structure and field.\n"
			"You may also specify an array of functions "
			"instead of a singleton.\n"
			"@memberof %s#\n"
			"@method fill",
			s->name);

		if (tsc)
			puts("\t\tfill(e: HTMLElement, custom): void\n"
			     "\t\t{");
		else
			puts("\t\tthis.fill = function(e, custom)\n"
			     "\t\t{");

		printf("\t\t\tthis._fill(e, this.obj, true, custom);\n"
		       "\t\t}%s\n"
		       "\n", tsc ? "" : ";");

		print_commentv(2, COMMENT_JS,
			"Like [fill]{@link %s#fill} but not including "
			"the root element \"e\".\n"
			"@param {Object} e - The DOM element.\n"
			"@param {Object} custom - The custom "
			"handler dictionary (see [fill]{@link "
			"%s#fill} for details).\n"
			"@memberof %s#\n"
			"@method fillInner",
			s->name, s->name, s->name);

		if (tsc)
			puts("\t\tfillInner(e: HTMLElement, custom): "
				"void\n"
			     "\t\t{");
		else
			puts("\t\tthis.fillInner = "
				"function(e, custom)\n"
			     "\t\t{");

		printf("\t\t\tthis._fill(e, this.obj, false, custom);\n"
		       "\t\t}%s\n"
		       "\n", tsc ? "" : ";");

		print_commentv(2, COMMENT_JS,
			"Implements all [fill]{@link %s#fill} style "
			"functions.\n"
			"@private\n"
			"@method _fill\n"
			"@memberof %s#\n"
			"@param {Object} e - The DOM element.\n"
			"@param {(Object|Object[])} o - The object "
			"(or array) to fill.\n"
			"@param {Number} inc - Whether to include "
			"the root or not when processing.\n"
			"@param {Object} custom - The custom "
			"handler dictionary (see [fill]{@link "
			"%s#fill}).",
			s->name, s->name, s->name);

		if (tsc)
			puts("\t\tprivate _fill(e: HTMLElement, "
				"o, inc: boolean, custom): void\n"
			     "\t\t{\n"
			     "\t\t\tlet i: number;\n"
			     "");
		else
			puts("\t\tthis._fill = function"
				"(e, o, inc, custom)\n"
			     "\t\t{\n"
			     "\t\t\tvar i;\n"
			     "");

		puts("\t\t\tif (null === o || null === e)\n"
		     "\t\t\t\treturn;\n"
		     "\t\t\tif (o instanceof Array) {\n"
		     "\t\t\t\tif (0 === o.length)\n"
		     "\t\t\t\t\treturn;\n"
		     "\t\t\t\to = o[0];\n"
		     "\t\t\t}");
		
		/* Custom callback on the object itself. */

		printf("\t\t\tif (typeof custom !== 'undefined' && \n"
		       "\t\t\t    null !== custom && '%s' in custom) {\n"
		       "\t\t\t\tif (custom['%s'] instanceof Array) {\n"
		       "\t\t\t\t\tfor (i = 0; "
				      "i < custom['%s'].length; i++)\n"
		       "\t\t\t\t\t\tcustom['%s'][i](e, \"%s\", o);\n"
		       "\t\t\t\t} else {\n"
		       "\t\t\t\t\tcustom['%s'](e, \"%s\", o);\n"
		       "\t\t\t\t}\n"
		       "\t\t\t}\n",
		       s->name, s->name, s->name, s->name, 
		       s->name, s->name, s->name);
		TAILQ_FOREACH(f, &s->fq, entries)
			gen_js_field(f);

		printf("\t\t}%s\n"
		       "\n", tsc ? "" : ";");

		print_commentv(2, COMMENT_JS,
			"Like [fill]{@link %s#fill} but for an "
			"array of %s.\n"
			"This will remove the first element within "
			"\"e\" then repeatedly clone and re-append it, "
			"filling in the cloned subtree with the "
			"array.\n"
			"If \"e\" is not an array, it is construed "
			"as an array of one.\n"
			"If the input array is empty, \"e\" is hidden "
			"by using the \"hide\" class.\n"
			"Otherwise, the \"hide\" class is removed.\n"
			"@param {Object} e - The DOM element.\n"
			"@param {Object} custom - The custom "
			"handler dictionary (see [fill]{@link "
			"%s#fill}).\n"
			"@memberof %s#\n"
			"@method fillArray",
			s->name, s->name, s->name, s->name);

		if (tsc)
			printf("\t\tfillArray(e: HTMLElement|null, "
				"custom): void\n"
			       "\t\t{\n"
			       "\t\t\tlet o: %sData|%sData[];\n"
			       "\t\t\tlet j: number;\n"
			       "\t\t\tlet cln, row: HTMLElement;\n"
			       "\n", s->name, s->name);
		else
			puts("\t\tthis.fillArray = function"
				"(e, custom)\n"
			     "\t\t{\n"
			     "\t\t\tvar o, j, row, cln;\n"
			     "");

		puts("\t\t\to = this.obj;");

		TAILQ_FOREACH(f, &s->fq, entries)
			if ( ! (FIELD_NOEXPORT & f->flags) &&
			    FTYPE_STRUCT == f->type) {
				puts("\t\t\tvar list, strct, i;");
				break;
			}

		printf("\t\t\tif (null === o || null === e)\n"
		       "\t\t\t\treturn;\n"
		       "\t\t\tif ( ! (o instanceof Array)) {\n"
		       "\t\t\t\tvar ar = [];\n"
		       "\t\t\t\tar.push(o);\n"
		       "\t\t\t\to = ar;\n"
		       "\t\t\t}\n"
		       "\t\t\tif (0 === o.length) {\n"
		       "\t\t\t\t_hide(e);\n"
		       "\t\t\t\treturn;\n"
		       "\t\t\t}\n"
		       "\t\t\t_show(e);\n"
		       "\t\t\trow = %se.children[0];\n"
		       "\t\t\tif (null === row)\n"
		       "\t\t\t\treturn;\n"
		       "\t\t\te.removeChild(row);\n"
		       "\t\t\twhile (null !== e.firstChild)\n"
		       "\t\t\t\te.removeChild(e.firstChild)\n"
		       "\t\t\tfor (j = 0; j < o.length; j++) {\n"
		       "\t\t\t\tcln = %srow.cloneNode(true);\n"
		       "\t\t\t\te.appendChild(cln);\n"
		       "\t\t\t\tthis._fill(cln, o[j], true, custom);\n"
		       "\t\t\t}\n"
		       "\t\t}%s\n", 
			tsc ? "<HTMLElement>" : "",
			tsc ? "<HTMLElement>" : "",
			tsc ? "" : ";");
		puts("\t}\n"
		     "");
	}

	TAILQ_FOREACH(bf, &cfg->bq, entries) {
		print_commentt(1, COMMENT_JS_FRAG_OPEN, bf->doc);
		print_commentv(1, COMMENT_JS_FRAG,
			"This defines the bit indices for the %s "
			"bit-field.\n"
			"The BITI fields are the bit indices "
			"(0--63) and the BITF fields are the "
			"masked integer values.\n"
			"@namespace\n"
			"@readonly\n"
			"@typedef %s", bf->name, bf->name);
		TAILQ_FOREACH(bi, &bf->bq, entries) 
			print_commentv(1, COMMENT_JS_FRAG,
				"@property {number} BITI_%s %s\n"
				"@property {number} BITF_%s %s",
				bi->name,
				NULL != bi->doc ? bi->doc : "",
				bi->name,
				NULL != bi->doc ? bi->doc : "");
		print_commentv(1, COMMENT_JS_FRAG_CLOSE,
			"@property {} format Uses a bit field's "
			"<code>jslabel</code> (or just the "
			"name, if no <code>jslabel</code> is defined) "
			"to format a custom label as invoked on an "
			"object's <code>fill</code> function. "
			"This will act on <code>xxx-yyy-label</code> "
			"classes, where <code>xxx</code> is the "
			"structure name and <code>yyy</code> is the "
			"field name. "
			"Multiple entries are comma-separated.\n"
			"For example, <code>xxx.fill(e, { 'xxx-yyy': "
			"%s.format });</code>, where <code>yyy</code> "
			"is a field of type <code>enum %s</code>.",
			bf->name, bf->name);
		printf("\tvar %s = {\n", bf->name);
		TAILQ_FOREACH(bi, &bf->bq, entries) {
			if (NULL != bi->doc)
				print_commentt(2, COMMENT_JS, bi->doc);
			printf("\t\tBITI_%s: %" PRId64 ",\n",
				bi->name, bi->value);
			printf("\t\tBITF_%s: %u,\n",
				bi->name, 1U << bi->value);
		}
		printf("\t\tformat: function(e, name, val) {\n"
		       "\t\t\tvar v, i = 0, str = '';\n"
		       "\t\t\tname += '-label';\n"
		       "\t\t\tif (null === val) {\n"
		       "\t\t\t\t_replcl(e, name, \'not given\', false);\n"
		       "\t\t\t\t_classaddcl(e, name, \'noanswer\', false);\n"
		       "\t\t\t\treturn;\n"
		       "\t\t\t}\n"
		       "\t\t\tv = parseInt(val);\n"
		       "\t\t\tif (0 === v) {\n"
		       "\t\t\t\t_replcl(e, name, \'%s\', false);\n"
		       "\t\t\t\treturn;\n"
		       "\t\t\t}\n",
		       NULL == bf->jslabel ? "none" : bf->jslabel);
		TAILQ_FOREACH(bi, &bf->bq, entries)
			printf("\t\t\tif (%s.BITF_%s & v)\n"
		       	       "\t\t\t\tstr += "
			        "(i++ > 0 ? ', ' : '') + '%s';\n",
			       bf->name, bi->name,
			       NULL != bi->jslabel ? bi->jslabel :
			       bi->name);
		printf("\t\t\tif (0 === str.length) {\n"
		       "\t\t\t\t_replcl(e, name, \'unknown\', false);\n"
		       "\t\t\t\treturn;\n"
		       "\t\t\t}\n"
		       "\t\t\t_replcl(e, name, str, false);\n"
		       "\t\t}\n"
		       "\t};\n"
		       "\n");
	}

	TAILQ_FOREACH(e, &cfg->eq, entries) {
		print_commentt(1, COMMENT_JS_FRAG_OPEN, e->doc);
		print_commentv(1, COMMENT_JS_FRAG,
			"This object consists of all values for "
			"the %s enumeration.\n"
			"It also contains a <code>format</code> "
			"function designed to work as a custom "
			"callback for <code>fill</code>-style "
			"functions for objects.\n"
			"@namespace\n"
			"@readonly\n"
			"@typedef %s", e->name, e->name);
		TAILQ_FOREACH(ei, &e->eq, entries) 
			print_commentv(1, COMMENT_JS_FRAG,
				"@property {number} %s %s", ei->name,
				NULL != ei->doc ? ei->doc : "");
		print_commentv(1, COMMENT_JS_FRAG,
			"@property {} format Uses the enumeration "
			"item's <code>jslabel</code> (or just the "
			"name, if no <code>jslabel</code> is defined) "
			"to format a custom label as invoked on an "
			"object's <code>fill</code> function. "
			"This will act on <code>xxx-yyy-label</code> "
			"classes, where <code>xxx</code> is the "
			"structure name and <code>yyy</code> is the "
			"field name. "
			"For example, <code>xxx.fill(e, { 'xxx-yyy': "
			"%s.format });</code>, where <code>yyy</code> "
			"is a field of type <code>enum %s</code>.",
			e->name, e->name);

		print_commentt(1, COMMENT_JS_FRAG_CLOSE, NULL);

		if (tsc) {
			printf("\tclass %s\n"
			       "\t{\n", e->name);
			TAILQ_FOREACH(ei, &e->eq, entries)
				printf("\t\tstatic readonly %s: number = %" 
					PRId64 ";\n", ei->name, 
					ei->value);
			puts("\t\tstatic format(e: HTMLElement, "
					"name: string, "
					"val: string|null): void\n"
			     "\t\t{");
		} else {
			printf("\tvar %s =\n"
			       "\t{\n", e->name);
			TAILQ_FOREACH(ei, &e->eq, entries)
				printf("\t\t%s: %" PRId64 ",\n",
					ei->name, ei->value);
			puts("\t\tformat: function(e, name, val)\n"
			     "\t\t{");
		}

		printf( "\t\t\tname += '-label';\n"
		       "\t\t\tif (null === val) {\n"
		       "\t\t\t\t_replcl(e, name, \'not given\', false);\n"
		       "\t\t\t\t_classaddcl(e, name, \'noanswer\', false);\n"
		       "\t\t\t\treturn;\n"
		       "\t\t\t}\n"
		       "\t\t\tswitch(parseInt(val)) {\n");
		TAILQ_FOREACH(ei, &e->eq, entries)
			printf("\t\t\tcase %s.%s:\n"
			       "\t\t\t\t_replcl(e, name, \'%s\', false);\n"
			       "\t\t\t\tbreak;\n",
			       e->name, ei->name, 
			       NULL == ei->jslabel ? 
			       ei->name : ei->jslabel);
		printf("\t\t\tdefault:\n"
		       "\t\t\t\tconsole.log(\'%s.format: "
		         "unknown value: ' + val);\n"
		       "\t\t\t\t_replcl(e, name, \'Unknown\', false);\n"
		       "\t\t\t\tbreak;\n"
		       "\t\t\t}\n"
		       "\t\t}\n"
		       "\t}%s\n"
		       "\n",
		       e->name, tsc ? "" : ";");
	}

	if ( ! tsc) {
		TAILQ_FOREACH(s, &cfg->sq, entries)
			printf("\troot.%s = %s;\n", s->name, s->name);
		TAILQ_FOREACH(bf, &cfg->bq, entries) 
			printf("\troot.%s = %s;\n", bf->name, bf->name);
		TAILQ_FOREACH(e, &cfg->eq, entries)
			printf("\troot.%s = %s;\n", e->name, e->name);
		puts("})(this);");
	} else
		puts("}");
}
