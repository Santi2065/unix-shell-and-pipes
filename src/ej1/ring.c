#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char **argv)
{	
	int start, status, pid, n;
	int buffer[1];

	if (argc != 4){ printf("Uso: anillo <n> <c> <s> \n"); exit(0);}
    
    /* Parsing of arguments */

    // Parseo de argv
    n        = atoi(argv[1]);
    buffer[0]= atoi(argv[2]);
    start    = atoi(argv[3]);

    // Validaciones básicas
    if (n < 1 || start < 1 || start > n) {
        fprintf(stderr, "Parámetros inválidos\n");
        exit(EXIT_FAILURE);
    }

    printf("Se crearán %i procesos, se enviará el caracter %i desde proceso %i \n", n, buffer[0], start);
    
   	/* You should start programming from here... */
	
	// Crear los pipes del anillo: pipes[i] conecta proceso i -> proceso (i+1)%n
    int pipes[n][2];
    for(int i = 0; i < n; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Crear procesos hijos
    for(int i = 0; i < n; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            // Proceso hijo con índice i (id = i+1)
            int pred = (i - 1 + n) % n;  // índice del pipe de lectura
            int succ = i;                // índice del pipe de escritura
            
            // Cerrar extremos no usados
            for(int j = 0; j < n; j++) {
                if (j == pred) {
                    close(pipes[j][1]);  // cierro escritura del pipe de entrada
                } else if (j == succ) {
                    close(pipes[j][0]);  // cierro lectura del pipe de salida
                } else {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }
            
            // Leer mensaje, incrementar y reenviar
            int msg;
            if (read(pipes[pred][0], &msg, sizeof(msg)) != sizeof(msg)) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            close(pipes[pred][0]);

            msg++;

            if (write(pipes[succ][1], &msg, sizeof(msg)) != sizeof(msg)) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            close(pipes[succ][1]);
            exit(EXIT_SUCCESS);
        }
    }

    // Padre: cerrar extremos excepto escritura en start-1 y lectura en su predecesor
    int pred_parent = (start - 2 + n) % n;
    for(int i = 0; i < n; i++) {
        if (i == (start - 1)) {
            // mantengo pipes[i][1]
            close(pipes[i][0]);
        } else if (i == pred_parent) {
            // mantengo pipes[pred_parent][0]
            close(pipes[i][1]);
        } else {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
    }

    // Enviar mensaje inicial
    if (write(pipes[start - 1][1], &buffer[0], sizeof(buffer[0])) != sizeof(buffer[0])) {
        perror("write padre");
        exit(EXIT_FAILURE);
    }
    close(pipes[start - 1][1]);

    // Leer resultado final
    int result;
    if (read(pipes[pred_parent][0], &result, sizeof(result)) != sizeof(result)) {
        perror("read padre");
        exit(EXIT_FAILURE);
    }
    close(pipes[pred_parent][0]);

    printf("Resultado final: %d\n", result);

    // Esperar terminación de hijos
    for(int j = 0; j < n; j++)
        wait(&status);

    return 0;
}
