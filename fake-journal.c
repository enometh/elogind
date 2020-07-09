//Using dynamic linker tricks to fake sd_journal_sendv:
//https://stackoverflow.com/questions/45617872/any-way-to-redirect-sd-journal-send-to-stdout-or-stderr
// sd_journal_sendv is BOGUS - it is a macro in elogind's sd-journal.h

#define __USE_GNU
#define _GNU_SOURCE
#include <printf.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <syslog.h>
typedef int bool;

#define _cleanup_(x) __attribute__((__cleanup__(x)))
#define _likely_(x) (__builtin_expect(!!(x), 1))
#define _unlikely_(x) (__builtin_expect(!!(x), 0))
#define _printf_(a, b) __attribute__((__format__(printf, a, b)))
#define _unused_ __attribute__((__unused__))
#define PROTECT_ERRNO                                                   \
        _cleanup_(_reset_errno_) _unused_ int _saved_errno_ = errno

#define UNPROTECT_ERRNO                         \
        do {                                    \
                errno = _saved_errno_;          \
                _saved_errno_ = -1;             \
        } while (false)

#define MAX(a, b) (a > b ? a : b)

static inline void _reset_errno_(int *saved_errno) {
        if (*saved_errno < 0) /* Invalidated by UNPROTECT_ERRNO? */
                return;

        errno = *saved_errno;
}

static inline void freep(void *p) {
        free(*(void**) p);
}

#define _cleanup_free_ _cleanup_(freep)


#define LINE_MAX 1024

#define DECIMAL_STR_MAX(type)                                           \
        (2+(sizeof(type) <= 1 ? 3 :                                     \
            sizeof(type) <= 2 ? 5 :                                     \
            sizeof(type) <= 4 ? 10 :                                    \
            sizeof(type) <= 8 ? 20 : sizeof(int[-2*(sizeof(type) > 8)])))

#define ELEMENTSOF(x) sizeof(x)/sizeof((x)[0])

#define snprintf_ok(buf, len, fmt, ...) \
        ((size_t) snprintf(buf, len, fmt, __VA_ARGS__) < (len))

#define xsprintf(buf, fmt, ...) \
        assert(snprintf_ok(buf, ELEMENTSOF(buf), fmt, __VA_ARGS__))

#define VA_FORMAT_ADVANCE(format, ap)                                   \
do {                                                                    \
        int _argtypes[128];                                             \
        size_t _i, _k;                                                  \
	bzero(_argtypes, sizeof(_argtypes));						\
        _k = parse_printf_format((format), ELEMENTSOF(_argtypes), _argtypes); \
        assert(_k < ELEMENTSOF(_argtypes));                             \
        for (_i = 0; _i < _k; _i++) {                                   \
                if (_argtypes[_i] & PA_FLAG_PTR)  {                     \
                        (void) va_arg(ap, void*);                       \
                        continue;                                       \
                }                                                       \
                                                                        \
                switch (_argtypes[_i]) {                                \
                case PA_INT:                                            \
                case PA_INT|PA_FLAG_SHORT:                              \
                case PA_CHAR:                                           \
                        (void) va_arg(ap, int);                         \
                        break;                                          \
                case PA_INT|PA_FLAG_LONG:                               \
                        (void) va_arg(ap, long int);                    \
                        break;                                          \
                case PA_INT|PA_FLAG_LONG_LONG:                          \
                        (void) va_arg(ap, long long int);               \
                        break;                                          \
                case PA_WCHAR:                                          \
                        (void) va_arg(ap, wchar_t);                     \
                        break;                                          \
                case PA_WSTRING:                                        \
                case PA_STRING:                                         \
                case PA_POINTER:                                        \
                        (void) va_arg(ap, void*);                       \
                        break;                                          \
                case PA_FLOAT:                                          \
                case PA_DOUBLE:                                         \
                        (void) va_arg(ap, double);                      \
                        break;                                          \
                case PA_DOUBLE|PA_FLAG_LONG_DOUBLE:                     \
                        (void) va_arg(ap, long double);                 \
                        break;                                          \
                default:                                                \
                        assert("Unknown format string argument.");      \
                }                                                       \
        }                                                               \
} while (0)

#define assert_return(expr, r)                                          \
        do {                                                            \
	    int ret=!!expr;						\
	    assert(ret);						\
	    if (!ret)							\
		return (r);						\
        } while (0);


#define IOVEC_INIT(base, len) { .iov_base = (base), .iov_len = (len) }
#define IOVEC_MAKE(base, len) (struct iovec) IOVEC_INIT(base, len)
#define IOVEC_INIT_STRING(string) IOVEC_INIT((char*) string, strlen(string))
#define IOVEC_MAKE_STRING(string) (struct iovec) IOVEC_INIT_STRING(string)

#define WHITESPACE        " \t\n\r"



char *delete_chars(char *s, const char *bad) {
        char *f, *t;

        /* Drops all specified bad characters, regardless where in the string */

        if (!s)
                return NULL;

        if (!bad)
                bad = WHITESPACE;

        for (f = s, t = s; *f; f++) {
                if (strchr(bad, *f))
                        continue;

                *(t++) = *f;
        }

        *t = 0;

        return s;
}

char *delete_trailing_chars(char *s, const char *bad) {
        char *p, *c = s;

        /* Drops all specified bad characters, at the end of the string */

        if (!s)
                return NULL;

        if (!bad)
                bad = WHITESPACE;

        for (p = s; *p; p++)
                if (!strchr(bad, *p))
                        c = p + 1;

        *c = 0;

        return s;
}

static inline char *skip_leading_chars(const char *s, const char *bad) {

        if (!s)
                return NULL;

        if (!bad)
                bad = WHITESPACE;

        return (char*) s + strspn(s, bad);
}

static inline bool isempty(const char *p) {
        return !p || !p[0];
}

char *strstrip(char *s) {
        if (!s)
                return NULL;

        /* Drops trailing whitespace. Modifies the string in place. Returns pointer to first non-space character */

        return delete_trailing_chars(skip_leading_chars(s, WHITESPACE), WHITESPACE);
}

static inline char *startswith(const char *s, const char *prefix) {
        size_t l;

        l = strlen(prefix);
        if (strncmp(s, prefix, l) == 0)
                return (char*) s + l;

        return NULL;
}

static inline const char *strna(const char *s) {
        return s ?: "n/a";
}


_printf_(1, 0) static int
fill_iovec_sprintf(const char *format, va_list ap, int extra, struct iovec **_iov)
{
    PROTECT_ERRNO;
    int r, n = 0, i = 0, j;
    struct iovec *iov = NULL;

    assert(_iov);

    if (extra > 0) {
	n = MAX(extra * 2, extra + 4);
	iov = malloc(n * sizeof(struct iovec));
	if (!iov) {
	    r = -ENOMEM;
	    goto fail;
	}

	i = extra;
    }

    while (format) {
	struct iovec *c;
	char *buffer;
	va_list aq;

	if (i >= n) {
	    n = MAX(i*2, 4);
	    c = realloc(iov, n * sizeof(struct iovec));
	    if (!c) {
		r = -ENOMEM;
		goto fail;
	    }

	    iov = c;
	}

	va_copy(aq, ap);
	if (vasprintf(&buffer, format, aq) < 0) {
	    va_end(aq);
	    r = -ENOMEM;
	    goto fail;
	}
	va_end(aq);

	VA_FORMAT_ADVANCE(format, ap);

	(void) strstrip(buffer); /* strip trailing whitespace, keep prefixing whitespace */

	iov[i++] = IOVEC_MAKE_STRING(buffer);

	format = va_arg(ap, char *);
    }

    *_iov = iov;

    return i;

 fail:
    for (j = 0; j < i; j++)
	free(iov[j].iov_base);

    free(iov);

    return r;
}


// BOGUS - it is a macro in sd-journal.h
int
sd_journal_sendv(const struct iovec *iov, int n) {
  /* send values seperated by newline */
  while (n-- > 0) {
    writev(1, iov+n, 1);//
    write(1,"\n", 1);
  }
}

int
sd_journal_send(const char *format, ...)
{
    int r, i, j;
    va_list ap;
    struct iovec *iov = NULL;

    va_start(ap, format);
    i = fill_iovec_sprintf(format, ap, 0, &iov);
    va_end(ap);

    if (_unlikely_(i < 0)) {
	r = i;
	goto finish;
    }

    r = sd_journal_sendv(iov, i);

 finish:
    for (j = 0; j < i; j++)
	free(iov[j].iov_base);

    free(iov);

    return r;
}



int
sd_journal_printv(int priority, const char *format, va_list ap)
{

    /* FIXME: Instead of limiting things to LINE_MAX we could do a
       C99 variable-length array on the stack here in a loop. */


    char p[strlen("PRIORITY=") + DECIMAL_STR_MAX(int) + 1];
    struct iovec iov[2];

    char buffer[8 + LINE_MAX]; // We keep the +8 to not make a too big mess below.

    assert_return(priority >= 0, -EINVAL);
    assert_return(priority <= 7, -EINVAL);
    assert_return(format, -EINVAL);

//#if 0 /// No bells and whistles needed in elogind
    xsprintf(p, "PRIORITY=%i", priority & LOG_PRIMASK);

    memcpy(buffer, "MESSAGE=", 8);
//#endif // 0
    vsnprintf(buffer+8, sizeof(buffer) - 8, format, ap);

    /* Strip trailing whitespace, keep prefix whitespace. */
    (void) strstrip(buffer);

    /* Suppress empty lines */
    if (isempty(buffer+8))
	return 0;

//#if 0 /// As elogind does not talk to systemd-journal, use syslog.
    iov[0] = IOVEC_MAKE_STRING(buffer);
    iov[1] = IOVEC_MAKE_STRING(p);

    return sd_journal_sendv(iov, 2);
//#else // 0
    syslog(LOG_DAEMON | priority, "%s", buffer + 8);
    return 0;
//#endif // 0
}


int
sd_journal_print(int priority, const char *format, ...)
{
    int r;
    va_list ap;

    va_start(ap, format);
    r = sd_journal_printv(priority, format, ap);
    va_end(ap);

    return r;
}

int
sd_journal_printv_with_location(int priority, const char *file, const char *line, const char *func, const char *format, va_list ap)
{
        assert_return(priority >= 0, -EINVAL);
        assert_return(priority <= 7, -EINVAL);
        assert_return(format, -EINVAL);

        char buffer[8 + LINE_MAX];
        vsnprintf(buffer, sizeof(buffer), format, ap);

        /* Strip trailing whitespace, keep prefixing whitespace */
        (void) strstrip(buffer);

        /* Suppress empty lines */
        if (isempty(buffer))
                return 0;

        /* just prefix with the data we have */
        return sd_journal_print(priority, "%s:%s:%s:%s", strna(file), strna(line), strna(func), buffer);
}


int
sd_journal_print_with_location(int priority, const char *file, const char *line, const char *func, const char *format, ...)
{
    int r;
    va_list ap;

    va_start(ap, format);
    r = sd_journal_printv_with_location(priority, file, line, func, format, ap);
    va_end(ap);

    return r;
}


int
sd_journal_sendv_with_location(const char *file, const char *line,
			       const char *func,
			       const struct iovec *iov, int n)
{

        assert_return(iov, -EINVAL);
        assert_return(n > 0, -EINVAL);

        int   i;
        int   priority = LOG_INFO;
        char *message = NULL;

        for (i = 0; i < n ; ++i) {
                if (startswith(iov[i].iov_base, "PRIORITY=")
                                && (1 == sscanf(iov[i].iov_base, "PRIORITY=%i", &priority)))
                        continue;
                if ((message = startswith(iov[i].iov_base, "MESSAGE=")))
                        break;
        }

        if (message)
                return sd_journal_print_with_location(priority, file, line, func, "%s", message);

        return 0;
}


int
sd_journal_send_with_location(const char *file, const char *line, const char *func, const char *format, ...) {
        _cleanup_free_ struct iovec *iov = NULL;
        int r, i, j;
        va_list ap;

        va_start(ap, format);
        i = fill_iovec_sprintf(format, ap, 3, &iov);
        va_end(ap);

        if (_unlikely_(i < 0)) {
                r = i;
                goto finish;
        }
        r = sd_journal_sendv_with_location(file, line, func, &iov[3], i - 3);
finish:
        for (j = 3; j < i; j++)
                free(iov[j].iov_base);

        return r;
}



/*
Compile:

gcc -shared -fPIC  fake-journal.c -o fake-journal.so -ldl

LD_DEBUG=all
LD_PRELOAD=$PWD/fake-journal.so ./a.out
PRIORITY=6
MESSAGE=Hello World, this is PID 18772!
CODE_FUNC=main
CODE_LINE=8
CODE_FILE=test.c
*/
