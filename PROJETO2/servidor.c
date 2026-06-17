#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORTA 9999

int main(void) {
    int fd_servidor, fd_cliente;
    struct sockaddr_in end_servidor;
    char buffer_in[128], buffer_out[32];
    char cmd[16], usr[32], pwd[32];

    fd_servidor = socket(AF_INET, SOCK_STREAM, 0);
    
    end_servidor.sin_family = AF_INET;
    end_servidor.sin_addr.s_addr = INADDR_ANY;
    end_servidor.sin_port = htons(PORTA);

    bind(fd_servidor, (struct sockaddr *)&end_servidor, sizeof(end_servidor));

    listen(fd_servidor, 5);

    printf("Servidor TCP rodando na porta %d...\n", PORTA);

    while (1) {
        fd_cliente = accept(fd_servidor, NULL, NULL);

        memset(buffer_in, 0, sizeof(buffer_in));

        recv(fd_cliente, buffer_in, sizeof(buffer_in) - 1, 0);

        if (sscanf(buffer_in, "%15s %31s %31s", cmd, usr, pwd) == 3) {

            if (strcmp(cmd, "LOGIN") == 0 && strcmp(usr, "admin") == 0 && strcmp(pwd, "1234") == 0) {
                strcpy(buffer_out, "STATUS 200");
            } else {
                strcpy(buffer_out, "STATUS 401");
            }
        } else {
            strcpy(buffer_out, "STATUS 400");
        }

        send(fd_cliente, buffer_out, strlen(buffer_out), 0);
        close(fd_cliente);
    }

    close(fd_servidor);
    return 0;
}