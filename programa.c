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

	for(i = 0; i < argc; i++) {
		for(j = 0; j < atoi(argv[i]); j++) {
			printf("PROGRAMA %d \n", getpid());
			puts("");
			sleep(1);
		}

		int p_pid = getppid();
		kill(p_pid, SIGUSR1);

		sleep(3);
		kill(p_pid, SIGUSR2);
	}

	puts("SAI DO FOR DO PROCESSO");

	return 0;
}
