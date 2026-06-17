# Relatório — Serviço de Geração de HASH (TCP)

Disciplina: Conectividade de Dispositivos e Sistemas
Equipe: (preencher nomes)
Data: (preencher)

================================================================

## 1. Justificativa do protocolo de transporte utilizado (TCP)

Optamos pelo TCP (Transmission Control Protocol) em vez do UDP pelos seguintes motivos:

- Entrega confiável e ordenada: o serviço de hash exige que o texto enviado pelo cliente chegue ao servidor íntegro e completo. Qualquer byte perdido ou fora de ordem alteraria o hash calculado, produzindo um resultado errado. O TCP garante retransmissão de pacotes perdidos e reordenação, o que o UDP não oferece.

- Orientado à conexão: a aplicação é uma sessão com estado (LOGIN → autenticação → vários HASH → EXIT). O TCP estabelece uma conexão (handshake de 3 vias) e a mantém aberta, o que combina naturalmente com esse fluxo de requisição/resposta.

- Controle de fluxo e congestionamento: mesmo que o payload seja grande, o TCP segmenta e controla o envio sem que a aplicação precise tratar isso.

- Modelo cliente-servidor simples: as primitivas socket/bind/listen/accept (servidor) e socket/connect (cliente) atendem diretamente ao caso de uso.

O UDP seria adequado para tráfego tolerante a perdas e sensível a latência (ex.: streaming, DNS). Não é o caso aqui, onde a integridade do dado é requisito absoluto.

================================================================

## 2. Protocolo de aplicação — sintaxe e semântica

O protocolo é textual, baseado em linhas (ASCII). Cada mensagem é enviada em uma operação de envio (send).

Mensagens do Cliente para o Servidor:

- LOGIN <user> <senha>   → autentica o cliente. Exemplo: "LOGIN admin 12345".
- HASH <algoritmo> <texto>   → solicita o hash do texto. O algoritmo 1 = MD5 e o algoritmo 2 = SHA-1. O texto pode conter espaços. Exemplo: "HASH 1 dados confidenciais".
- EXIT   → encerra a sessão.

Mensagens do Servidor para o Cliente (formato: STATUS <código> <informação>):

- STATUS 200 Autenticado   → login com usuário/senha corretos.
- STATUS 401 Acesso negado   → login com credenciais inválidas.
- STATUS 200 <hash_hex>   → hash calculado com sucesso (cliente autenticado).
- STATUS 401 Nao autenticado   → pedido de HASH sem login válido.
- STATUS 200 Bye   → resposta ao comando EXIT.
- STATUS 400   → mensagem malformada ou comando desconhecido.

Códigos de status: 200 = sucesso; 401 = não autorizado; 400 = requisição inválida.
Credenciais fixas (para o trabalho): usuário "admin", senha "12345".

Exemplo de troca de mensagens de uma sessão completa:

  C → S : LOGIN admin 12345
  S → C : STATUS 200 Autenticado
  C → S : HASH 1 teste
  S → C : STATUS 200 698dc19d489c4e4db73e28a713eab07b        (MD5)
  C → S : HASH 2 teste
  S → C : STATUS 200 2e6f9b0d5885b6010f9167787445617f553a735f  (SHA-1)
  C → S : EXIT
  S → C : STATUS 200 Bye

(Opcional) Figura 1 — Diagrama de sequência da troca de mensagens (gerado no
websequencediagrams.com). [INSERIR IMAGEM]

================================================================

## 3. Captura Wireshark e descrição do encapsulamento

A captura foi feita na interface de loopback (lo), com o filtro de exibição "tcp.port == 9999", durante uma sessão completa entre o cliente e o servidor.

Figura 2 — Lista de pacotes capturados no Wireshark. [INSERIR PRINT]

Endereços e portas observados:

- IP de origem e de destino: 127.0.0.1 (loopback) — cliente e servidor rodam na mesma VM.
- Porta do servidor: 9999 (fixa, definida na aplicação).
- Porta do cliente: efêmera, atribuída pelo sistema operacional (na captura, 44316).
- Interface de captura: lo (loopback), pois o tráfego não sai da máquina.

Correspondência entre pacotes e mensagens (observada na captura):

- Pacotes 1 a 3: SYN, SYN-ACK e ACK — abertura da conexão (handshake de 3 vias).
- Pacote 4 (PSH-ACK, Len=17): LOGIN admin 12345 (17 bytes).
- Pacote 6 (PSH-ACK, Len=22): STATUS 200 Autenticado.
- Pacote 8 (PSH-ACK, Len=12): HASH 1 teste.
- Pacote 9 (PSH-ACK, Len=43): STATUS 200 + 32 dígitos hexadecimais (MD5).
- Pacote 12 (PSH-ACK, Len=51): STATUS 200 + 40 dígitos hexadecimais (SHA-1).
- Pacote 14 (PSH-ACK, Len=26): HASH 1 dados confidenciais.
- Pacote 17 (PSH-ACK, Len=4): EXIT.
- Pacote 18 (PSH-ACK, Len=14): STATUS 200 Bye.
- Pacotes 20 a 22: FIN, FIN-ACK e ACK — encerramento da conexão.

Observação importante: o campo Length (Len) de cada segmento corresponde exatamente ao número de bytes da mensagem da aplicação, comprovando que os dados trafegam diretamente no payload do segmento TCP.

Descrição do encapsulamento:

Ao expandir um dos pacotes de dados no Wireshark (por exemplo, o pacote do "LOGIN admin 12345"), observa-se a mensagem da aplicação encapsulada camada a camada, de fora para dentro, conforme o modelo TCP/IP:

1. Frame (quadro): metadados da captura — número do quadro, tamanho total em bytes e interface (lo).
2. Camada de Enlace (Ethernet II / loopback): cabeçalho de enlace. Na interface de loopback os endereços MAC aparecem nulos (00:00:00:00:00:00), pois o tráfego não passa por uma placa de rede física.
3. Camada de Rede (Internet Protocol Version 4 — IPv4): endereços de origem e destino, ambos 127.0.0.1.
4. Camada de Transporte (Transmission Control Protocol — TCP): porta de origem (efêmera) e porta de destino (9999), número de sequência (Seq), número de confirmação (Ack), flags (SYN, ACK, PSH, FIN) e janela (Window).
5. Camada de Aplicação (Data): o payload com a mensagem do nosso protocolo, por exemplo "LOGIN admin 12345".

Figura 3 — Pacote expandido no Wireshark mostrando as camadas de encapsulamento (Frame, Ethernet/Loopback, IP, TCP e Data). [INSERIR PRINT]

================================================================

## 4. Arquitetura da solução

Figura 4 — Diagrama da arquitetura da solução. [INSERIR IMAGEM: arquitetura.svg exportada como PNG]

- Entidades: processo cliente e processo servidor (ambos na mesma máquina virtual).
- Arquitetura de rede: modelo cliente-servidor sobre TCP/IP, com comunicação via interface de loopback (127.0.0.1).
- Protocolos por camada:
  - Aplicação: protocolo próprio (mensagens LOGIN, HASH e STATUS).
  - Transporte: TCP, porta 9999 no servidor e porta efêmera no cliente.
  - Rede: IPv4 (127.0.0.1).
  - Enlace/Física: interface de loopback (lo).
- Biblioteca de hash: OpenSSL (libcrypto), API EVP — funções EVP_md5() e EVP_sha1().

Fluxo de conectividade: o cliente cria o socket e executa connect() para 127.0.0.1:9999; o servidor, após bind() na porta 9999, listen() e accept(), atende a conexão. A troca de dados ocorre com send() e recv() em ambos os lados, seguindo o protocolo de aplicação descrito na seção 2.

================================================================

## 5. Saída do netstat

Figura 5 — Saída do comando "netstat -tnp | grep 9999" com a sessão ativa. [INSERIR PRINT]

Com o cliente conectado (sessão ativa, antes do EXIT), o netstat mostra os dois lados da conexão pela loopback, pois cliente e servidor estão na mesma VM:

  tcp  127.0.0.1:38850  127.0.0.1:9999   ESTABELECIDA  6637/./client
  tcp  127.0.0.1:9999   127.0.0.1:38850  ESTABELECIDA  6636/./server

- A linha do servidor usa a porta fixa 9999; a linha do cliente usa a porta efêmera sorteada pelo sistema operacional (no exemplo, 38850).
- Ambas em estado ESTABLISHED (ESTABELECIDA) confirmam que a conexão TCP está ativa.
- Os campos PID/Program confirmam que os processos são o ./server e o ./client.

================================================================

## 6. Como compilar e executar

Dependências:
  sudo apt update -y
  sudo apt install gcc build-essential libssl-dev -y

Compilação:
  gcc server.c -o server -lcrypto      (o servidor precisa da libcrypto / OpenSSL)
  gcc client.c -o client

Execução (em dois terminais):
  ./server      (terminal 1)
  ./client      (terminal 2 — login: admin / senha: 12345)
