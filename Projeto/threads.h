#ifndef THREADS_H
#define THREADS_H

#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Número de posições do clipboard
#define NUMBEROFPOSITIONS 10

// Constantes de funcionamento do programa
#define WAIT 2
#define PASTE 1
#define COPY 0

#define LOCAL 0
#define ONLINE 1

#define CLIPBOARD 1
#define APP 0

/**
 * Estrutura de cada posição do clipboard
 * data - ponteiro para a informação
 * size - tamanho da mensagem apontado pelo data
 */
typedef struct _clipboard {
	char *data;
	size_t size;
} clipboard_struct;

/**
 * Estrutura do protocolo de comunicação
 * size    - tamanho da data a enviar
 * region  - região que se quer alterar
 * action  - acao pretendida na comunicação
 */
typedef struct _message {
	size_t size;
	int region;
	int action;
} Message_struct;

/**
 * Estrutura com as informações de cada thread
 * thread_id      - id da thread
 * inputArmgument - sock_fd do cliente
 * type           - tipo de thread (app ou outro clipboard)
 */
typedef struct _thread_info_struct {
	pthread_t thread_id; 
	int inputArgument;
	int type;
	struct _thread_info_struct *next;
} thread_info_struct;


/**
 * Inicializa os mutex do sistema
 * @return em caso de sucesso
 */
int initMutex();

/**
 * Inicializa os rwLocks do sistema
 * @return em caso de sucesso
 */
int initRWLock();

/**
 * Inicializa os conditional waits do sistema
 * @return em caso de sucesso
 */
int initCondWait();


/**
 * Adiciona uma estrutura de thread à lista de Threads
 * @param new estrutura da nova thread
 */
void clipboardThreadListAdd(thread_info_struct *new);


/**
 * Procura uma thread na lista de threads e remove-a da lista
 * @param thread_id número da thread
 */
void clipboardThreadListRemove(pthread_t thread_id);


/**
 * @brief      Escreve dados para um socket ou pipe, em caso de não ter conseguido 
 * enviar tudo, tenta novamente até enviar tudo
 *
 * @param[in]  sock_fd  O file descriptor do socket
 * @param      buf      Buffer para guardar informação
 * @param[in]  len      A quuatidade de informação que se pretende escrever
 *
 * @return     Retorna o núumero de bytes lidos ou -1 em caso de perda de conecção
 */
int writeAll(int sock_fd, char *buf, int len);


/**
 * @brief      Le dados para um socket ou pipe, em caso de não ter conseguido 
 * enviar tudo, tenta novamente até ler tudo
 *
 * @param[in]  sock_fd  o file descriptor do cliente
 * @param      buf      Buffer para guardar informação
 * @param[in]  len      Número de bytes que se prentende ler
 *
 * @return     Retorna o núumero de bytes lidos ou -1 em caso de perda de conecção
 */
int readAll(int sock_fd, char *buf, int len);


/**
 * Realiza um copy - recebe a mensagem de um cliente e envia para esta ser propagada
 * @param  messageReceived messagem com os dados sobre a nova informacao
 * @param  client          file descriptor do cliente
 * @param  type            refere a origem da mensagem - APP ou CLIPBOARD
 * @return                 1 sucesso, -1 em caso de perda de conecção com o cliente
 */
int copy(Message_struct messageReceived, int client, int type);

/**
 * Realiza um paste - recebe a mensagem de um cliente com uma regiao pretendida e envia-a
 * @param  messageReceived messagem com os dados sobre a nova informacao
 * @param  client          file descriptor do cliente
 * @param  type            refere a origem da mensagem - APP ou CLIPBOARD
 * @return                 1 sucesso, -1 em caso de perda de conecção
 */
int paste(Message_struct messageReceived, int client, int type);

/**
 * Espera por uma nova infrmação aparecer numa região do clipboard e envia para o cliente
 * @param  messageReceived mensagem com os parametros da acao pretendida
 * @param  client          file descriptor do cliente
 * @return                 1 em caso de sucesso, -1 em caso de perda de conecção
 */
int wait(Message_struct messageReceived, int client);

/**
 * Realiza pastes da informação contida no clipboard de modo a atualizar a inforamção do debaixo
 * @param  client file descriptor do cliente
 * @return        retorna 1 em caso de sucesso
 */
int backup(int client);

/**
 * Le a informação - mensagem mais data - de um clipboard de cima
 * @param  message messagem com os dados sobre a nova informacao
 * @param  data    vetor com a inforamcao
 * @return         -1 erro; 0 sucesso
 */
int readUp(int client, void * data, size_t size);

/**
 * Escreve a informação para o clipboard acima deste, ou para o pipe quando em modo local
 * @param  message messagem com os dados sobre a nova informacao
 * @param  data    vetor com a inforamcao
 * @return         -1 erro; 0 sucesso
 */
int writeUp(Message_struct message, char * data);

/**********************
 * Funções das Threads
 **********************/

/**
 * Thread responsavel por receber os pedidos dos clientes e fazer as ações por este pretendidas
 * @param  arg estrutura com as informacoes sobre a thread
 */
void * clientThread(void * arg);

/**
 * Thread responsavel por receber as atualizações vindas de cima da arvore e as propagar para baixo
 * 			em modo ONLINE - le do socket 		em modo LOCAL - le do pipe
 * @param  arg estrutura com as informacoes sobre a thread
 * @return     [description]
 */
void * upThread(void * arg);

/**
 * Thread responsavel por aceitar conecçoes debaixo e atualizar estas com a informação que existe no clipboard
 * @param  arg estrutura com as informacoes sobre a thread
 */
void * downThread(void * arg);


#endif