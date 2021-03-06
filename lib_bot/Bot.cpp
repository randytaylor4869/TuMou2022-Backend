#include "Bot.h"

#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <windows.h>
#pragma warning(disable : 4996)
/*
  Deprecated POSIX names:
    write, open
 */
#undef STDIN_FILENO
#undef STDOUT_FILENO
#undef STDERR_FILENO
#define STDIN_FILENO  _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define STDERR_FILENO _fileno(stderr)
#else
#include <poll.h>
#include <sys/wait.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define quq(__syscall, ...) _quq(#__syscall, __syscall(__VA_ARGS__))

static int _quq(const char *name, int ret)
{
    if (ret == -1) {
        fprintf(stderr, "%s() failed > < [errno %d]\n", name, errno);
        exit(1);
    }
    return ret;
}

static inline long diff_ms(const struct timespec t1, const struct timespec t2)
{
    long ret = (t2.tv_sec - t1.tv_sec) * 1000;
    ret += (1000000000 + t2.tv_nsec - t1.tv_nsec) / 1000000 - 1000;
    return ret;
}

static inline char tenacious_write(int fd, const char *buf, size_t len)
{
    size_t ptr = 0;
    while (ptr < len) {
        int written = write(fd, buf + ptr, len - ptr);
        if (written == -1) return -1;
        ptr += written;
    }
    return 0;
}

/* Sends data prefixed with its 24-bit length to a file descriptor. */
static int bot_send_blob(int pipe, size_t len, const char *payload)
{
    if (len == 0) len = strlen(payload);
    if (len > 0xffffff) return BOT_ERR_TOOLONG;
    char len_buf[3] = {
        (char)(len & 0xff),
        (char)((len >> 8) & 0xff),
        (char)((len >> 16) & 0xff)
    };
    if (tenacious_write(pipe, len_buf, 3) < 0 ||
        tenacious_write(pipe, payload, len) < 0)
    {
        fprintf(stderr, "write() failed with errno %d\n", errno);
        return BOT_ERR_SYSCALL;
    }
    return BOT_ERR_NONE;
}

/*
  Receives data from a file descriptor with a given timeout.
  Returns a pointer to the data and stores the length in *o_len.
  In case of errors, returns NULL, stores the error code (see above) in *o_len,
  and prints related messages to stderr.
 */
static char *bot_recv_blob(int pipe, size_t *o_len, int timeout)
{
#ifdef _WIN32
    /* Under windows, a simple implementation without timeout is employed */
    unsigned char buf[4];
    if (_read(pipe, buf, 3) < 3) {
        *o_len = BOT_ERR_FMT;
        return NULL;
    }
    int len = ((unsigned int)buf[2] << 16) |
        ((unsigned int)buf[1] << 8) |
        (unsigned int)buf[0];

    char *ret = (char *)malloc(len + 1);
    int ptr = 0;

    while (ptr < len) {
        int read_len = _read(pipe, ret + ptr, len - ptr);
        if (read_len == -1) {
            free(ret);
            *o_len = BOT_ERR_SYSCALL;
            return NULL;
        }
        ptr += read_len;
    }

#else
    struct pollfd pfd = (struct pollfd){pipe, POLLIN, 0};
    char *ret = NULL;
    size_t len = 0, ptr = 0;
    unsigned char buf[4];
    struct timespec t1, t2;
    int loops = 0;

    while (len == 0 || ptr < len) {
        /* Unlikely; in case poll()'s time slightly differs
           from CLOCK_MONOTONIC, or rounding errors happen */
        if ((timeout <= 0 || ++loops >= 1000000) && timeout != -1) {
            fprintf(stderr, "Unidentifiable exception with errno %d\n", errno);
            if (ret) free(ret);
            *o_len = BOT_ERR_SYSCALL;
            return NULL;
        }

        /* Keep time */
        clock_gettime(CLOCK_MONOTONIC, &t1);
        /* Wait for reading */
        pfd.revents = 0;
        int poll_ret = poll(&pfd, 1, timeout);
        if (poll_ret == -1) {
            fprintf(stderr, "poll() failed with errno %d\n", errno);
            if (ret) free(ret);
            *o_len = BOT_ERR_SYSCALL;
            return NULL;
        }
        /* Calculate remaining time */
        clock_gettime(CLOCK_MONOTONIC, &t2);
        if (timeout > 0) timeout -= diff_ms(t1, t2);

        /* Is the pipe still open on the other side? */
        if (pfd.revents & POLLHUP) {
            if (ret) free(ret);
            *o_len = BOT_ERR_CLOSED;
            return NULL;
        }

        if (poll_ret == 1 && (pfd.revents & POLLIN)) {
            /* Ready for reading! Let's see */
            int read_len;

            if (ret == NULL) {
                read_len = read(pipe, buf, 3);
            } else {
                read_len = read(pipe, ret + ptr, len - ptr);
            }

            if (read_len == -1) {
                fprintf(stderr, "read() failed with errno %d\n", errno);
                if (ret) free(ret);
                *o_len = BOT_ERR_SYSCALL;
                return NULL;
            }

            if (ret == NULL) {
                if (read_len < 3) {
                    /* Invalid */
                    if (ret) free(ret);
                    *o_len = BOT_ERR_FMT;
                    return NULL;
                }
                /* Parse the length */
                len = ((unsigned int)buf[2] << 16) |
                    ((unsigned int)buf[1] << 8) |
                    (unsigned int)buf[0];
                ret = (char *)malloc(len + 1);
                if (len == 0) break;    /* Nothing to read */
            } else {
                /* Move buffer pointer */
                ptr += read_len;
            }
        } else {
            if (ret) free(ret);
            if (timeout <= 0) {
                *o_len = BOT_ERR_TIMEOUT;
            } else {
                fprintf(stderr, "poll() returns unexpected events %d\n", pfd.revents);
                *o_len = BOT_ERR_SYSCALL;
            }
            return NULL;
        }
    }
#endif

    *o_len = len;
    ret[len] = '\0';
    return ret;
}

const char *bot_strerr(int code)
{
    switch (code) {
        case BOT_ERR_NONE:    return "ok";
        case BOT_ERR_FMT:     return "incorrect message format";
        case BOT_ERR_SYSCALL: return "failure during system calls";
        case BOT_ERR_TOOLONG: return "message too long";
        case BOT_ERR_CLOSED:  return "pipe closed";
        case BOT_ERR_TIMEOUT: return "timeout";
        default:              return "unknown error";
    }
}

typedef struct _bot_player {
#ifdef _WIN32
    intptr_t pid;
#else
    pid_t pid;
#endif

    /* fd_send is the child's stdin, fd_recv is stdout
       Parent writes to fd_send and reads from fd_recv */
    int fd_send, fd_recv;
    int fd_log;
} bot_player;

#ifdef _WIN32
#define bot_judge_pause(__cp)
#define bot_judge_resume(__cp)
#else
#define bot_judge_pause(__cp)   kill(-(__cp).pid, SIGUSR1)
#define bot_judge_resume(__cp)  kill(-(__cp).pid, SIGUSR2)
#endif

/*
  Creates the child.
  Child processes are normally paused, but during `bot_judge_recv()`
  the process is resumed, and paused again after its response arrives.
 */
static bot_player bot_judge_create(const char *cmd, const char *log)
{
    bot_player ret;
    ret.pid = -1;

    int fd_send[2], fd_recv[2];
#ifdef _WIN32
    if (_pipe(fd_send, 1024, _O_BINARY) != 0 ||
        _pipe(fd_recv, 1024, _O_BINARY) != 0)
#else
    if (pipe(fd_send) != 0 || pipe(fd_recv) != 0)
#endif
    {
        fprintf(stderr, "pipe() failed with errno %d\n", errno);
        exit(1);    /* Non-zero exit status by the judge will be
                       reported as "System Error" */
    }

    int fd_log = open(log, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_log == -1) {
        fprintf(stderr, "open(%s) failed with errno %d\n", log, errno);
        exit(1);
    }

#ifdef _WIN32
    /* Back up stdin/stdout/stderr, since following _dup2() calls
       overwrite these */
    int fd_stdin = _dup(STDIN_FILENO);
    int fd_stdout = _dup(STDOUT_FILENO);
    int fd_stderr = _dup(STDERR_FILENO);

    _dup2(fd_send[0], STDIN_FILENO);
    _dup2(fd_recv[1], STDOUT_FILENO);
    _dup2(fd_log, STDERR_FILENO);

    /* Spawn the child process */
    intptr_t handle = _spawnl(_P_NOWAIT, cmd, cmd, NULL);

    ret.pid = handle;
    ret.fd_log = fd_log;

    /* Restore original stdin/stdout/stderr */
    _dup2(fd_stdin, STDIN_FILENO);
    _dup2(fd_stdout, STDOUT_FILENO);
    _dup2(fd_stderr, STDERR_FILENO);
    _close(fd_stdin);
    _close(fd_stdout);

#else
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork() failed with errno %d\n", errno);
        exit(1);
    }

    if (pid == 0) {
        /* Child process */
        dup2(fd_send[0], STDIN_FILENO);
        dup2(fd_recv[1], STDOUT_FILENO);
        dup2(fd_log, STDERR_FILENO);
        close(fd_send[1]);
        close(fd_recv[0]);
        if (execl(cmd, cmd, (char *)NULL) != 0) {
            if (execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL) != 0) {
                fprintf(stderr, "exec(%s) failed with errno %d\n", cmd, errno);
                exit(1);
            }
        }
    }

    ret.pid = pid;
#endif

    /* Parent process */
    close(fd_send[0]);
    close(fd_recv[1]);
    ret.fd_send = fd_send[1];
    ret.fd_recv = fd_recv[0];
    bot_judge_pause(ret);

    return ret;
}

int num_players;
static bot_player *players;

int bot_judge_init(int argc, char *const argv[])
{
    int n = (argc - 1) / 2;
    num_players = n;
    players = (bot_player *)malloc(sizeof(bot_player) * n);

    int i;
    for (i = 0; i < n; i++)
        players[i] = bot_judge_create(argv[1 + i], argv[1 + i + n]);

    return n;
}

void bot_judge_finish()
{
    int i;
    for (i = 0; i < num_players; i++) {
#ifdef _WIN32
        _close(players[i].fd_log);
        TerminateProcess((HANDLE)players[i].pid, 0);
#else
        fsync(players[i].fd_log);
        kill(players[i].pid, SIGTERM);
        while (waitpid(players[i].pid, 0, 0) > 0) { }
#endif
    }
}

void bot_judge_send(int id, const char *str)
{
    bot_send_blob(players[id].fd_send, 0, str);
}

char *bot_judge_recv(int id, int *o_len, int timeout)
{
    size_t len;
    bot_judge_resume(players[id]);
    char *resp = bot_recv_blob(players[id].fd_recv, &len, timeout);
    bot_judge_pause(players[id]);
    if (o_len != NULL) *o_len = len;
    return resp;
}

void bot_send(const char *s)
{
    bot_send_blob(STDOUT_FILENO, 0, s);
}

char *bot_recv()
{
    size_t len;
    char *ret = bot_recv_blob(STDIN_FILENO, &len, -1);
    return ret;
}