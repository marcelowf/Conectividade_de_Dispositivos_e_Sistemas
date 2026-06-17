#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORTA 9999

int main(void) {
    int fd_soquete;
    struct sockaddr_in end_servidor;
    char usr_cliente[32], pwd_cliente[32];
    char txt_envio[128], txt_resposta[32];

    printf("Digite o usuário: ");
    scanf("%31s", usr_cliente);
    printf("Digite a senha: ");
    scanf("%31s", pwd_cliente);

    fd_soquete = socket(AF_INET, SOCK_STREAM, 0);

    end_servidor.sin_family = AF_INET;
    end_servidor.sin_port = htons(PORTA);
    inet_pton(AF_INET, "127.0.0.1", &end_servidor.sin_addr);

    if (connect(fd_soquete, (struct sockaddr *)&end_servidor, sizeof(end_servidor)) < 0) {
        printf("Erro ao conectar no servidor.\n");
        return 1;
    }

    sprintf(txt_envio, "LOGIN %s %s", usr_cliente, pwd_cliente);
    send(fd_soquete, txt_envio, strlen(txt_envio), 0);

    memset(txt_resposta, 0, sizeof(txt_resposta));

    recv(fd_soquete, txt_resposta, sizeof(txt_resposta) - 1, 0);

    printf("\nResposta do Servidor: %s\n", txt_resposta);

    if (strcmp(txt_resposta, "STATUS 200") == 0) {
        printf("Resultado: OK (Acesso concedido)\n");
    } else if (strcmp(txt_resposta, "STATUS 401") == 0) {
        printf("Resultado: Acesso negado\n");
    } else {
        printf("Resultado: Erro na requisição\n");
    }

    close(fd_soquete);
    return 0;
}