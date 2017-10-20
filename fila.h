typedef struct fila Fila;

Fila* fila_cria(void);

void fila_insere(Fila* f, float v);

float fila_retira(Fila* f);

int fila_vazia(Fila* f);

void fila_libera(Fila *f);