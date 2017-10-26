
/****************************************************************************************************
 *
 *    TRABALHO 1 - Sistemas de Computação
 *    Professor: Luiz Fernando Seibel
 *    25/10/2017
 *
 *    Aluno:    Gabriel Barbosa de Oliveira - 1410578
 *    Aluno:    Karina Fernandes Tronkos - 1511690
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

#define MAX_PROG 5
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
    int fila;
    int prio_count;
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

//Cabeçalho das Funções
Fila* interpretador(FILE * fp1, Fila *fila_processo, int * numProgramas, int * statusInterp);
int criaProcesso(int * pidProcesso, int posicao, int array_burst[], int bursts_count);
int escalonador(FILE * fp2, Fila *fila_processo, int * numProgramas, int * statusInterp);
int criaPrograma(int n, int array_burst[], int bursts_count);
void sleep_scheduler(long time_slice);
void schedule(Fila *fila, Fila *fila2, Fila *fila3, int fila_atual);
void asked_for_io();
void finished_io();
void finished_execution(int signum);

//Variáveis globais
int did_ask_for_io = 0;
InfoProcesso current_proccess;
Fila *f1, *f2, *f3;
Fila *fila_io;
Fila *fila_processo;
Fila *running;

//Funções Auxiliares
int array_count(int array[]) {
  int count = (int) sizeof(*array) / sizeof(int);

  return count;
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

int change_burst(int *burst, int f_slice, char* pid) {
  int i;

  for(i = 0; i < MAX_PRIO; i++) {
    *burst = *(burst + i * 4);
    if(*burst > 0) {
      if(*burst > f_slice) {
        printf("BURST %d\n", *burst);
        *burst -= f_slice;
        printf("BURST LEFT %d\n", *burst);
        return 1;

      } else {
        printf("BURST %d %s\n", *burst, pid);
        *burst = 0;
        printf("BURST LEFT %d\n", 0);
        return 0;

      }
    }
  }

  return 0;
}

char** convert_int_to_string_array(int array[], int count_array) {
  char** string_array = malloc(count_array * sizeof(char*));
  int i = 0;

  for (i = 0; i < count_array; i++) {
    char c[sizeof(int)];

    snprintf(c, sizeof(int), "%d", array[i]);

    string_array[i] = malloc(sizeof(c));
    strcpy(string_array[i], c);
  }

  return string_array;
}

/***************************************************************************************************/

int main (void) {
  fila_processo = fila_cria(-1);
  fila_io = fila_cria(-1);
  running = fila_cria(-1);

  int * numProgramas = malloc(sizeof(*numProgramas));
  int * statusInterp = malloc(sizeof(*statusInterp));
  size_t * tam_pipe = malloc(sizeof(size_t));
  int pidInterpretador;
  int fd[2];

  signal(SIGUSR1, asked_for_io);
  signal(SIGUSR2, finished_io);
  signal(SIGCHLD, finished_execution);

  FILE * fp1 = fopen ("entrada.txt", "r") ; /* Abertura de arquivo de entrada */
  FILE * fp2 = fopen ("saida.txt", "w") ;   /* Abertura de arquivo de saida  */

  if(pipe(fd) < 0) {
    printf("Erro no pipe\n");
    exit(-1);
  }

  *numProgramas = 0;
  *statusInterp = 0;

  if (fp1 == NULL || fp2 == NULL) {  /* Teste de validação de abertura */
    puts("ERRO");
    fprintf(fp2, "Erro na abertura de arquivo. \n") ;
    return 1 ;
  }

  pidInterpretador = fork();

  if(pidInterpretador == 0) { // Filho - Interpretador
    puts("INTERPRETADOR");
    close(fd[0]);
    fila_processo = interpretador(fp1, fila_processo, numProgramas, statusInterp);

    puts("INTERPRETADOR END");
    printf("%d\n", *numProgramas);
    write(fd[1], numProgramas, sizeof(int));

    *tam_pipe = sizeof(*fila_processo);

    printf("TAM PIPE %lu\n", *tam_pipe);
    write(fd[1], tam_pipe, sizeof(size_t));
    write(fd[1], fila_processo, sizeof(*fila_processo));

  } else { // Pai - Escalonador
    close(fd[1]);
    read(fd[0], numProgramas, sizeof(int));
    read(fd[0], tam_pipe, sizeof(size_t));
    read(fd[0], fila_processo, *tam_pipe);

    while(*numProgramas == 0); //Espera ler ao menos um programa para iniciar escalonamento

    escalonador(fp2, fila_processo, numProgramas, statusInterp) ; //Chama funcao do escalonador
    printf("Escalonamento chegou ao fim! \n");
    kill(pidInterpretador, SIGKILL);
  }

  fclose(fp1);
  fclose(fp2);

  return 0;
}

/****************************************************************************************************
 *
 *    Função: interpretador
 *
 ****************************************************************************************************/

Fila* interpretador(FILE * fp1, Fila *fila_processo, int *numProgramas, int * statusInterp) {
  // int i = 0; /* ret armazena retorno da função escalonador    */
  int j = 0;
  *numProgramas = 0;
  *statusInterp = 0;
  InfoProcesso *processo = (InfoProcesso*) malloc(sizeof(InfoProcesso));

  // fscanf(fp1, "%d", numProgramas);
  scanf("%ds", numProgramas);
  printf("NUMPROGRAMAS %d\n", *numProgramas);

  // i = 0;

  // while(scanf(" exec %s (%d,%d,%d)", processo->nome, &processo->prio[0], &processo->prio[1], &processo->prio[2]) == 4) {
  //   // j++;
  //   fila_insere(fila_processo, *processo);
  // }

  // *numProgramas = j;

  for(int i = 0; i < *numProgramas; i++) { /* inicia a leitura do arquivo entrada */
    // fscanf(fp1,"%s", processo->nome); /* le do arquivo e guarda no vetor info */
    scanf("%s", processo->nome);
    printf("NOME INTERPRETADOR %s\n", processo->nome);

    for(j = 0; j < 3; j++) {
      // fscanf(fp1, "%d", &processo->prio[j]);
      scanf("%d", &processo->prio[j]);
      printf("BURST INTERPRETADOR %d\n", processo->prio[j]);
    }

    processo->prio_count = j;

    fila_insere(fila_processo, *processo);

    // i++;
  }

  *statusInterp = 1; //Marca que terminou de interpretar

  return fila_processo;
}

/****************************************************************************************************
 *
 *    Função: criaProcesso
 *
 ****************************************************************************************************/

int criaProcesso(int * pidProcesso, int posicao, int array_burst[], int bursts_count) {
  int pid;

  pid = criaPrograma(posicao, array_burst, bursts_count);

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
 *
 ****************************************************************************************************/

int criaPrograma (int n, int array_burst[], int bursts_count) {
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
    execv(nomePrograma, convert_int_to_string_array(array_burst, bursts_count));

  }

  return 0;
}


/****************************************************************************************************
 *
 *    Função: escalonador
 *
 ****************************************************************************************************/

int escalonador (FILE * fp2, Fila *fila_processo, int * numProgramas, int * statusInterp) {
  int i;

  f1 = fila_cria(1);
  f2 = fila_cria(2);
  f3 = fila_cria(4);

  int flag = 0;

  for(i = 0; i < *numProgramas; i++) { /* insere os programas na 1a fila */
    InfoProcesso processo = fila_retira(fila_processo);
    criaProcesso(&processo.pid, i, processo.prio, processo.prio_count);
    fila_insere(f1, processo);
    fila_insere(fila_processo, processo);
  }

  while(!fila_vazia(fila_processo)) {
    if (!fila_vazia(f1)) {
      puts("********* ESTOU NA FILA 1 *********");
      schedule(f1, f2, NULL, 1);

    } else if (!fila_vazia(f2)) {
      puts("********* ESTOU NA FILA 2 *********");
      schedule(f2, f3, f1, 2);

    } else if (!fila_vazia(f3)) {
      puts("********* ESTOU NA FILA 3 *********");
      schedule(f3, f2, NULL, 3);

    }
  }

  return 0;
}

void schedule(Fila *fila, Fila* fila2, Fila *fila3, int fila_atual) {
  current_proccess = fila_retira(fila);
  kill(current_proccess.pid, SIGCONT);
  current_proccess.fila = fila_atual;
  fila_insere(running, current_proccess);
  printf("PROCESSO %s\n", current_proccess.nome);

  sleep_scheduler(fila->prio);

  if(did_ask_for_io) {
    did_ask_for_io = 0;


  } else {
    if(change_burst(&(current_proccess.prio), fila->prio, current_proccess.nome) == 1) {
      if (current_proccess.pid != 0) {
        printf("========== SIGSTOP %s\n", current_proccess.nome);
        kill(current_proccess.pid, SIGSTOP);
        did_ask_for_io = 0;
      }

      fila_insere(fila2, current_proccess);

    }
  }
}

void sleep_scheduler(long time_slice) {
  long tempo = time(NULL);

  for(EVER) {
    long new_time = time(NULL);

    if(((new_time - tempo) >= time_slice) || did_ask_for_io) {
      break;
    }
  }
}

void asked_for_io() {
  InfoProcesso running_process = fila_retira(running);
  puts("ASKED FOR IO >>>>>");
  printf("PROCESSO %s >>>>>\n", running_process.nome);
  fila_insere(fila_io, running_process);
  did_ask_for_io = 1;
}

void finished_io() {
  puts("<<<< VOLTEI DE IO");
  did_ask_for_io = 0;

  if(!fila_vazia(fila_io)) {
    InfoProcesso out = fila_retira(fila_io);
    printf("<<<< PROCESSO %s\n", out.nome);

    kill(out.pid, SIGSTOP);

    switch(out.fila) {
      case 1:
        fila_insere(f1, out);
        break;

      case 2:
        fila_insere(f1, out);
        break;

      case 3:
        fila_insere(f2, out);
        break;
    }
  }
}

void finished_execution(int signum) {
  if(signum != 20) {
    puts("PROCESSO TERMINOU");
    // fila_retira(fila_processo);
  }
}
