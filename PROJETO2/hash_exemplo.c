#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h> 

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

int main(void) {
    const char *texto = "TEXTO A SER GERADO O HASH";
    size_t comprimento = strlen(texto);

    EVP_MD_CTX *contexto;
    
    unsigned char hash_md5[16];  
    unsigned char hash_sha1[20]; 
    unsigned int tam;
    
    char *string_md5;
    char *string_sha1;

    contexto = EVP_MD_CTX_new();
    if (contexto == NULL) return 1;

    EVP_DigestInit_ex(contexto, EVP_md5(), NULL);
    EVP_DigestUpdate(contexto, texto, comprimento);
    EVP_DigestFinal_ex(contexto, hash_md5, &tam);

    EVP_DigestInit_ex(contexto, EVP_sha1(), NULL);
    EVP_DigestUpdate(contexto, texto, comprimento);
    EVP_DigestFinal_ex(contexto, hash_sha1, &tam);

    EVP_MD_CTX_free(contexto);

    /* Converte os bytes para string */
    string_md5 = hash_to_string(hash_md5, 16);
    string_sha1 = hash_to_string(hash_sha1, 20);

    printf("Texto Original: %s\n\n", texto);
    
    if (string_md5 != NULL) {
        printf("MD5  String: %s\n", string_md5);
        free(string_md5);
    }
    
    if (string_sha1 != NULL) {
        printf("SHA1 String: %s\n", string_sha1);
        free(string_sha1);
    }

    return 0;
}