
/****************************************************************************************************
 *
 *	TRABALHO 1 - Sistemas de Computação
 *	Professor: Luiz Fernando Seibel
 *	25/10/2017
 *
 *	Aluno:	Gabriel Barbosa de Oliveira - 1410578
 *	Aluno:	Karina Fernandes Tronkos - 1511690
 *
 *	O programa lê, na ordem dada, as seguintes informações de um arquivo "entrada.txt" :
 *
 *		1. A palavra exec indicando mais um programa a ser executado ;
 *
 *		2. O nome do programa a ser executado, o nome do programa é uma string sempre de nome
 *       programaN, onde n é um numero inteiro de 1 a 7
 *
 *	Apos ler e armazenar as informações, o programa chama executa os processos na ordem cor-
 *	reta, conforme especificado no enunciado do trabalho.
 *
 ****************************************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/shm.h>

#include "fila.h"

#define MAX_PROG 5 /* num max de programas */
#define MAX_TAM 10
#define MAX_PRIO 15
#define MAX_NOME 20

/***************************************************************************************************/

//Estruturas de dados auxiliares.
struct infoProcesso{
    char nome[MAX_NOME]; /* Nome do programa referente ao processo */
    int pid;       /* PID do processo */
    int prio[MAX_PRIO];         /* Prioridade do processo */
    int status;    /* 0 caso rolando, 1 caso terminado */
};

/***************************************************************************************************/

int interpretador (FILE * fp1, InfoProcesso vetorProcesso[], int * numProgramas, int * statusInterp);
int criaProcesso (int * pidProcesso, int posicao, char * nomeProcesso);
int escalonador (FILE * fp2, InfoProcesso vetorProcesso[], int * numProgramas, int * statusInterp);
int criaPrograma (int n);

/***************************************************************************************************/

int main (void) {
    InfoProcesso * vetorProcesso;
    int * numProgramas;
    int * statusInterp;
    int pidInterpretador;
    int retorno;
    int i;

    int segmentoInfo, segmentoNum, segmentoTerminado;

    FILE * fp1 = fopen ("entrada.txt", "r") ; /* Abertura de arquivo de entrada */
    FILE * fp2 = fopen ("saida.txt", "w") ;   /* Abertura de arquivo de saida  */


    *numProgramas = 0;
    *statusInterp = 0;

    for (i = 0; i < MAX_PROG; i++){
      vetorProcesso[i].status = 1;
      vetorProcesso[i].prio = 1000;
    }

    if (fp1 == NULL || fp2 == NULL) {  /* Teste de validação de abertura */
      fprintf (fp2, "Erro na abertura de arquivo. \n") ;
      return 1 ;
    }

    pidInterpretador = fork();

    if(pidInterpretador == 0) { // Filho - Interpretador
      retorno = interpretador (fp1, vetorProcesso, numProgramas, statusInterp) ;

      if (retorno != 0) {
        fprintf (fp2, "Erro no interpretador. \n") ;
        return 1 ;
      }

    } else { // Pai - Escalonador
      while(*numProgramas == 0); //Espera ler ao menos um programa para iniciar escalonamento

      escalonador(fp2, vetorProcesso, numProgramas, statusInterp) ; //Chama funcao do escalonador
      printf("Escalonamento chegou ao fim! \n");
      kill(pidInterpretador, SIGKILL);
    }

    fclose (fp1) ;
    fclose (fp2) ;

    return 0;
}

/****************************************************************************************************
 *
 *	Função: interpretador
 *
 ****************************************************************************************************/

void interpretador (FILE * fp1, InfoProcesso vetorProcesso[], int *numProgramas, int * statusInterp) {
    int i = 0; /* ret armazena retorno da função escalonador	*/
    char exec[MAX_NOME];            /* auxiliar na leitura do arquivo lendo "exec" ou "prioridade="*/
    numProgramas = 0;

    *statusInterp = 0;        /* Marca que comecou a interpretar */

    while(fscanf (fp1, "%s", exec) == 1) { /* inicia a leitura do arquivo entrada */
        fscanf(fp1,"%s", vetorProcesso[i].nome); /* le do arquivo e guarda no vetor info */

    	for (j = 0; j < 3; j++) {
    		fscanf(fp1, "%d", vetorProcesso[j].prio);
    	}

      (*numProgramas)++;
      i++;
    }

    *statusInterp = 1; //Marca que terminou de interpretar


    /* TRATAR ERROOOOOOOOOOOO */
}



/****************************************************************************************************
 *
 *	Função: criaProcesso
 *
 ****************************************************************************************************/

int criaProcesso (int * pidProcesso, int posicao, char * nomeProcesso) {
    int pid;

    pid = criaPrograma(posicao);

    if ( pid == -1 ) {
        printf("PID -1. Não foi possivel criar o processo. \n");
        return -1;
    }

    *pidProcesso = pid;
    return 0;
}



/****************************************************************************************************
 *
 *	Funções: criaPrograma
 *
 *	Retorno:
 *		pid - pid do processo criado
 *		0 - quando não consegue executar o fork
 *
 ****************************************************************************************************/

int criaPrograma (int n) {
  int pid;
  char nomePrograma[10] = "programa";
  sprintf(nomePrograma, "programa%d", n);

  pid = fork(); /* Executa o fork */

  if (pid < 0) { /* Com problemas no fork retorna erro */
    return 0;

  } else if ( pid != 0 ) { /* PAI */
    kill (pid, SIGSTOP);	/* Pausa processo filho no início da execução */
    return pid; /* Retorna o pid do filho */

  } else { /* FILHO */
    execl(nomePrograma, "nada", NULL);

  }

  return 0;
}


/****************************************************************************************************
 *
 *	Função: escalonador
 *
 ****************************************************************************************************/

int escalonador (FILE * fp2, InfoProcesso vetorProcesso[], int * numProgramas, int * statusInterp) {
  int i;
  Fila f1[MAX_TAM], f2[MAX_TAM], f3[MAX_TAM];
  InfoProcesso current_proccess;

  f1 = fila_cria(1);
  f2 = fila_cria(2);
  f3 = fila_cria(4);

  for (i=0; i < numProgramas; i++) { /* insere os programas na 1a fila */
	  fila_insere(f1, vetorProcesso[i]);
  }

  if (fila_vazia(f1) != 1) {
    current_proccess = fila_retira(f1);
    kill(current_proccess.pid, SIGCONT);
  }
}
