/*
   server.c
   Servico TCP de geracao de HASH (MD5 / SHA-1).

   Baseado nos exemplos do professor:
   - servidor.c      -> estrutura do socket TCP e parsing com sscanf / STATUS
   - servidorTCP.c   -> loop while(recv > 0) mantendo a conexao aberta
   - hash_exemplo.c  -> calculo de MD5/SHA-1 com OpenSSL (EVP) e hash_to_string

   Compilar:  gcc server.c -o server -lcrypto
*/

#include <stdio.h>
#include <string.h>      /* strlen, strcmp, memset */
#include <stdlib.h>      /* malloc, free */
#include <sys/socket.h>
#include <arpa/inet.h>   /* inet_addr */
#include <unistd.h>      /* write, close */
#include <openssl/evp.h> /* EVP_* (MD5 / SHA-1) */

#define PORTA 9999

/* --- vindo do hash_exemplo.c (sem alteracoes) --- */
char* hash_to_string(const unsigned char *hash, int length) {
    int i;
    char *output = (char *)malloc((length * 2) + 1);
    if (output == NULL) return NULL;

    for (i = 0; i < length; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[length * 2] = '\0';
    return output;
}

/*
   Calcula o hash do texto recebido.
   algoritmo == 1 -> MD5   (16 bytes)
   algoritmo == 2 -> SHA-1 (20 bytes)
   Escreve o resultado em hexadecimal em 'saida'. Retorna 1 se ok, 0 se algoritmo invalido.
   Reaproveita as mesmas chamadas EVP do hash_exemplo.c.
*/
int calcular_hash(const char *texto, int algoritmo, char *saida) {
    EVP_MD_CTX *contexto;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int tam;
    char *hex;
    const EVP_MD *tipo;

    if (algoritmo == 1)
        tipo = EVP_md5();
    else if (algoritmo == 2)
        tipo = EVP_sha1();
    else
        return 0;

    contexto = EVP_MD_CTX_new();
    if (contexto == NULL) return 0;

    EVP_DigestInit_ex(contexto, tipo, NULL);
    EVP_DigestUpdate(contexto, texto, strlen(texto));
    EVP_DigestFinal_ex(contexto, hash, &tam);

    EVP_MD_CTX_free(contexto);

    hex = hash_to_string(hash, tam);
    if (hex == NULL) return 0;

    strcpy(saida, hex);
    free(hex);
    return 1;
}

int main(int argc, char *argv[])
{
    int socket_desc, client_sock, c, read_size;
    struct sockaddr_in server, client;
    char client_message[2000];

    /* Criar socket */
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("Nao foi possivel criar o socket!\n");
    }
    puts("Socket criado!\n");

    /* Preparar a estrutura sockaddr_in */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORTA);

    /* Bind */
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind falhou. Erro!\n");
        return 1;
    }
    puts("bind OK!\n");

    /* Listen */
    listen(socket_desc, 3);

    /* Esperando cliente..... */
    printf("Esperando conexoes de clientes na porta %d...\n\n", PORTA);

    c = sizeof(struct sockaddr_in);
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

    if (client_sock < 0)
    {
        perror("accept falhou!\n");
        return 1;
    }

    puts("Conexao de cliente aceita!\n");

    {
        int autenticado = 0;            /* estado da sessao (por conexao) */
        char cmd[16];
        char usr[32], pwd[32];
        char resposta[2200];
        char hash_hex[(EVP_MAX_MD_SIZE * 2) + 1];

        /* Recebe mensagens do cliente ate EXIT/desconexao (padrao do servidorTCP.c) */
        while ((read_size = recv(client_sock, client_message, 2000, 0)) > 0)
        {
            printf("Cliente mandou: %s\n", client_message);
            fflush(stdout);

            memset(cmd, '\0', sizeof(cmd));
            sscanf(client_message, "%15s", cmd);

            if (strcmp(cmd, "LOGIN") == 0)
            {
                /* LOGIN <user> <senha> */
                if (sscanf(client_message, "%*s %31s %31s", usr, pwd) == 2 &&
                    strcmp(usr, "admin") == 0 && strcmp(pwd, "12345") == 0)
                {
                    autenticado = 1;
                    strcpy(resposta, "STATUS 200 Autenticado");
                }
                else
                {
                    autenticado = 0;
                    strcpy(resposta, "STATUS 401 Acesso negado");
                }
            }
            else if (strcmp(cmd, "HASH") == 0)
            {
                int algoritmo = 0;
                char *payload;

                if (!autenticado)
                {
                    strcpy(resposta, "STATUS 401 Nao autenticado");
                }
                /* HASH <1|2> <texto...> : le o numero e aponta para o restante */
                else if (sscanf(client_message, "%*s %d", &algoritmo) == 1 &&
                         (algoritmo == 1 || algoritmo == 2))
                {
                    /* pula "HASH", o numero e os espacos para achar o texto */
                    payload = client_message;
                    payload += strlen("HASH");
                    while (*payload == ' ') payload++;   /* espacos antes do numero */
                    while (*payload && *payload != ' ') payload++; /* o numero */
                    while (*payload == ' ') payload++;   /* espacos antes do texto */

                    if (*payload == '\0')
                    {
                        strcpy(resposta, "STATUS 400");
                    }
                    else if (calcular_hash(payload, algoritmo, hash_hex))
                    {
                        sprintf(resposta, "STATUS 200 %s", hash_hex);
                    }
                    else
                    {
                        strcpy(resposta, "STATUS 400");
                    }
                }
                else
                {
                    strcpy(resposta, "STATUS 400");
                }
            }
            else if (strcmp(cmd, "EXIT") == 0)
            {
                strcpy(resposta, "STATUS 200 Bye");
                send(client_sock, resposta, strlen(resposta), 0);
                puts("Cliente pediu EXIT. Encerrando conexao.\n");
                break;
            }
            else
            {
                strcpy(resposta, "STATUS 400");
            }

            /* Envia a resposta ao cliente */
            send(client_sock, resposta, strlen(resposta), 0);

            memset(client_message, '\0', sizeof(client_message));
        }
    }

    if (read_size == 0)
    {
        puts("Cliente desconectado!\n");
        fflush(stdout);
    }
    else if (read_size == -1)
    {
        perror("recv falhou!\n");
    }

    close(client_sock);
    close(socket_desc);
    return 0;
}
