#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "nnx_internal.h"

/*
 * Runtime v0: Nginx remains an external executable.
 *
 * nnx_run() owns the child-process lifecycle but does not vendor or embed
 * Nginx. The generated config/runtime registration will be expanded next.
 */
int nnx_run(nnx_app *app, int port)
{
    const char *nginx;
    pid_t pid;
    int status;

    if (!app || port <= 0 || port > 65535)
        return -1;

    nginx = getenv("NNX_NGINX_BIN");
    if (!nginx || !*nginx)
        nginx = "nginx";

    /*
     * The app pointer cannot cross exec(). A production runtime therefore
     * needs a generated module/config registration boundary. For now fail
     * explicitly rather than pretending this is already solved.
     */
    fprintf(stderr,
        "nnx: runtime prepared for external Nginx '%s' on port %d\n"
        "nnx: app registration across exec is the next runtime milestone\n",
        nginx, port);

    pid = fork();
    if (pid < 0)
        return -1;

    if (pid == 0) {
        execlp(nginx, nginx, "-v", (char *)NULL);
        _exit(errno == ENOENT ? 127 : 126);
    }

    if (waitpid(pid, &status, 0) < 0)
        return -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return -1;

    return 0;
}
