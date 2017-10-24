typedef struct infoProcesso InfoProcesso;

typedef struct fila Fila;

Fila* fila_cria(int prioridade);

void fila_insere(Fila* f, InfoProcesso processo);

InfoProcesso fila_retira(Fila* f);

int fila_vazia(Fila* f);

void fila_libera(Fila *f);
