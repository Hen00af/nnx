#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "nnx_internal.h"

static volatile sig_atomic_t nnx_stop_signal;

static void nnx_on_signal(int sig)
{
    nnx_stop_signal = sig;
}

static int nnx_write_config(const char *path, int port)
{
    FILE *fp = fopen(path, "w");
    if (!fp) return -1;

    fprintf(fp,
        "daemon off;\n"
        "master_process off;\n"
        "error_log stderr notice;\n"
        "pid logs/nginx.pid;\n"
        "events { worker_connections 1024; }\n"
        "http {\n"
        "  access_log off;\n"
        "  server {\n"
        "    listen 127.0.0.1:%d;\n"
        "    location / { nnx; }\n"
        "  }\n"
        "}\n", port);

    return fclose(fp) == 0 ? 0 : -1;
}

static int nnx_mkdir(const char *path)
{
    if (mkdir(path, 0700) == 0 || errno == EEXIST) return 0;
    return -1;
}

int nnx_run(nnx_app *app, int port)
{
    const char *nginx;
    char prefix[] = "/tmp/nnx-XXXXXX";
    char conf[512];
    char logs[512];
    pid_t pid;
    int status;

    if (!app || port <= 0 || port > 65535) return -1;

    nginx = getenv("NNX_NGINX_BIN");
    if (!nginx || !*nginx) nginx = "nginx";

    if (!mkdtemp(prefix)) return -1;
    snprintf(logs, sizeof(logs), "%s/logs", prefix);
    snprintf(conf, sizeof(conf), "%s/nginx.conf", prefix);
    if (nnx_mkdir(logs) != 0 || nnx_write_config(conf, port) != 0) return -1;

    signal(SIGINT, nnx_on_signal);
    signal(SIGTERM, nnx_on_signal);

    pid = fork();
    if (pid < 0) return -1;

    if (pid == 0) {
        execlp(nginx, nginx, "-p", prefix, "-c", conf, (char *)NULL);
        _exit(errno == ENOENT ? 127 : 126);
    }

    fprintf(stderr, "nnx: listening on http://127.0.0.1:%d via %s\n", port, nginx);

    for (;;) {
        pid_t rc = waitpid(pid, &status, nnx_stop_signal ? 0 : WNOHANG);
        if (rc == pid) break;
        if (rc < 0 && errno != EINTR) return -1;
        if (nnx_stop_signal) {
            kill(pid, nnx_stop_signal == SIGINT ? SIGINT : SIGTERM);
            while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
            break;
        }
        usleep(100000);
    }

    unlink(conf);
    rmdir(logs);
    rmdir(prefix);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
