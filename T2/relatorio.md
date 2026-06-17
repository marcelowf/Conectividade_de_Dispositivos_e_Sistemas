# Relatório — Trabalho 2: Aplicação IoT Produtor-Consumidor com MQTT

**Disciplina:** Redes de Computadores
**Cenário:** SmartAgriculture — Sistema de monitoramento de variáveis ambientais
**Equipe:** _[preencher nomes]_
**Data:** 16/06/2026

---

## 1. Introdução e arquitetura

Foi implementada uma aplicação IoT simplificada seguindo o paradigma **produtor-consumidor**,
utilizando o protocolo de aplicação **MQTT** (Message Queuing Telemetry Transport) com o broker
open-source **Mosquitto** instalado em uma VM Linux.

O cenário simula um sistema de agricultura inteligente (*SmartAgriculture*), onde sensores
ambientais publicam leituras de **temperatura** e **umidade**, e um coletor central as consome.

Componentes:

- **Produtor (`sensor.py`):** publica valores aleatórios de temperatura (0–30) e umidade (0–60)
  a cada 10 segundos, nos tópicos `sensor/5000/temperatura` e `sensor/5000/umidade`.
  O payload é codificado em 2 bytes *big-endian* (`pack(">H", valor)`).
- **Broker (`Mosquitto`):** intermediário que recebe as publicações e as encaminha aos assinantes.
  Escuta na porta TCP **1883** (porta padrão do MQTT).
- **Consumidor (`coletor.py`):** assina o tópico `sensor/#` (todos os tópicos sob `sensor/`) e
  imprime cada leitura recebida.

Como o broker e as duas aplicações rodam na **mesma VM**, toda a comunicação ocorre pela interface
de **loopback** (`127.0.0.1`).

> _[INSERIR PRINT: diagrama de arquitetura — image.png (sensores → broker → coletor)]_

---

## 2. Ambiente: broker Mosquitto ativo

O broker foi iniciado com `sudo systemctl start mosquitto` e verificado com
`sudo systemctl status mosquitto`, apresentando estado **`active (running)`**.

> _[INSERIR PRINT: systemctl status mosquitto — active (running)]_

---

## 3. Operação dos módulos (produtor e consumidor)

### 3.1 Produtor — `sensor.py`

Executado com `python3 sensor.py`, publicando alternadamente temperatura e umidade a cada 10s:

```
sensor/5000/temperatura/10
sensor/5000/umidade/12
sensor/5000/temperatura/21
sensor/5000/umidade/57
...
```

> _[INSERIR PRINT: terminal do sensor publicando]_

### 3.2 Consumidor — `coletor.py`

Executado com `python3 coletor.py`, recebendo as mensagens publicadas no broker:

> _[INSERIR PRINT: terminal do coletor recebendo as mensagens]_

A correspondência entre os valores publicados pelo sensor e os recebidos pelo coletor comprova
o funcionamento do fluxo **produtor → broker → consumidor**.

---

## 4. Capturas Wireshark e estrutura de encapsulamento

A captura foi feita na interface **Loopback: lo** (pois todo o tráfego usa `127.0.0.1`),
com o filtro de exibição **`mqtt`** aplicado.

> _[INSERIR PRINT: lista de pacotes Wireshark filtrada por mqtt — mostrando Connect, Subscribe, Publish, etc.]_

### 4.1 Estrutura de encapsulamento de protocolos

Ao expandir um pacote **Publish Message** no Wireshark, observa-se o aninhamento das camadas,
do quadro de enlace até o dado de aplicação:

```
+--------------------------------------------------------------+
| Ethernet II / Linux cooked  (camada de enlace)               |
|  +--------------------------------------------------------+  |
|  | IPv4   (camada de rede)   Src/Dst: 127.0.0.1           |  |
|  |  +--------------------------------------------------+  |  |
|  |  | TCP  (camada de transporte)   Dst Port: 1883     |  |  |
|  |  |  +--------------------------------------------+  |  |  |
|  |  |  | MQTT (camada de aplicação)                 |  |  |  |
|  |  |  |   Publish Message                          |  |  |  |
|  |  |  |   Topic: sensor/5000/temperatura           |  |  |  |
|  |  |  |   Payload: 2 bytes (valor)                 |  |  |  |
|  |  |  +--------------------------------------------+  |  |  |
|  |  +--------------------------------------------------+  |  |
|  +--------------------------------------------------------+  |
+--------------------------------------------------------------+
```

Ou seja: a mensagem **MQTT** é encapsulada dentro de um segmento **TCP**, que por sua vez é
encapsulado em um datagrama **IP**, transportado dentro de um **quadro** da camada de enlace.

> _[INSERIR PRINT: pacote Publish expandido no Wireshark mostrando as camadas Ethernet → IP → TCP → MQTT]_

### 4.2 Justificativa de endereços IP, portas e protocolos

| Campo | Valor observado | Justificativa |
|-------|-----------------|---------------|
| **IP de origem** | `127.0.0.1` | Sensor e coletor rodam na mesma VM; usam o endereço de loopback. |
| **IP de destino** | `127.0.0.1` | O broker Mosquitto também está na mesma VM. |
| **Porta de destino** | `1883` | Porta padrão (*well-known*) do protocolo MQTT, onde o broker escuta. |
| **Porta de origem** | porta efêmera alta (ex.: `37371`, `33455`) | Atribuída dinamicamente pelo SO a cada cliente (sensor e coletor). |
| **Protocolo de transporte** | **TCP** | MQTT exige entrega confiável e ordenada; TCP é orientado a conexão. |
| **Protocolo de aplicação** | **MQTT v3.1** | Protocolo de mensageria publish/subscribe usado pela aplicação IoT. |

---

## 5. Diagrama de sequência do protocolo de aplicação (MQTT)

A captura evidencia a seguinte troca de mensagens MQTT:

```
 Sensor (produtor)        Broker (Mosquitto)        Coletor (consumidor)
        |                        |                          |
        |----- CONNECT --------->|                          |
        |<---- CONNACK ----------|                          |
        |                        |<------- CONNECT ---------|
        |                        |-------- CONNACK -------->|
        |                        |<--- SUBSCRIBE (sensor/#)-|
        |                        |---- SUBACK ------------->|
        |                        |                          |
        |- PUBLISH temperatura ->|                          |
        |                        |-- PUBLISH temperatura -->|
        |- PUBLISH umidade ----->|                          |
        |                        |-- PUBLISH umidade ------>|
        |          ...           |            ...           |
        |--- PINGREQ ----------->|                          |  (keep-alive)
        |<-- PINGRESP -----------|                          |
        v                        v                          v
```

- **CONNECT / CONNACK:** estabelecimento da sessão MQTT entre cada cliente e o broker.
- **SUBSCRIBE / SUBACK:** o coletor se inscreve no tópico `sensor/#`.
- **PUBLISH:** o sensor publica (QoS 0); o broker reencaminha ao coletor inscrito.
- **PINGREQ / PINGRESP:** mensagens de *keep-alive* para manter a conexão viva.

> _[OPCIONAL — INSERIR aqui o diagrama de sequência refeito na ferramenta diagrams.net, mais bonito]_

---

## 6. Sockets estabelecidos (`netstat -n`)

Com o sensor e o coletor em execução, o comando `netstat -tn | grep 1883` mostrou as conexões
TCP no estado **ESTABELECIDA** (ESTABLISHED):

```
tcp  0  0  127.0.0.1:1883    127.0.0.1:37371   ESTABLISHED
tcp  0  0  127.0.0.1:1883    127.0.0.1:33455   ESTABLISHED
tcp  0  0  127.0.0.1:33455   127.0.0.1:1883    ESTABLISHED
tcp  4  0  127.0.0.1:37371   127.0.0.1:1883    ESTABLISHED
```

> _[INSERIR PRINT: netstat -tn | grep 1883]_

### Explicação do socket observado

Um **socket** é identificado pelo par **(endereço IP : porta de origem, endereço IP : porta de destino)**,
que define unicamente uma conexão TCP. Na captura:

- O broker está sempre na porta **1883** (`127.0.0.1:1883`).
- Existem **dois clientes** conectados, com portas efêmeras distintas: **37371** e **33455** —
  correspondendo ao sensor e ao coletor.
- Como a comunicação é via **loopback**, cada conexão aparece nos **dois sentidos**
  (ex.: `1883 → 37371` e `37371 → 1883`), totalizando 4 linhas para 2 conexões.

Cada par `cliente:porta_efêmera ↔ broker:1883` representa uma conexão TCP independente e
ativa entre um cliente MQTT e o broker.

---

## 7. Conclusão

A aplicação demonstrou na prática o paradigma produtor-consumidor típico de IoT:
o sensor (produtor) publica leituras ambientais, o broker Mosquitto as intermedia, e o
coletor (consumidor) as recebe via assinatura de tópicos. A análise das capturas confirmou
o uso do protocolo de aplicação **MQTT** sobre **TCP/IP**, na porta padrão **1883**, com o
encapsulamento de protocolos e o estabelecimento de sockets corretamente observados.

---

### Anexos / código

- `sensor.py` — módulo produtor
- `coletor.py` — módulo consumidor
- `captura.pcapng` — captura completa do Wireshark
