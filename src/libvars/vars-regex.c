/*!
  @defgroup regex Regular expressions
  @ingroup types

  A regular expression (or regexp) is a pattern used to match strings.
*/

/*!
  @defgroup regex_create Creating and destroying regexps
  @ingroup regex
*/

/*!
  @defgroup regex_match Matching and substituting strings
  @ingroup regex
*/

/*!
  @defgroup regex_misc Other regexp functions
  @ingroup regex
*/

/*!
  @defgroup regex_syntax Regexp syntax
  @ingroup regex

  This regexp syntax description is excerpted from the GNU regex manual.

  Regular expressions have a syntax in which a few characters are special
  constructs and the rest are "ordinary".  An ordinary character is a
  simple regular expression which matches that character and nothing else.
  The special characters are <tt>$</tt>, <tt>^</tt>, <tt>.</tt>,
  <tt>*</tt>, <tt>+</tt>, <tt>?</tt>, <tt>[</tt>, <tt>]</tt> and
  <tt>\\</tt>.  Any other character appearing in a regular expression is
  ordinary, unless a <tt>\\</tt> precedes it.

  For example, <tt>f</tt> is not a special character, so it is ordinary,
  and therefore <tt>f</tt> is a regular expression that matches the string
  <tt>f</tt> and no other string.  (It does \e not match the string
  <tt>ff</tt>.)  Likewise, <tt>o</tt> is a regular expression that matches
  only <tt>o</tt>.

  Any two regular expressions A and B can be concatenated.  The result is a
  regular expression which matches a string if A matches some amount of the
  beginning of that string and B matches the rest of the string.

  As a simple example, we can concatenate the regular expressions
  <tt>f</tt> and <tt>o</tt> to get the regular expression <tt>fo</tt>,
  which matches only the string <tt>fo</tt>. Still trivial.

  Note: for Unix compatibility, special characters are treated as ordinary
  ones if they are in contexts where their special meanings make no sense.
  For example, <tt>*foo</tt> treats <tt>*</tt> as ordinary since there is
  no preceding expression on which the <tt>*</tt> can act.  It is poor
  practice to depend on this behavior; better to quote the special
  character anyway, regardless of where is appears.

  The following are the characters and character sequences which have
  special meaning within regular expressions.  Any character not mentioned
  here is not special; it stands for exactly itself for the purposes of
  searching and matching.

  @par Sequence: .

  <tt>.</tt> is a special character that matches anything except a newline.
  Using concatenation, we can make regular expressions like <tt>a.b</tt>
  which matches any three-character string which begins with <tt>a</tt> and
  ends with <tt>b</tt>.

  @par Sequence: *

  <tt>*</tt> is not a construct by itself; it is a suffix, which means the
  preceding regular expression is to be repeated as many times as possible.
  In <tt>fo*</tt>, the <tt>*</tt> applies to the <tt>o</tt>, so
  <tt>fo*</tt> matches <tt>f</tt> followed by any number of <tt>o</tt>'s.

  The case of zero <tt>o</tt>'s is allowed: <tt>fo*</tt> does match
  <tt>f</tt>.

  <tt>*</tt> always applies to the \e smallest possible preceding
  expression.  Thus, <tt>fo*</tt> has a repeating <tt>o</tt>, not a
  repeating <tt>fo</tt>.

  The matcher processes a <tt>*</tt> construct by matching, immediately, as
  many repetitions as can be found.  Then it continues with the rest of the
  pattern.  If that fails, backtracking occurs, discarding some of the
  matches of the <tt>*</tt>'d construct in case that makes it possible to
  match the rest of the pattern.  For example, matching <tt>c[ad]*ar</tt>
  against the string <tt>caddaar</tt>, the <tt>[ad]*</tt> first matches
  <tt>addaa</tt>, but this does not allow the next <tt>a</tt> in the
  pattern to match.  So the last of the matches of <tt>[ad]</tt> is undone
  and the following <tt>a</tt> is tried again.  Now it succeeds.

  @par Sequence: +

  <tt>+</tt> is like <tt>*</tt> except that at least one match for the
  preceding pattern is required for <tt>+</tt>.  Thus, <tt>c[ad]+r</tt>
  does not match <tt>cr</tt> but does match anything else that
  <tt>c[ad]*r</tt> would match.

  <tt>?</tt> is like <tt>*</tt> except that it allows either zero or one
  match for the preceding pattern.  Thus, <tt>c[ad]?r</tt> matches
  <tt>cr</tt> or <tt>car</tt> or <tt>cdr</tt>, and nothing else.

  @par Sequence: [ ... ]

  <tt>[</tt> begins a "character set", which is terminated by a <tt>]</tt>.
  In the simplest case, the characters between the two form the set.  Thus,
  <tt>[ad]</tt> matches either <tt>a</tt> or <tt>d</tt>, and <tt>[ad]*</tt>
  matches any string of <tt>a</tt>'s and <tt>d</tt>'s (including the empty
  string), from which it follows that <tt>c[ad]*r</tt> matches
  <tt>car</tt>, etc.

  Character ranges can also be included in a character set, by writing two
  characters with a <tt>-</tt> between them.  Thus, <tt>[a-z]</tt> matches
  any lower-case letter.  Ranges may be intermixed freely with individual
  characters, as in <tt>[a-z$%.]</tt>, which matches any lower case letter
  or <tt>$</tt>, <tt>%</tt> or period.

  Note that the usual special characters are not special any more inside a
  character set.  A completely different set of special characters exists
  inside character sets: <tt>]</tt>, <tt>-</tt> and <tt>^</tt>.

  To include a <tt>]</tt> in a character set, you must make it the first
  character.  For example, <tt>[]a]</tt> matches <tt>]</tt> or <tt>a</tt>.
  To include a <tt>-</tt>, you must use it in a context where it cannot
  possibly indicate a range: that is, as the first character, or
  immediately after a range.

  @par Sequence: [^ ... ]

  <tt>[^</tt> begins a "complement character set", which matches any
  character except the ones specified.  Thus, <tt>[^a-z0-9A-Z]</tt> matches
  all characters \e except letters and digits.

  <tt>^</tt> is not special in a character set unless it is the first
  character.  The character following the <tt>^</tt> is treated as if it
  were first (it may be a <tt>-</tt> or a <tt>]</tt>).

  @par Sequence: ^

  <tt>^</tt> is a special character that matches the empty string -- but
  only if at the beginning of a line in the text being matched.  Otherwise
  it fails to match anything. Thus, <tt>^foo</tt> matches a <tt>foo</tt>
  which occurs at the beginning of a line.

  @par Sequence: $

  <tt>$</tt> is similar to <tt>^</tt> but matches only at the end of a
  line.  Thus, <tt>xx*$</tt> matches a string of one or more <tt>x</tt>'s
  at the end of a line.

  @par Sequence: \\

  <tt>\\</tt> has two functions: it quotes the above special characters
  (including <tt>\\</tt>), and it introduces additional special constructs.

  Because <tt>\\</tt> quotes special characters, <tt>\\$</tt> is a regular
  expression which matches only <tt>$</tt>, and <tt>\\[</tt> is a regular
  expression which matches only <tt>[</tt>, and so on.

  For the most part, <tt>\\</tt> followed by any character matches only that
  character.  However, there are several exceptions: characters which, when
  preceded by <tt>\\</tt>, are special constructs.  Such characters are
  always ordinary when encountered on their own.

  No new special characters will ever be defined.  All extensions to the
  regular expression syntax are made by defining new two-character
  constructs that begin with <tt>\\</tt>.

  @par Sequence: \\|

  <tt>\\|</tt> specifies an alternative.  Two regular expressions A and B
  with <tt>\\|</tt> in between form an expression that matches anything that
  either A or B will match.

  Thus, <tt>foo\\|bar</tt> matches either <tt>foo</tt> or <tt>bar</tt> but
  no other string.

  <tt>\\|</tt> applies to the largest possible surrounding expressions.
  Only a surrounding <tt>\\( ... \\)</tt> grouping can limit the grouping
  power of <tt>\\|</tt>.

  Full backtracking capability exists when multiple <tt>\\|</tt>'s are used.

  @par Sequence: \\( ... \\)

  <tt>\\( ... \\)</tt> is a grouping construct that serves three purposes:

    -# To enclose a set of <tt>\\|</tt> alternatives for other operations.
       Thus, <tt>\\(foo\\|bar\\)x</tt> matches either <tt>foox</tt> or
       <tt>barx</tt>.
    -# To enclose a complicated expression for the postfix <tt>*</tt> to
       operate on.  Thus, <tt>ba\\(na\\)*</tt> matches <tt>bananana</tt>,
       etc., with any (zero or more) number of <tt>na</tt>'s.
    -# To mark a matched substring for future reference.  This last
       application is not a consequence of the idea of a parenthetical
       grouping; it is a separate feature which happens to be assigned as a
       second meaning to the same <tt>\\( ... \\)</tt> construct because
       there is no conflict in practice between the two meanings.  Here is
       an explanation of this feature:
    .

  @par Sequence: \\DIGIT

  After the end of a <tt>\\( ... \\)</tt> construct, the matcher remembers
  the beginning and end of the text matched by that construct.  Then, later
  on in the regular expression, you can use <tt>\\</tt> followed by DIGIT to
  mean "match the same text matched the DIGIT'th time by the <tt>\\(
  ... \\)</tt> construct."  The <tt>\\( ... \\)</tt> constructs are numbered
  in order of commencement in the regexp.

  The strings matching the first nine <tt>\\( ... \\)</tt> constructs
  appearing in a regular expression are assigned numbers 1 through 9 in
  order of their beginnings.  <tt>\\1</tt> through <tt>\\9</tt> may be used
  to refer to the text matched by the corresponding <tt>\\( ... \\)</tt>
  construct.

  For example, <tt>\\(.*\\)\\1</tt> matches any string that is composed of two
  identical halves.  The <tt>\\(.*\\)</tt> matches the first half, which may
  be anything, but the <tt>\\1</tt> that follows must match the same exact
  text.

  @par Other constructs

  <tt>\\`</tt> matches the empty string, but only at the beginning of the
  string being matched against.

  <tt>\\'</tt> matches the empty string, but only at the end of the string
  being matched against.

  <tt>\\b</tt> matches the empty string, but only if it is at the beginning
  or end of a word.  Thus, <tt>\\bfoo\\b</tt> matches any occurrence of
  <tt>foo</tt> as a separate word.  <tt>\\bball\\(s\\|\\)\\b</tt> matches
  <tt>ball</tt> or <tt>balls</tt> as a separate word.

  <tt>\\B</tt> matches the empty string, provided it is \e not at the
  beginning or end of a word.

  <tt>\\<</tt> matches the empty string, but only if it is at the beginning
  of a word.

  <tt>\\></tt> matches the empty string, but only if it is at the end of a
  word.

  <tt>\\w</tt> matches any word-constituent character.

  <tt>\\W</tt> matches any character that is not a word-constituent.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>

#include "vars-config.h"
#include "vars-buffer.h"
#include "vars-macros.h"
#include "vars-memory.h"
#include "vars-regex.h"
#include "vars-system.h"

/* Type definition */
struct v_regex {
    struct v_header id;         /* Type marker */
    char *name;                 /* Regexp string */
    struct re_pattern_buffer buf; /* Compiled pattern */
};

/* Type variable */
vtype *vregex_type = NULL;

/* Default pattern buffer size */
static int patbufsize = 100;

/* Last string matched */
static char *laststring;

/* Did last regexp compile? */
static int compiled;

/* List of matches from last match */
static struct re_registers matchlist;

/* Error message from last match (if any) */
V_NBUF_DECL(errbuf);

/* Internal functions */
static int vr_xmldump(vregex *r, FILE *fp);

/* Check if pointer is a regexp */
int
vr_check(void *ptr)
{
    return (ptr != NULL && v_type(ptr) == vregex_type);
}

/*!
  @brief   Return a copy of a regexp.
  @ingroup regex_create
  @param   r Regexp.
  @return  Copy of the regexp.
*/
vregex *
vr_copy(vregex *r)
{
    return vr_create(r->name);
}

/*!
  @brief   Create a new regexp.
  @ingroup regex_create
  @param   str Regexp string.
  @return  Regexp.
  @retval  NULL if string is not a valid regexp.
*/
vregex *
vr_create(char *str)
{
    return vr_create_translate(str, NULL);
}

/*!
  @brief   Create a new regexp with a case-folding translate table.
  @ingroup regex_create
  @param   str Regexp string.
  @return  Regexp.

  Like vr_create(), but set the regexp to ignore upper/lower case
  differences when matching.
*/
vregex *
vr_create_nocase(char *str)
{
    static char *nocase_table = NULL;
    int i;

    if (nocase_table == NULL) {
        nocase_table = V_ALLOC(char, 256);

        for (i = 0; i < 0400; i++)
            nocase_table[i] = i;

        for (i = 'a'; i <= 'z'; i++)
            nocase_table[i] = i - 040;
    }

    return vr_create_translate(str, nocase_table);
}

/*!
  @brief   Create a new regexp with a translate table.
  @ingroup regex_create
  @param   str Regexp string.
  @param   table Translation table.
  @return  Regexp.

  Specify a translate table to use when matching characters.  The table
  should be a 256-element array, one element per ASCII character, with each
  entry being the character value to translate to.
*/
vregex *
vr_create_translate(char *str, char *table)
{
    static vheader *id = NULL;
    const char *error;
    vregex *r;

    if (id == NULL) {
        vr_declare();
        id = v_header(vregex_type);
    }

    r = V_ALLOC(vregex, 1);

    r->id = *id;
    r->name = V_STRDUP(str);

    r->buf.allocated = patbufsize;
    r->buf.buffer = V_ALLOC(unsigned char, patbufsize);
    r->buf.fastmap = V_ALLOC(char, 256);
    r->buf.translate = table;

    if ((error = re_compile_pattern(str, strlen(str), &r->buf)) != NULL) {
        V_NBUF_SET(errbuf, (char *) error);
        vr_destroy(r);
        compiled = 0;
        return NULL;
    }

    re_compile_fastmap(&r->buf);
    compiled = 1;

    return r;
}

/* Declare regexp type */
vtype *
vr_declare(void)
{
    if (vregex_type == NULL) {
        vregex_type = v_create("REGEX", "R");
        v_copy_func(vregex_type, (void *(*)()) vr_copy);
        v_read_func(vregex_type, (void *(*)()) vr_read);
        v_write_func(vregex_type, vr_write);
        v_freeze_func(vregex_type, vr_freeze);
        v_thaw_func(vregex_type, (void *(*)()) vr_thaw);
        v_print_func(vregex_type, vr_print);
        v_destroy_func(vregex_type, vr_destroy);
        v_traverse_func(vregex_type, vr_traverse);
        v_xmldump_func(vregex_type, vr_xmldump);
    }

    return vregex_type;
}

/*!
  @brief   Deallocate a regexp.
  @ingroup regex_create
  @param   r Regexp.
*/
void
vr_destroy(vregex *r)
{
    VR_CHECK(r);

    V_DEALLOC(r->name);
    V_DEALLOC(r->buf.buffer);

    if (r->buf.fastmap != NULL)
        V_DEALLOC(r->buf.fastmap);

    V_DEALLOC(r);
}

/*!
  @brief   Return last regexp compilation error.
  @ingroup regex_misc
  @return  Error string.
  @retval  NULL if there was no error.
*/
char *
vr_error(void)
{
    return (compiled ? NULL : V_NBUF_VAL(errbuf));
}

/*!
  @brief   Return bracketed parts of a string that matches a regexp.
  @ingroup regex_match
  @param   string String to match.
  @param   r Regexp.
  @return  List of matches.
  @retval  NULL if nothing matched.
*/
vlist *
vr_extract(char *string, vregex *r)
{
    char *match;
    int n = 0;
    vlist *l;

    VR_CHECK(r);

    l = vl_create();

    if (!vr_match(string, r))
        return NULL;

    while ((match = vr_matched(n++)) != NULL)
        vl_spush(l, match);

    return l;
}

/* Freeze a regexp to a stream */
int
vr_freeze(vregex *r, FILE *fp)
{
    VR_CHECK(r);

    v_freeze_start(fp);
    v_freeze_string(r->name, fp);
    v_freeze_finish(fp);

    return 1;
}

/*!
  @brief   Return a sublist of a list which matches a regexp.
  @ingroup regex_match
  @param   l List of strings.
  @param   r Regexp.
  @return  List of matched strings.
*/
vlist *
vr_grep(vlist *l, vregex *r)
{
    viter iter;
    vlist *m;

    VL_CHECK(l);
    VR_CHECK(r);

    m = vl_create();

    v_iterate(l, iter)
        if (vr_match(vl_iter_svalref(iter), r))
            vl_push(m, vs_copy(vl_iter_val(iter)));

    return m;
}

/*!
  @brief   Check for a regexp match.
  @ingroup regex_match
  @param   str String to match.
  @param   r Regexp.
  @return  Whether it matched.
*/
int
vr_match(char *str, vregex *r)
{
    int result, len = strlen(str);

    VR_CHECK(r);

    laststring = str;
    result = re_search(&r->buf, str, len, 0, len, &matchlist);

    return (result >= 0);
}

/*!
  @brief   Return the Nth bracketed match.
  @ingroup regex_match
  @param   num Match number.
  @return  Matched string.

  Returns parts of the string that matched in the last vr_match() call.
  With \c num = 0, returns the whole substring that matched.  With \c num >
  0, returns the <tt>num</tt>'th bracket match (part of the regexp enclosed
  by <tt>\\(</tt> and <tt>\\)</tt>).  If \c num is greater than the number
  of brackets in the regexp, returns \c NULL.  Note that the string
  argument to the last vr_match() call should \e not have been modified
  when you use this function, or chaos may result.
*/
char *
vr_matched(int num)
{
    static vbuffer *b = NULL;
    int i;

    /* Initialise */
    vb_init(b);

    /* Get match */
    if (num < 0 || num >= RE_NREGS)
        return NULL;

    if (matchlist.start[num] < 0)
        return NULL;

    for (i = matchlist.start[num]; i < matchlist.end[num]; i++)
        vb_putc(b, laststring[i]);

    return vb_get(b);
}

/* Print contents of a regexp */
void
vr_print(vregex *r, FILE *fp)
{
    VR_CHECK(r);

    v_print_type(vregex_type, r, fp);

    v_print_start();
    v_push_indent();

    v_indent(fp);
    fprintf(fp, "%s\n", r->name);

    v_pop_indent();
    v_print_finish();
}

/* Read regexp from a stream */
vregex *
vr_read(FILE *fp)
{
    char *pat;

    /* Read regexp pattern */
    if ((pat = v_read_string(fp)) == NULL)
        return NULL;

    return vr_create(pat);
}

/*!
  @brief   Return the regexp string used to build a regexp.
  @ingroup regex_misc
  @param   r Regexp.
  @return  Regexp string.
*/
char *
vr_regexp(vregex *r)
{
    return r->name;
}

/*!
  @brief   Return a string with matches substituted for a string.
  @ingroup regex_match
  @param   str String to match.
  @param   r Regexp
  @param   sub Substitution string.
  @return  Result (pointer to internal buffer).

  Substitute all matches for a regexp in a string with another string, and
  return the result. If \c sub is \c NULL, no replacement is done.
*/
char *
vr_subst(char *str, vregex *r, char *sub)
{
    static vbuffer *b = NULL;
    int len;

    VR_CHECK(r);

    /* Initialise */
    vb_init(b);

    while (*str != '\0') {
        if ((len = re_match(&r->buf, str, strlen(str), 0, NULL)) > 0) {
            /* Skip over matched part of string */
            while (len-- > 0)
                str++;

            /* Add substitution to result if required */
            if (sub != NULL)
                vb_puts(b, sub);
        } else {
            vb_putc(b, *str++);
        }
    }

    return vb_get(b);
}

/* Thaw a regexp from file */
vregex *
vr_thaw(FILE *fp)
{
    vregex *r;

    v_thaw_start();

    if (!v_thaw_follow(fp, V_TOKEN_STRING, "regexp string"))
        goto fail;

    if ((r = vr_create(v_thaw_svalue)) == NULL) {
        v_thaw_err("invalid regexp: %s (%s)", v_thaw_svalue, vr_error());
        goto fail;
    }

    v_thaw_finish();

    return r;

  fail:
    v_thaw_finish();
    return NULL;
}

/* Traverse a regexp */
int
vr_traverse(vregex *r, int (*func)(void *ptr))
{
    int val;

    VR_CHECK(r);

    if ((val = func(r)) != 0)
        return val;

    v_push_traverse(r);
    v_pop_traverse();

    return 0;
}

/* Write regexp to a stream */
int
vr_write(vregex *r, FILE *fp)
{
    VR_CHECK(r);

    return v_write_string(r->name, fp);
}

/* Dump XML regex to a stream */
static int
vr_xmldump(vregex *r, FILE *fp)
{
    VR_CHECK(r);

    v_xmldump_start(fp);
    v_xmldump_data(fp, "string", r->name);
    v_xmldump_finish(fp);

    return 1;
}
