#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "clipboardIntern.h"

// Nome do file descriptor a ser utilizado
#define SOCKET_ADDR "./CLIPBOARD_SOCKET"

/**
 * Connects to a fifo with the name declared as input
 * @param  clipboard_dir descrição do path do file descriptor do fifo
 * @return               valor do fifo para aceder ao clipboard, -1 em caso de erro
 */
int clipboard_connect(char * clipboard_dir);

/**
 * Copia a informação apontada por buf, com tamanho count para uma regiao do clipboard
 * @param  clipboard_id número do fifo devlvido pelo connect
 * @param  region       regiao onde se quer guardar a informacao
 * @param  buf          ponteiro para a data
 * @param  count        número de bytes da data
 * @return              número de bytes copiados em caso de sucesso, -1 em caso de erro
 */
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);

/**
 * Devolve a informação de uma região do clipboard, guarda na variavel apontada por buf, com tamanho count
 * @param  clipboard_id número do fifo devlvido pelo connect
 * @param  region       regiao onde se quer obter a informacao
 * @param  buf          ponteiro para a data
 * @param  count        número de bytes da data
 * @return              número de bytes obtidos do clipboard em caso de sucesso, -1 em caso de erro
 */
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);

/**
 * Espera por uma nova informação numa regiao do clipboard. 
 * Devolve a informação de uma região do clipboard, guarda na variavel apontada por buf, com tamanho count
 * @param  clipboard_id número do fifo devlvido pelo connect
 * @param  region       regiao onde se quer obter a informacao
 * @param  buf          ponteiro para a data
 * @param  count        número de bytes da data
 * @return              número de bytes obtidos do clipboard em caso de sucesso, -1 em caso de erro
 */
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);

/**
 * Fecha a conecção com o clipboard
 * @param clipboard_id desciptor do connect
 */
void clipboard_close(int clipboard_id);

#endif