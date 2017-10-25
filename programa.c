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

int main (int argc, char *argv[]) {
	int i, j;

	printf("PROGRAMA USUARIO %d\n", argc);

	for(i=0; i < argc; i++) {
		printf("EU AQUI DE NOVO %s\n", argv[0]);
		for(j=0; j < atoi(argv[i]); j++) {
			printf("PROGRAMA %d \n", getpid());
			sleep(1);
		}

		int p_pid = getppid();
		kill(p_pid, SIGUSR1);

		sleep(3);
		puts("ACORDEI");
		kill(p_pid, SIGUSR2);
	}

	return 0;
}
