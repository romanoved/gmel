#define _GNU_SOURCE
#define _XOPEN_SOURCE

#include "popen.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int* GMEL_PIDS;

FILE* gmel_popen_wrapper(char** args, char* type) {
    FILE* iop;
    int pdes[2], fds, pid;

    if ((*type != 'r' && *type != 'w') || type[1]) return (NULL);

    if (GMEL_PIDS == NULL) {
        if ((fds = getdtablesize()) <= 0) return (NULL);
        if ((GMEL_PIDS = (int*)malloc((u_int)(fds * sizeof(int)))) == NULL)
            return (NULL);
        bzero((char*)GMEL_PIDS, fds * sizeof(int));
    }
    if (pipe(pdes) < 0) return (NULL);
    switch (pid = vfork()) {
        case -1: /* error */
            (void)close(pdes[0]);
            (void)close(pdes[1]);
            return (NULL);
            /* NOTREACHED */
        case 0: /* child */
            if (*type == 'r') {
                if (pdes[1] != fileno(stdout)) {
                    (void)dup2(pdes[1], fileno(stdout));
                    (void)close(pdes[1]);
                }
                (void)close(pdes[0]);
            } else {
                if (pdes[0] != fileno(stdin)) {
                    (void)dup2(pdes[0], fileno(stdin));
                    (void)close(pdes[0]);
                }
                (void)close(pdes[1]);
            }
            execvp(args[0], args);
            _exit(127);
            /* NOTREACHED */
    }
    /* parent; assume fdopen can't fail...  */
    if (*type == 'r') {
        iop = fdopen(pdes[0], type);
        (void)close(pdes[1]);
    } else {
        iop = fdopen(pdes[1], type);
        (void)close(pdes[0]);
    }
    GMEL_PIDS[fileno(iop)] = pid;
    return (iop);
}

int gmel_pclose_wrapper(FILE* iop) {
    int fdes;
    sigset_t omask, nmask;
    union wait pstat;
    int pid;

    /*
     * pclose returns -1 if stream is not associated with a
     * `popened' command, if already `pclosed', or waitpid
     * returns an error.
     */
    if (GMEL_PIDS == NULL || GMEL_PIDS[fdes = fileno(iop)] == 0) return (-1);
    (void)fclose(iop);
    sigemptyset(&nmask);
    sigaddset(&nmask, SIGINT);
    sigaddset(&nmask, SIGQUIT);
    sigaddset(&nmask, SIGHUP);
    (void)sigprocmask(SIG_BLOCK, &nmask, &omask);
    do {
        pid = waitpid(GMEL_PIDS[fdes], (int*)&pstat, 0);
    } while (pid == -1 && errno == EINTR);
    (void)sigprocmask(SIG_SETMASK, &omask, NULL);
    GMEL_PIDS[fdes] = 0;
    return (pid == -1 ? -1 : pstat.w_status);
}

int gmel_append_buf(char** buf, size_t* buf_size, unsigned int* total_readed,
                    char* src, size_t src_size) {
    char* tmp_buf;
    size_t required = *total_readed + src_size;
    size_t new_bus_size = *buf_size;

    if (!src_size) return 0;

    if (*buf_size < required) {
        new_bus_size =
            *buf_size + (required > *buf_size ? required : *buf_size);
        tmp_buf = (char*)GMEL_ALLOC(new_bus_size);
        memcpy(tmp_buf, *buf, *buf_size);
        free(*buf);
        *buf = tmp_buf;
    } else
        tmp_buf = *buf;
    tmp_buf = tmp_buf + *total_readed;
    memcpy(tmp_buf, src, src_size);
    *total_readed += src_size;
    *buf_size = new_bus_size;
    return 0;
}

void gmel_fold_newlines(char* buffer, unsigned int* length, int trim_newlines) {
    char* dst = buffer;
    char* src = buffer;
    char* last_nonnl = buffer - 1;
    src[*length] = 0;
    for (; *src != '\0'; ++src) {
        if (src[0] == '\r' && src[1] == '\n') continue;
        if (*src == '\n') {
            *dst++ = ' ';
        } else {
            last_nonnl = dst;
            *dst++ = *src;
        }
    }

    if (!trim_newlines && (last_nonnl < (dst - 2))) last_nonnl = dst - 2;

    *(++last_nonnl) = '\0';
    *length = last_nonnl - buffer;
}

void expand_escapes(char* dest, const char* src) {
    char c;

    while ((c = *(src++))) {
        switch (c) {
            case '\a':
                *(dest++) = '\\';
                *(dest++) = 'a';
                break;
            case '\b':
                *(dest++) = '\\';
                *(dest++) = 'b';
                break;
            case '\t':
                *(dest++) = '\\';
                *(dest++) = 't';
                break;
            case '\n':
                *(dest++) = '\\';
                *(dest++) = 'n';
                break;
            case '\v':
                *(dest++) = '\\';
                *(dest++) = 'v';
                break;
            case '\f':
                *(dest++) = '\\';
                *(dest++) = 'f';
                break;
            case '\r':
                *(dest++) = '\\';
                *(dest++) = 'r';
                break;
            case '\\':
                *(dest++) = '\\';
                *(dest++) = '\\';
                break;
            case '\"':
                *(dest++) = '\\';
                *(dest++) = '\"';
                break;
            default:
                *(dest++) = c;
        }
    }

    *dest = '\0';
}

char* sprintf_argv(char** argv) {
    char* tmp = NULL;
    char* escaped_arg = NULL;
    char* rez = (char*)malloc(2 * sizeof(char));
    if (!rez) {
        perror("util_common.h::sprintf_argv malloc failed");
        abort();
    }
    rez[0] = '[';
    rez[1] = '\0';

    while (*argv)
        for (; *argv; ++argv) {
            escaped_arg = (char*)malloc(2 * strlen(*argv) + 1);
            if (!escaped_arg) {
                perror("util_common.h::sprintf_argv malloc failed");
                abort();
            }
            expand_escapes(escaped_arg, *argv);
            tmp = safe_sprintf("%s'%s'%s", rez, escaped_arg,
                               *(argv + 1) ? ", " : "");
            free(escaped_arg);
            free(rez);
            rez = tmp;
        }
    tmp = safe_sprintf("%s]", rez);
    free(rez);
    return tmp;
}

char* gmel_popen(char* func_name, int argc, char** argv) {
    char* err_str = "";
    size_t readed;
    char read_buffer[4096];
    size_t read_buf_size = sizeof(read_buffer);

    char* buf = NULL;
    size_t buf_size = 0;

    unsigned int total_readed = 0;
    int ret_code = 0;

    FILE* proc = gmel_popen_wrapper(argv, "r");
    if (!proc) {
        err_str = safe_sprintf("$(error %s: popen failed on %s (%s))",
                               func_name, sprintf_argv(argv), strerror(errno));
        gmk_expand(err_str);
        perror(err_str);
        abort();
        return NULL;
    }

    buf = (char*)GMEL_ALLOC(read_buf_size);
    buf_size = read_buf_size;
    *buf = 0;

    while ((readed = fread(read_buffer, 1, read_buf_size, proc))) {
        gmel_append_buf(&buf, &buf_size, &total_readed, read_buffer, readed);
    }

    ret_code = gmel_pclose_wrapper(proc);
    if (ret_code) {
        free(buf);
        err_str = safe_sprintf("$(error %s: %s exit with code %d)", func_name,
                               sprintf_argv(argv), ret_code / 256);
        gmk_expand(err_str);
        perror(err_str);
        abort();
        return NULL;
    }
    gmel_fold_newlines(buf, &total_readed, 1);
    return buf;
}
