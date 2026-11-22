#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

volatile sig_atomic_t was_sighup = 0;

static void sighup_handler(int signal_number)
{
    was_sighup = 1;
}

int main(void)
{
    int port = 8080;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }
    if (listen(server_fd, 16) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    sigset_t blocked_mask;
    sigset_t original_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blocked_mask, &original_mask) == -1) {
        perror("sigprocmask");
        close(server_fd);
        return EXIT_FAILURE;
    }

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sighup_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGHUP, &action, NULL) == -1) {
        perror("sigaction");
        close(server_fd);
        return EXIT_FAILURE;
    }

    int client_fd = -1;
    for (;;) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;
        if (client_fd != -1) {
            FD_SET(client_fd, &read_fds);
            if (client_fd > max_fd)
                max_fd = client_fd;
        }

        int ready = pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &original_mask);

        if (was_sighup) {
            printf("Получен сигнал SIGHUP\n");
            was_sighup = 0;
        }
        if (ready == -1) {
            if (errno == EINTR)
                continue;
            perror("pselect");
            break;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            int new_fd = accept(server_fd, NULL, NULL);
            if (new_fd == -1) {
                perror("accept");
            } else {
                printf("Новое подключение\n");
                if (client_fd != -1)
                    close(new_fd);
                else
                    client_fd = new_fd;
            }
        }

        if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
            char buffer[1024];
            ssize_t received = recv(client_fd, buffer, sizeof(buffer), 0);
            if (received > 0) {
                printf("Получено байтов: %zd\n", received);
            } else {
                close(client_fd);
                client_fd = -1;
            }
        }
    }

    if (client_fd != -1)
        close(client_fd);
    close(server_fd);
    sigprocmask(SIG_SETMASK, &original_mask, NULL);
    return EXIT_FAILURE;
}
