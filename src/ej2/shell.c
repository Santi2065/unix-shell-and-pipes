#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

#define MAX_COMMANDS 200
#define MAX_ARGS 64
#define MAX_ALLOWED_ARGS 63  // Se permiten hasta 63 argumentos (sin contar el comando).

// Elimina los espacios al inicio y final de la cadena.
char* trim(char *str) {
    while (isspace((unsigned char)*str)) str++;  // Salta espacios al inicio.
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end = '\0';  // Reemplaza el espacio por fin de cadena.
        end--;
    }
    return str;
}

// Separa la línea en argumentos, manejando comillas simples y dobles.
// Retorna la cantidad de argumentos o -1 si falta cerrar alguna comilla.
int parse_command(char *command, char **args) {
    int count = 0;
    char *p = command;
    while (*p) {
        // Omite espacios en blanco.
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        if (*p == '\"' || *p == '\'') {
            char quote = *p;
            p++;  // Salta la comilla de apertura.
            char *start = p;
            while (*p && *p != quote) p++;  // Busca la comilla de cierre.
            if (*p != quote) {
                // Error: falta comilla de cierre.
                return -1;
            }
            int len = p - start;
            args[count] = malloc(len + 1);
            if (!args[count]) exit(EXIT_FAILURE);
            strncpy(args[count], start, len);
            args[count][len] = '\0';
            count++;
            p++;  // Salta la comilla de cierre.
        } else {
            char *start = p;
            while (*p && !isspace((unsigned char)*p)) p++;
            int len = p - start;
            args[count] = malloc(len + 1);
            if (!args[count]) exit(EXIT_FAILURE);
            strncpy(args[count], start, len);
            args[count][len] = '\0';
            count++;
        }
    }
    args[count] = NULL;  // Termina la lista.
    return count;
}

// Divide la línea en comandos separados por '|' ignorando los que estén
// dentro de comillas. Retorna la cantidad de comandos.
int split_pipeline(char *line, char **cmds) {
    int count = 0, start = 0;
    int in_quote = 0;
    char quote_char = '\0';
    for (int i = 0; line[i] != '\0'; i++) {
        char c = line[i];
        if (in_quote) {
            if (c == quote_char)
                in_quote = 0;
        } else {
            if (c == '\"' || c == '\'') {
                in_quote = 1;
                quote_char = c;
            } else if (c == '|') {
                line[i] = '\0'; 
                cmds[count++] = trim(&line[start]);
                start = i + 1;
            }
        }
    }
    cmds[count++] = trim(&line[start]);
    return count;
}

int main() {
    // Aumentamos el buffer para soportar líneas de comando largas.
    char command[4096];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) 
    {
        // Muestra el prompt solo si se está en modo interactivo.
        if (isatty(STDIN_FILENO))
            printf("Shell> ");
        
        if (!fgets(command, sizeof(command), stdin))
            break;
        
        // Quita el salto de línea al final.
        command[strcspn(command, "\n")] = '\0';

        // Si no hay entrada, continúa.
        if (strlen(command) == 0)
            continue;
        
        // Si se escribe "exit", termina la shell.
        if (strcmp(command, "exit") == 0)
            break;
        
        // Verifica que no se use el pipe al inicio o al final.
        if (command[0] == '|' || command[strlen(command)-1] == '|') {
            fprintf(stderr, "Error de sintaxis cerca de '|'\n");
            continue;
        }

        // Separa la línea en comandos por el carácter '|' (ignorando comillas).
        command_count = split_pipeline(command, commands);
        
        // Asegura que ninguno de los comandos resultantes esté vacío.
        int valido = 1;
        for (int i = 0; i < command_count; i++) {
            if (strlen(commands[i]) == 0) {
                fprintf(stderr, "Error de sintaxis: comando vacío entre pipes\n");
                valido = 0;
                break;
            }
        }
        if (!valido) continue;
        
        // Crea los pipes necesarios para conectar los procesos.
        int pipes[MAX_COMMANDS-1][2];
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }
        
        // Crea un proceso para cada comando de la tubería.
        for (int i = 0; i < command_count; i++) {
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) { // Proceso hijo
                // Si NO es el primer comando, redirige la entrada desde el pipe previo.
                if (i > 0) {
                    dup2(pipes[i-1][0], STDIN_FILENO);
                }
                // Si NO es el último comando, redirige la salida hacia el siguiente pipe.
                if (i < command_count - 1) {
                    dup2(pipes[i][1], STDOUT_FILENO);
                }
                // Cierra todos los pipes en el hijo.
                for (int j = 0; j < command_count - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                
                // Si el comando es "exit" dentro de una tubería, simplemente sale.
                char *cmd_trim = trim(commands[i]);
                if (strcmp(cmd_trim, "exit") == 0) {
                    exit(EXIT_SUCCESS);
                }
                
                // Separa el comando en argumentos (manejo básico de comillas).
                char *args[MAX_ARGS];
                int arg_count = parse_command(commands[i], args);
                if (arg_count == -1) {
                    fprintf(stderr, "Error de sintaxis: comilla sin cerrar\n");
                    exit(EXIT_FAILURE);
                }
                // Verifica que no se exceda el número permitido de argumentos (sin contar el comando).
                if (arg_count > 0 && (arg_count - 1) > MAX_ALLOWED_ARGS) {
                    fprintf(stderr, "Error: demasiados argumentos\n");
                    exit(EXIT_FAILURE);
                }
                if (arg_count == 0)
                    exit(EXIT_FAILURE);
                
                // Ejecuta el comando.
                execvp(args[0], args);
                
                // En caso de error al ejecutar, muestra el mensaje y finaliza.
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            // El proceso padre cierra los extremos de los pipes ya usados.
            if (i > 0) {
                close(pipes[i-1][0]);
                close(pipes[i-1][1]);
            }
        }
        
        // Si hay más de un comando, cierra los pipes restantes.
        if (command_count > 1) {
            close(pipes[command_count - 2][0]);
            close(pipes[command_count - 2][1]);
        }
        // El padre espera a que terminen todos los procesos hijos.
        for (int i = 0; i < command_count; i++) {
            wait(NULL);
        }
    }
    return 0;
}
