#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

static char SERVER_PROGRAM_NAME[] = "posix_ipc-server";

int my_strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

void my_concat_path(char *dest, const char *path, const char *name) {
    int i = 0;
    while (path[i] != '\0') {
        dest[i] = path[i];
        i++;
    }
    dest[i++] = '/';
    int j = 0;
    while (name[j] != '\0') {
        dest[i + j] = name[j];
        j++;
    }
    dest[i + j] = '\0';
}

int main(int argc, char **argv) {
    if (argc != 2) {
        const char msg1[] = "usage: ";
        const char msg2[] = " <output_filename>\n";
        write(STDERR_FILENO, msg1, my_strlen(msg1));
        write(STDERR_FILENO, argv[0], my_strlen(argv[0]));
        write(STDERR_FILENO, msg2, my_strlen(msg2));
        exit(EXIT_FAILURE);
    }

    char progpath[1024];
    ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
    if (len == -1) {
        const char msg[] = "error: failed to read full program path\n";
        write(STDERR_FILENO, msg, my_strlen(msg));
        exit(EXIT_FAILURE);
    }
    while (len > 0 && progpath[len] != '/') {
        --len;
    }
    progpath[len] = '\0';

    int client_to_server[2];
    int server_to_client[2];

    if (pipe(client_to_server) == -1 || pipe(server_to_client) == -1) {
        const char msg[] = "error: failed to create pipes\n";
        write(STDERR_FILENO, msg, my_strlen(msg));
        exit(EXIT_FAILURE);
    }

    const pid_t child = fork();

    if (child == -1) {
        const char msg[] = "error: failed to spawn new process\n";
        write(STDERR_FILENO, msg, my_strlen(msg));
        exit(EXIT_FAILURE);
    }

    if (child == 0) {
        dup2(client_to_server[0], STDIN_FILENO);
        close(client_to_server[0]);
        close(client_to_server[1]);

        dup2(server_to_client[1], STDOUT_FILENO);
        close(server_to_client[0]);
        close(server_to_client[1]);

        char path[2048];
        my_concat_path(path, progpath, SERVER_PROGRAM_NAME);

        char *const args[] = {SERVER_PROGRAM_NAME, argv[1], NULL};
        execv(path, args);

        const char msg[] = "error: failed to exec into server\n";
        write(STDERR_FILENO, msg, my_strlen(msg));
        exit(EXIT_FAILURE);
    }

    close(client_to_server[0]);
    close(server_to_client[1]);

    char buf[1024];
    ssize_t bytes;

    const char welcome[] = "[Parent] Enter floats (e.g. 20.5 2.0 5.0) or press Enter on empty line:\n";
    write(STDOUT_FILENO, welcome, my_strlen(welcome));

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0) {
        if (buf[0] == '\n') {
            break;
        }

        write(client_to_server[1], buf, bytes);

        char resp[64];
        ssize_t resp_bytes = read(server_to_client[0], resp, sizeof(resp) - 1);
        
        if (resp_bytes <= 0) {
            const char msg[] = "[Parent] Child closed connection unexpectedly.\n";
            write(STDOUT_FILENO, msg, my_strlen(msg));
            break;
        }
        resp[resp_bytes] = '\0';

        if (resp[0] == 'Z' && resp[1] == 'E' && resp[2] == 'R' && resp[3] == 'O') {
            const char msg[] = "[Parent] Division by zero detected in child. Parent is exiting now!\n";
            write(STDOUT_FILENO, msg, my_strlen(msg));
            break;
        }

        const char status_msg[] = "[Parent] Server processed successfully. Status from child: ";
        write(STDOUT_FILENO, status_msg, my_strlen(status_msg));
        write(STDOUT_FILENO, resp, resp_bytes);
    }

    close(client_to_server[1]);
    close(server_to_client[0]);

    int status;
    waitpid(child, &status, 0);
    
    const char final_msg[] = "[Parent] Child process exited. Program finished.\n";
    write(STDOUT_FILENO, final_msg, my_strlen(final_msg));

    return 0;
}