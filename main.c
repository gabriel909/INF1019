
/****************************************************************************************************
 *
 *    TRABALHO 1 - Sistemas de Computação
 *    Professor: Luiz Fernando Seibel
 *    25/10/2017
 *
 *    Aluno:    Gabriel Barbosa de Oliveira - 1410578
 *    Aluno:    Karina Fernandes Tronkos - 1511690
 *
 *    O programa lê, na ordem dada, as seguintes informações de um arquivo "entrada.txt" :
 *
 *        1. A palavra exec indicando mais um programa a ser executado ;
 *
 *        2. O nome do programa a ser executado, o nome do programa é uma string sempre de nome
 *       programaN, onde n é um numero inteiro de 1 a 7
 *
 *    Apos ler e armazenar as informações, o programa chama executa os processos na ordem cor-
 *    reta, conforme especificado no enunciado do trabalho.
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

#define MAX_PROG 5 /* num max de programas */
#define MAX_TAM 10
#define MAX_PRIO 15
#define MAX_NOME 20
#define N 100
#define EVER ;;

/***************************************************************************************************/

//Estruturas de dados auxiliares.
typedef struct infoProcesso {
    char nome[MAX_NOME]; /* Nome do programa referente ao processo */
    int pid;       /* PID do processo */
    int prio[MAX_PRIO];         /* Prioridade do processo */
    int status;    /* 0 caso rolando, 1 caso terminado */
} InfoProcesso;

/***************************************************************************************************/

/****************************************************************************************************
 *
 *    FILA
 *
 ****************************************************************************************************/

typedef struct fila {
    int n; /* numero de elementos armazenados na fila */
    int ini; /* indice para o inicio da fila */
    InfoProcesso vet[N];
    int prio;
} Fila;

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
        printf("Capacidade da fila estourou. \n");
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
      printf("Fila vazia. \n");
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

/****************************************************************************************************
 *
 *    MAIN
 *
 ****************************************************************************************************/

void interpretador(FILE * fp1, InfoProcesso vetorProcesso[], int * numProgramas, int * statusInterp);
int criaProcesso(int * pidProcesso, int posicao);
int escalonador(FILE * fp2, InfoProcesso vetorProcesso[], int * numProgramas, int * statusInterp);
int criaPrograma(int n);
void sleep_scheduler(long time_slice);
void schedule(Fila *fila, Fila *fila2, Fila *fila3);
void asked_for_io();
void finished_io();

int did_ask_for_io = 0;
InfoProcesso current_proccess;

int array_count(void* array) {
  return (int) sizeof(array) / sizeof(array[0]);
}

int is_empty(int* array) {
  int i = 0;

  while(i <= array_count(array)) {
    if(array[i] != 0) {
      return 0;
    }

    i++;
  }

  return 1;
}

int change_burst(int burst[], int f_slice) {
  int i = 0;

  for(EVER) {
    if(burst[i] != 0) {
      if(burst[i] > f_slice) {
        burst[i] -= f_slice;
        printf("BURST LEFT %d\n", burst[i]);
        return 1;

      } else {
        burst[i] = 0;
        printf("BURST LEFT %d\n", 0);
        return 0;

      }
    }

    i++;
  }
}

/***************************************************************************************************/

int main (void) {
  InfoProcesso vetorProcesso[MAX_PROG];
  int * numProgramas = malloc(sizeof(*numProgramas));
  int * statusInterp = malloc(sizeof(*statusInterp));
  int pidInterpretador;
  int i;
  int fd[2];

  signal(SIGUSR1, asked_for_io);
  signal(SIGUSR2, finished_io);

  FILE * fp1 = fopen ("entrada.txt", "r") ; /* Abertura de arquivo de entrada */
  FILE * fp2 = fopen ("saida.txt", "w") ;   /* Abertura de arquivo de saida  */

  if(pipe(fd) < 0) {
    printf("Erro no pipe\n");
    exit(-1);
  }

  *numProgramas = 0;
  *statusInterp = 0;

  for (i = 0; i < MAX_PROG; i++){
    vetorProcesso[i].status = 1;

    for(int j = 0; j < MAX_PRIO; j++) {
      vetorProcesso[i].prio[j] = 1000;
    }
  }

  if (fp1 == NULL || fp2 == NULL) {  /* Teste de validação de abertura */
    puts("ERRO");
    fprintf(fp2, "Erro na abertura de arquivo. \n") ;
    return 1 ;
  }

  pidInterpretador = fork();

  if(pidInterpretador == 0) { // Filho - Interpretador
    puts("INTERPRETADOR");
    close(fd[0]);
    interpretador(fp1, vetorProcesso, numProgramas, statusInterp);
    puts("INTERPRETADOR END");
    printf("%d\n", *numProgramas);
    write(fd[1], numProgramas, sizeof(int));

  } else { // Pai - Escalonador
    close(fd[1]);
    read(fd[0], numProgramas, sizeof(int));

    while(*numProgramas == 0); //Espera ler ao menos um programa para iniciar escalonamento

    puts("ESCALONADOR");

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
 *    Função: interpretador
 *
 ****************************************************************************************************/

void interpretador(FILE * fp1, InfoProcesso vetorProcesso[], int *numProgramas, int * statusInterp) {
  int i = 0; /* ret armazena retorno da função escalonador    */
  char exec[MAX_NOME];            /* auxiliar na leitura do arquivo lendo "exec" ou "prioridade="*/
  *numProgramas = 0;
  *statusInterp = 0;        /* Marca que comecou a interpretar */

  while(fscanf(fp1, "%s", exec) == 1) { /* inicia a leitura do arquivo entrada */
    puts("INTERPRETADOR WHILE");
    fscanf(fp1,"%s", vetorProcesso[i].nome); /* le do arquivo e guarda no vetor info */
    printf("%s\n", vetorProcesso[i].nome);

    for(int j = 0; j < 3; j++) {
      fscanf(fp1, "%d", &vetorProcesso[i].prio[j]);
      printf("BURST %d\n", vetorProcesso[j].prio[j]);
    }

    (*numProgramas)++;

    printf("%d\n", *numProgramas);
    i++;
  }

  *statusInterp = 1; //Marca que terminou de interpretar
}

/****************************************************************************************************
 *
 *    Função: criaProcesso
 *
 ****************************************************************************************************/

int criaProcesso(int * pidProcesso, int posicao) {
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
 *    Funções: criaPrograma
 *
 *    Retorno:
 *        pid - pid do processo criado
 *        0 - quando não consegue executar o fork
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
    kill(pid, SIGSTOP);    /* Pausa processo filho no início da execução */
    return pid; /* Retorna o pid do filho */

  } else { /* FILHO */
    execl(nomePrograma, "1", "2", "3", NULL);

  }

  return 0;
}


/****************************************************************************************************
 *
 *    Função: escalonador
 *
 ****************************************************************************************************/

int escalonador (FILE * fp2, InfoProcesso vetorProcesso[], int * numProgramas, int * statusInterp) {
  int i;
  Fila *f1, *f2, *f3;

  f1 = fila_cria(1);
  f2 = fila_cria(2);
  f3 = fila_cria(4);

  for (i=0; i < *numProgramas; i++) { /* insere os programas na 1a fila */
    criaProcesso(&vetorProcesso[i].pid, i);
    fila_insere(f1, vetorProcesso[i]);
  }

  puts("INSERIU FILA 1");

  while(vetorProcesso != 0) {
    if (fila_vazia(f1) != 1) {
      schedule(f1, f2, NULL);

    } else if (fila_vazia(f2) != 1) {
      //Trocar!!!!!
      schedule(f2, f3, f1);

    } else if (fila_vazia(f3) != 1) {
      schedule(f3, f2, NULL);

    }
  }

  return 0;
}

void schedule(Fila *fila, Fila* fila2, Fila *fila3) {
  current_proccess = fila_retira(fila);
  kill(current_proccess.pid, SIGCONT);

  sleep_scheduler(fila->prio);

  printf("CURRENT PROCCESS PID %d\n", current_proccess.pid);
  kill(current_proccess.pid, SIGSTOP);
  did_ask_for_io = 0;

  if(change_burst(current_proccess.prio, fila->prio) == 1) {
    fila_insere(fila2, current_proccess);

  } else {
    if(fila3 == NULL) {
      fila_insere(fila, current_proccess);

    } else {
      fila_insere(fila3, current_proccess);

    }
  }
}

void sleep_scheduler(long time_slice) {
  long tempo = time(NULL);

  for(EVER) {
    long new_time = time(NULL);

    if(((new_time - tempo) == time_slice) || did_ask_for_io != 0) {
      break;
    }
  }
}

void asked_for_io() {
  puts("ASKED FOR IO");
  did_ask_for_io = 1;
}

void finished_io() {

}
