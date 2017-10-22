#include <stdio.h>
#include <stdlib.h>
#include "fila.h"

#define N 100

struct fila {
	int n; /* numero de elementos armazenados na fila */
	int ini; /* indice para o inicio da fila */
	InfoProcesso vet[N];
	int prio;
}

/* Essa funcao para criar a fila aloca dinamicamente essa estrutura e inicializa a fila como sendo vazia. */
Fila* fila_cria(int prioridade) {
	Fila* f = (Fila*) malloc(sizeof(Fila));
	f->n = 0; /* inicializa fila vazia */
	f->ini = 0; /* escolhe uma posicao inicial */
	f->prio = prioridade;
	return f;
}

/* Verifica se a fila esta vazia. */
int fila_vazia(Fila *f) {
	return (f->n == 0);
}

/* Para inserir um elemento na fila, usamos a prox posicao livre do vetor, indicada por fim. */
void fila_insere(Fila* f, InfoProcesso processo) {
	int fim;
	
	if (f->n == N) { /* fila cheia: capacidade esgotada */
		printf("Capacidade da fila estourou. \n")
		exit(1);
	}
	/* insere elemento na prox posicao livre */
	fim = (f->ini + f->n) % N;
	f->vet[fim] = processo;
	f->n++;
}

/* A funcao para retirar o elemento do inicio da fila fornece o valor do elemento retirado como retorno. */
InfoProcesso fila_retira(Fila* f) {
	InfoProcesso processo;

	if (fila_vazia(f)) {
		printf("Fila vazia. \n")
		exit(1);
	}
	/* retira elemento do inicio */
	processo = f->vet[f->ini];
	f->ini = (f->ini + 1) % N;
	f->n--;
	return processo;
}

/* Funcao para liberar a memoria alocada pela fila. */
void fila_libera(Fila* f) {
	free(f);
}
