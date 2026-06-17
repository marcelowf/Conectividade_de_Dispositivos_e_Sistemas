/*
   client.c
   Cliente do servico TCP de geracao de HASH (MD5 / SHA-1).

   Baseado nos exemplos do professor:
   - cliente.c     -> conexao TCP, prompt de usuario/senha e montagem do LOGIN
   - clienteTCP.c  -> loop livre lendo linhas com fgets e enviando ao servidor

   Compilar:  gcc client.c -o client
*/

#include <stdio.h>      /* printf */
#include <string.h>     /* strlen */
#include <sys/socket.h> /* socket */
#include <arpa/inet.h>  /* inet_addr */
#include <stdlib.h>
#include <unistd.h>

#define PORTA 9999

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char *message, *server_reply;
    char usr_cliente[32], pwd_cliente[32];

    message = malloc(2000);
    server_reply = malloc(2000);

    /* Criar socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        printf("Nao pude criar o socket!\n");
    }
    puts("Socket criado!\n");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORTA);

    /* Conectar ao servidor */
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("conexao falhou. Error!\n");
        return 1;
    }
    puts("Conectado ao servidor!\n");

    /* --- Etapa de login (estilo cliente.c) --- */
    printf("Digite o usuario: ");
    scanf("%31s", usr_cliente);
    printf("Digite a senha: ");
    scanf("%31s", pwd_cliente);
    getchar(); /* descarta o '\n' deixado pelo scanf antes dos fgets do loop */

    memset(message, '\0', 2000);
    sprintf(message, "LOGIN %s %s", usr_cliente, pwd_cliente);
    printf("Cliente enviou: %s\n", message);

    if (send(sock, message, strlen(message), 0) < 0)
    {
        puts("Send falhou!\n");
        return 1;
    }

    memset(server_reply, '\0', 2000);
    if (recv(sock, server_reply, 2000, 0) < 0)
    {
        puts("recv falhou!\n");
        return 1;
    }
    puts("Servidor respondeu: ");
    puts(server_reply);

    /* --- Loop livre (estilo clienteTCP.c) --- */
    puts("\nComandos disponiveis:");
    puts("  HASH 1 <texto>   -> calcula MD5 do texto");
    puts("  HASH 2 <texto>   -> calcula SHA-1 do texto");
    puts("  EXIT             -> encerra a conexao\n");

    while (1)
    {
        memset(message, '\0', 2000);

        printf("Entre com uma mensagem : ");

        if (fgets(message, 2000, stdin) == NULL)
            break;

        if ((strlen(message) > 0) && (message[strlen(message) - 1] == '\n'))
            message[strlen(message) - 1] = '\0';

        /* Enviando mensagem ao servidor */
        printf("Cliente enviou: %s\n", message);

        if (send(sock, message, strlen(message), 0) < 0)
        {
            puts("Send falhou!\n");
            return 1;
        }

        /* Recebendo retorno do servidor */
        memset(server_reply, '\0', 2000);
        if (recv(sock, server_reply, 2000, 0) < 0)
        {
            puts("recv falhou!\n");
            break;
        }

        puts("Servidor respondeu: ");
        puts(server_reply);

        /* Se o comando foi EXIT, encerra apos receber o "STATUS 200 Bye" */
        if (strcmp(message, "EXIT") == 0)
            break;
    }

    free(message);
    free(server_reply);

    close(sock);
    return 0;
}
