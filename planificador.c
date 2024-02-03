#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define TIEMPO_REAL 0
#define USUARIO 1 

#define tam_memoria 1024

typedef enum {
    tiempo_real,
    user
} tipo_proceso;

typedef enum {
    inicio,
    listo, 
    esperando, 
    terminado
} ciclo_vida_proceso;

typedef struct proceso_t proceso_t;
typedef struct recurso recurso;

#define num_impresora 2
#define num_cd 2

typedef struct recurso {
    int disponible; // 0 si está disponible, 1 si está en uso
    int utilizado_por;  // ID del proceso que está utilizando el recurso; tiene un valor de -1 si no está en uso por nadie
} recurso;

recurso impresora[num_impresora];
recurso cd[num_cd];
recurso escaner;
recurso modem;


typedef struct proceso_t {
    int tiempo_llegada;
    int prioridad;
    int tiempo_proceso;
    int m_bytes;
    int impresoras;
    int escaners;
    int modems;
    int cds;
    tipo_proceso tipo;
    proceso_t *siguiente_proceso;
    pid_t pid;
    int id;
    int color;
} proceso_t;



proceso_t* rt_list = NULL; 
proceso_t* usr_list = NULL; 
int memoria[tam_memoria];

//#define _PRINT_MEMORY_ = 0;
//#define _PRINT_ACTIONS_ = 0;

// Prototipo de la función para imprimir una lista de procesos
void print_process_list(const char* list_name, proceso_t* head);
void inicializarMemoria(int s, int tamaño);





// Función para inicializar recursos
void initialize_resources() {
    // Inicializar impresoras y unidades de CD
    for (int num = 0; num < num_impresora; num++) {
        impresora[num].disponible = 0;  // No disponible
        impresora[num].utilizado_por = -1;  // No tomado por ningún proceso

        cd[num].disponible = 0;  // No disponible
        cd[num].utilizado_por = -1;  // No tomado por ningún proceso
    }

    // Inicializar escáner
    escaner.disponible = 0;  // No disponible
    escaner.utilizado_por = -1;  // No tomado por ningún proceso

    // Inicializar módem
    modem.disponible = 0;  // No disponible
    modem.utilizado_por = -1;  // No tomado por ningún proceso
}

// Función para obtener el tiempo actual en microsegundos
long get_current_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void execute_user_process(proceso_t* process) {
    printf("\n");
    printf("\e[0;%dmEjecutando proceso de Usuario:\n", process->color);  // Usando el color del proceso 
    printf("ID: %d, Tiempo de proceso: %d\n", process->id, process->tiempo_proceso);
    long start_time = get_current_time();  // Tiempo de inicio del proceso
    sleep(process->tiempo_proceso);  // Simulación de ejecución del proceso
    long end_time = get_current_time();  // Tiempo de finalización del proceso
    double elapsed_time = (end_time - start_time) / 1000000.0; // convertir a segundos
    printf("Proceso de Usuario ID: %d ha ocupado %.6f segundos en el sistema.\n", process->id, elapsed_time);
    printf("\n");
    
}

void execute_rt_process(proceso_t* process) {
    printf("\n");
    printf("\e[0;%dmEjecutando proceso de Tiempo Real:\n", process->color);  // Usando el color del proceso
    printf("ID: %d, Tiempo de proceso: %d\n", process->id, process->tiempo_proceso);
    long start_time = get_current_time();  // Tiempo de inicio del proceso
    sleep(process->tiempo_proceso);  // Simulación de ejecución del proceso
    long end_time = get_current_time();  // Tiempo de finalización del proceso
    double elapsed_time = (end_time - start_time) / 1000000.0; // convertir a segundos
    printf("Proceso de Tiempo Real ID: %d ha ocupado %.6f segundos en el sistema.\n", process->id, elapsed_time);
    printf("\n");
}



// Función principal del dispatcher
void dispatcher() {
    initialize_resources();

    // Variables del reloj y procesos actuales
    int disp_clock = 0;
    proceso_t* current_rt_process = NULL;
    proceso_t* current_user_process = NULL;

    // Bucle principal del dispatcher
    while (rt_list != NULL || usr_list != NULL) { 
        // Manejar procesos de tiempo real que han llegado
        if(rt_list != NULL && rt_list->tiempo_llegada <= disp_clock) {
            current_rt_process = rt_list;
            rt_list = rt_list->siguiente_proceso;

            // Verificar si hay recursos disponibles para el proceso
            if (current_rt_process->impresoras <= num_impresora && current_rt_process->cds <= num_cd) {
                // Asignar recursos al proceso y ejecutarlo
                current_rt_process->siguiente_proceso = NULL;
                current_rt_process->pid = fork();

                if (current_rt_process->pid == 0) {
                    // Proceso hijo
                    execute_rt_process(current_rt_process);
                    
                    exit(0);
                }
            } else {
                printf("Recursos insuficientes para el proceso de Tiempo Real ID: %d\n", current_rt_process->id);
            
            }
        } else {
            // Si no hay procesos de tiempo real, manejar procesos de usuario
            if (usr_list != NULL && usr_list->tiempo_llegada <= disp_clock) {
                current_user_process = usr_list;
                usr_list = usr_list->siguiente_proceso;

                // Verificar si hay recursos disponibles para el proceso
                if (current_user_process->impresoras <= num_impresora && current_user_process->cds <= num_cd) {
                    // Asignar recursos al proceso y ejecutarlo
                    current_user_process->siguiente_proceso = NULL;
                    current_user_process->pid = fork();

                     // Código después de la creación del proceso hijo
                        if (current_user_process->pid == 0) {
                            execute_user_process(current_user_process);
                            
                            exit(0); // Asegúrate de que el proceso hijo salga correctamente
                        } else {
                            // Impresión de depuración
                            //printf("Proceso hijo creado con PID: %d\n", current_user_process->pid);

                            // Código para esperar a que el proceso hijo termine
                            int status;
                            waitpid(current_user_process->pid, &status, 0);

                            // Impresión de depuración
                            //printf("Proceso hijo con PID %d ha terminado\n", current_user_process->pid);

                        }

                } else {
                    printf("Recursos insuficientes para el proceso de Usuario ID: %d\n", current_user_process->id);
                }
            }
        }

        // Incrementar el reloj y esperar 1 segundo
        disp_clock++;
        sleep(1);
    }

    printf("Dispatcher ha completado la ejecución de todos los procesos.\n");
}


int main(int argc, char** argv) {


    char *filename;

    if (argc != 2) {
        printf("Error: uso ./hostd dispatch_list.txt\n");
        exit(1);
    }

    printf("esto funcionando ");

    filename = argv[1];


    /*zeroMemory(0, HOST_MEM-1);
    dispatcher();*/

    FILE *fp;                   // Puntero al archivo
    char t[100];                 // Variable temporal para almacenar líneas del archivo
    char *token;                // Variable para almacenar los tokens obtenidos al dividir la línea
    int i, j, id = 0;           // Variables de control y un identificador único para cada proceso
    int array[8];               // Arreglo para almacenar los valores de los atributos de un proceso
    proceso_t *temp;            // Puntero temporal para crear y llenar la información de un proceso

    // Abre el archivo para lectura
    fp = fopen(filename, "r");

    // Verifica si el archivo se abrió correctamente
    if (fp == NULL) {
        printf("Error: no se pudo abrir el archivo %s\n", filename);
        exit(1);
    }

    i = 0;
    j = 0;
    
    while (fgets(t, 100, fp) != NULL) {

        // Si encuentra una línea en blanco, termina la lectura
        if (strcmp(t, "\n") == 0) {
            break;
        }

        // Tokeniza la línea utilizando la coma como delimitador
        token = strtok(t, ", ");
        while (token != NULL) {
            array[j] = atoi(token);
            j += 1;
            token = strtok(NULL, ",");
        }

        // Reserva memoria para un nuevo proceso_t
        temp = malloc(sizeof(proceso_t));

        // Llena una estructura proceso_t con los valores del array
        temp->tiempo_llegada = array[0];
        temp->prioridad = array[1];
        temp->tiempo_proceso = array[2];
        temp->m_bytes = array[3];
        temp->impresoras = array[4];
        temp->escaners = array[5];
        temp->modems = array[6];
        temp->cds = array[7];

        temp->siguiente_proceso = NULL;
        temp->id = id++;
        temp->color = rand() % 8 + 30; 
        /*
        temp->pid = -1;
        
        */

        // Establece el tipo de proceso
        temp->tipo = (temp->prioridad == 0) ? TIEMPO_REAL : USUARIO;

        // Decide en qué cola colocar el proceso y en qué orden
        //dispatch_to_appropriate_queue(temp);

        
         // Decide en qué cola colocar el proceso y en qué orden
        if (temp->tipo == TIEMPO_REAL) {
            // Lista de tiempo real
            if (rt_list == NULL) {
                rt_list = temp;
            } else {
                proceso_t* it = rt_list;
                proceso_t* prev = rt_list;

                while (it != NULL) {
                    if (it->tiempo_llegada >= temp->tiempo_llegada) {
                        temp->siguiente_proceso = it;
                        if (it != rt_list)
                            prev->siguiente_proceso = temp;
                        else
                            rt_list = temp;
                        break;
                    } else {
                        prev = it;
                        it = it->siguiente_proceso;
                    }
                }

                if (it == NULL) {
                    prev->siguiente_proceso = temp;
                }
            }
        } else {
            // Lista de usuario
            if (usr_list == NULL) {
                usr_list = temp;
            } else {
                proceso_t* it = usr_list;
                proceso_t* prev = usr_list;

                while (it != NULL) {
                    if (it->tiempo_llegada >= temp->tiempo_llegada) {
                        temp->siguiente_proceso = it;
                        if (it != usr_list)
                            prev->siguiente_proceso = temp;
                        else
                            usr_list = temp;
                        break;
                    } else {
                        prev = it;
                        it = it->siguiente_proceso;
                    }
                }

                if (it == NULL) {
                    prev->siguiente_proceso = temp;
                }
            }
        }

        i += 1;
        j = 0;
    }

    fclose(fp);

    // Imprimir listas después de leer el archivo
    print_process_list("Time Real", rt_list);
    print_process_list("Usuario", usr_list);

    inicializarMemoria(0, tam_memoria-1);

    dispatcher();

    //---------------------------------

    // continuandoooooo...


    return 0;
}

void print_process_list(const char* list_name, proceso_t* head) {
    printf("\n%s List:\n", list_name);
    proceso_t* current = head;
    while (current != NULL) {
        printf("[%d] [Tiempo de llegada: %d] [Prioridad: %d] [Tiempo de proceso: %d] [M bytes: %d] [Impresoras: %d] [Escaners: %d] [Modems: %d] [CDs: %d]\n",
               current->id, current->tiempo_llegada, current->prioridad, current->tiempo_proceso, current->m_bytes,
               current->impresoras, current->escaners, current->modems, current->cds);
        current = current->siguiente_proceso;
    }
}


void inicializarMemoria(int s, int tamaño){
    int i;
    for(i = s; i<=tamaño; i++){
        memoria[i] = 0;
    }
}
