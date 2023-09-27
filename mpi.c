#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/time.h>

/*Los caracteres que usaremos para crear la cadena
incluye letras mayusculas y minusculas, numeros y algunos
simbolos*/
#define CARACTER_INI 48
#define CARACTER_FIN 122

// Caracter no encontrado: espacio
#define CAR_NE 32

#define MIN_LONG_CAD 70
#define MAX_LONG_CAD 140
#define PESO_COMPROBAR 5000000
#define PESO_GENERAR 10000000

void forzarEspera(unsigned long);
void generarPalabra(int, char *, char *);
char *comprobarCadena(int longitud_c, char *cadenaVerdadera, char *cadenaCandidata);
double mygettime(void);

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int numProcesos, numComprobadores, numGeneradores, i, j;
    int idWorld, flagTipo, palabraEncontrada, flagSalir;
    int miComprobador;
    int *listaComprobadores, *listaGeneradores;
    int tamanoPalabra;

    char *secreta, *pistaRecibida, *generada, *cadena_acertada, *palabra, *c_pista, *c_anterior;

    double tmpIni, tmpFin, tmpTotal;

    int iteraciones = 0;

    MPI_Status status;
    MPI_Request peticionSalir;

    MPI_Comm_size(MPI_COMM_WORLD, &numProcesos);
    MPI_Comm_rank(MPI_COMM_WORLD, &idWorld);

    /*Se inicializa la semilla para generar numeros
    aleatorios con el id del proceso, para que así
    sea diferente en dichos procesos*/
    srand(idWorld + time(0));

    // Creamos un array con los ids de los comprobadores
    numComprobadores = atoi(argv[1]);
    listaComprobadores = (int *)malloc(numComprobadores * sizeof(int));

    /*Se añadiran al comunicador de Comprobadores los numComprobadores
      primeros procesos*/
    for (i = 1; i <= numComprobadores; i++)
    {
        listaComprobadores[i - 1] = i;
    }

    // Creamos un array con generadores
    numGeneradores = numProcesos - numComprobadores - 1;
    listaGeneradores = (int *)malloc(numGeneradores * sizeof(int));

    // El resto de procesos, seran generadores (i ya esta inicializada)
    for (j = 0; j < numGeneradores; j++)
    {
        listaGeneradores[j] = i++;
    }

    // Aqui entra el proceso de E/S
    if (idWorld == 0)
    {
        printf("\nNUMERO DE PROCESOS: Total %d: E/S: 1, Comprobadores: %d, Generadores: %d \n\n", numProcesos, numComprobadores, numGeneradores);
        fflush(stdout);
        printf("NOTIFICACION TIPO\n");
        fflush(stdout);

        // Se genera la palabra de forma aleatoria, incluido su tamano
        // tamanoPalabra = rand()%(MAX_LONG_CAD - MIN_LONG_CAD + 1) + MIN_LONG_CAD;

        tamanoPalabra = 11;
        // cadenaGenerada = (char *)malloc((tamanoPalabra + 1)*sizeof(char));
        secreta = (char *)malloc((tamanoPalabra + 1) * sizeof(char));
        sprintf(secreta, "HOLA_AMIGOS");

        printf("%s", secreta);
        // generarPalabra(tamanoPalabra, cadenaGenerada);

        /*Para comprobadores:
         -> Les notificamos que son comprobadores
         -> Les enviamos el tamano de la cadena a adivinar
         -> Les enviamos la cadena*/
        flagTipo = 1;
        for (i = 0; i < numComprobadores; i++)
        {
            MPI_Send(&flagTipo, 1, MPI_INT, listaComprobadores[i], 0, MPI_COMM_WORLD);
            MPI_Send(&tamanoPalabra, 1, MPI_INT, listaComprobadores[i], 1, MPI_COMM_WORLD);
            MPI_Send(secreta, tamanoPalabra, MPI_CHAR, listaComprobadores[i], 2, MPI_COMM_WORLD);

            // Muestra por pantalla notificacion de tipo
            printf("0%d) 0\n", listaComprobadores[i]);
            fflush(stdout);
        }

        /*Para generadores:
         -> Les notificamos que son generadores
         -> El idWorld de su comprobador correspondiente
         -> Les enviamos el tamano de la cadena a adivinar*/

        flagTipo = 0;
        for (int k = 1, i = 0; i < numGeneradores; i++, k++)
        {
            /*Cuando el contador k supera el número
              de comprobadores vuelve al principio*/
            if (k > numComprobadores)
            {
                k = 1;
            }

            MPI_Send(&flagTipo, 1, MPI_INT, listaGeneradores[i], 0, MPI_COMM_WORLD);

            // OJO CON ESTE QUE EL SIGUIENTE SEND/RECV HA DADO ERRORES
            printf("0%d) %d\n", listaGeneradores[i], k);
            fflush(stdout);

            MPI_Send(&k, 1, MPI_INT, listaGeneradores[i], 1, MPI_COMM_WORLD);
            MPI_Send(&tamanoPalabra, 1, MPI_INT, listaGeneradores[i], 2, MPI_COMM_WORLD);
        }

        printf("\nNOTIFICACION PALABRA COMPROBADORES\n");
        for (i = 1; i < numComprobadores + 1; ++i)
        {
            printf("0%d) %s, %d\n", i, secreta, tamanoPalabra);
            fflush(stdout);
        }

        printf("\nBUSCANDO\n");
        pistaRecibida = (char *)malloc((tamanoPalabra + 1) * sizeof(char));
        flagSalir = 0;
        while (!flagSalir)
        {
            MPI_Recv(pistaRecibida, tamanoPalabra, MPI_CHAR, MPI_ANY_SOURCE, 5, MPI_COMM_WORLD, &status);
            pistaRecibida[tamanoPalabra] = '\0';

            printf("0%d) PISTA......: %s\n", status.MPI_SOURCE, pistaRecibida);
            fflush(stdout);
            if (strcmp(pistaRecibida, secreta) == 0)
            {
                flagSalir = 1;
                printf("\n\n\n\n Ha ganado el proceso %d\n\n\n", status.MPI_SOURCE);
                fflush(stdout);
                for (int i = 1; i < numProcesos; i++)
                {
                    MPI_Send(&flagSalir, 1, MPI_INT, i, 6, MPI_COMM_WORLD);
                    if (i <= numComprobadores)
                    {
                        MPI_Send(pistaRecibida, tamanoPalabra, MPI_CHAR, i, 3, MPI_COMM_WORLD);
                    }
                }
                for (int i = 1; i < numProcesos; i++)
                {
                    MPI_Recv(&tmpTotal, 1, MPI_DOUBLE, i, 7, MPI_COMM_WORLD, &status);
                    MPI_Recv(&iteraciones, 1, MPI_INT, i, 8, MPI_COMM_WORLD, &status);

                    printf("\nProceso %d: %f segundos y %d iteraciones\n", i, tmpTotal, iteraciones);
                    fflush(stdout);
                }
            }
        }

        printf("\nSoy %d y he ACABAO", idWorld);
        fflush(stdout);
    }
    else
    {
        /*Los demas procesos reciben su rol:
         - si flagTipo=1, son comprobadores
         - si flagTipo=0, son generadores*/
        MPI_Recv(&flagTipo, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        /*LAS PALABRAS QUE HAY:ca
            E/S
           -> secreta: palabra secreta que hay que adivinar.
         *&-> pistaRecibida: palabra que envia el generador para ser comprobada. gen -> comp

            COMP
         -> secreta: la palabra que se tiene que adivinar. e/s -> comp
         *+-> generada: la cadena que le manda un generador para ser comprobada.
         *&-> cadena_acertada: caracteres que ha acertado el generador. usada  por comprobador. comp -> gen

         GEN
         *&-> c_pista: caracteres que ha acertado el generador
         *+-> palabra: nunca supe su uso, grande pablo xd*/

        // Aqui entran los comprobadores
        if (flagTipo)
        {
            tmpIni = mygettime();

            generada = (char *)malloc(sizeof(char) * (tamanoPalabra + 1));
            cadena_acertada = (char *)malloc(sizeof(char) * (tamanoPalabra + 1));
            char *secreta = (char *)malloc((tamanoPalabra + 1) * sizeof(char));

            MPI_Recv(&tamanoPalabra, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
            MPI_Recv(secreta, tamanoPalabra, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &status);
            secreta[tamanoPalabra] = '\0';

            /*Se hace una recepción no bloqueante para finalizar
              la búsqueda de palabras una vez que un generador haya
              encontrado la palabra secreta */

            flagSalir = 0;
            MPI_Irecv(&flagSalir, 1, MPI_INT, 0, 6, MPI_COMM_WORLD, &peticionSalir);

            // Sale del bucle si otro proceso encuentra la cadena secreta
            while (!flagSalir)
            {
                // printf("\n COMP despues %d/%d, tamano Palabra %d y palabra %s", idWorld, numProcesos, tamanoPalabra, cadenaGenerada);
                // printf("\nCOMP %d antes de recibir %d el tamano", idWorld, tamanoPalabra);
                // fflush(stdout);

                // Recibe en la cadena propuesta la cadena generada por el generador
                MPI_Recv(generada, tamanoPalabra, MPI_CHAR, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status);
                if (!flagSalir)
                {
                    generada[tamanoPalabra] = '\0';
                    // printf("\nRECV %d esta cadena gen %s", idWorld, generada);
                    // fflush(stdout);

                    // Comprueba la cadena y guarda en la cadena acertado los caracteres coincidentes
                    cadena_acertada = comprobarCadena(tamanoPalabra, secreta, generada);

                    // Envia la cadena al generador con los caracteres que ha acertado
                    MPI_Send(cadena_acertada, tamanoPalabra, MPI_CHAR, status.MPI_SOURCE, 4, MPI_COMM_WORLD);

                    iteraciones++;
                }
                else
                {
                    int p = idWorld;
                    while ((p + numComprobadores) < numProcesos)
                    {
                        p = p + numComprobadores;
                        MPI_Send("H", tamanoPalabra, MPI_CHAR, p, 4, MPI_COMM_WORLD);
                        printf("\n -> Send enviado desde %d hasta %d", idWorld, p);
                        fflush(stdout);
                    }
                }
            }

            tmpFin = mygettime();

            tmpTotal = tmpFin - tmpIni;

            MPI_Send(&tmpTotal, 1, MPI_DOUBLE, 0, 7, MPI_COMM_WORLD);
            MPI_Send(&iteraciones, 1, MPI_INT, 0, 8, MPI_COMM_WORLD);

            printf("\nSoy %d y Termine jeje", idWorld);
            fflush(stdout);
        }

        // Aqui entran los generadores
        if (!flagTipo)
        {

            tmpIni = mygettime();
            MPI_Recv(&miComprobador, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
            MPI_Recv(&tamanoPalabra, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, &status);

            // printf("\n GEN %d/%d Mi Comprob %d, y tam palabra %d", idWorld, numProcesos, miComprobador, tamanoPalabra);
            // fflush(stdout);

            // pablo maricon

            palabra = malloc(sizeof(char) * (tamanoPalabra + 1));

            c_pista = malloc(sizeof(char) * (tamanoPalabra + 1));
            c_anterior = malloc(sizeof(char) * (tamanoPalabra + 1));
            /*En la primera iteración le pasamos como pista
             una cadena de solo espacios para empezar a funcionar*/
            memset(c_pista, ' ', tamanoPalabra);
            c_pista[tamanoPalabra] = '\0';

            flagSalir = 0;
            MPI_Irecv(&flagSalir, 1, MPI_INT, 0, 6, MPI_COMM_WORLD, &peticionSalir);
            int j = 0;
            while (!flagSalir)
            {
                generarPalabra(tamanoPalabra, palabra, c_pista);
                printf("\n GEn %d iteracion %d ANTES", idWorld, j);
                fflush(stdout);
                MPI_Send(palabra, tamanoPalabra, MPI_CHAR, miComprobador, 3, MPI_COMM_WORLD);
                printf("\n GEn %d iteracion %d MEDIO", idWorld, j);
                fflush(stdout);
                strcpy(c_anterior, c_pista);
                MPI_Recv(c_pista, tamanoPalabra, MPI_CHAR, miComprobador, 4, MPI_COMM_WORLD, &status);
                printf("\n GEn %d iteracion %d DESPUES", idWorld, j);
                fflush(stdout);
                j++;

                // printf("\n GEn %d he recibido mi pista %s", idWorld, c_pista);
                // fflush(stdout);

                /* La cadena generada en la ultima iteracion
                   se compara con la de la actual y se manda
                   solo si se ha averiguado una letra nueva*/
                if (strcmp(c_anterior, c_pista) != 0)
                    MPI_Send(c_pista, tamanoPalabra, MPI_CHAR, 0, 5, MPI_COMM_WORLD);

                iteraciones++;
            }
            tmpFin = mygettime();

            tmpTotal = tmpFin - tmpIni;

            printf("Llega aqui");
            fflush(stdout);
            MPI_Send(&tmpTotal, 1, MPI_DOUBLE, 0, 7, MPI_COMM_WORLD);
            MPI_Send(&iteraciones, 1, MPI_INT, 0, 8, MPI_COMM_WORLD);

            printf("\nSoy %d y Termine jeje con %d", idWorld, iteraciones);
            fflush(stdout);
        }
    }

    MPI_Finalize();

    return 0;
}

void generarPalabra(int longitudPalabra, char *palabraAleatoria, char *cadena_pista)
{
    int i;

    for (i = 0; i < longitudPalabra; i++)
    {
        if (cadena_pista[i] == ' ')
        {
            // Se añade un caracter aleatorio a la cadena
            palabraAleatoria[i] = rand() % (CARACTER_FIN - CARACTER_INI + 1) + CARACTER_INI;
        }
        else
        {
            palabraAleatoria[i] = cadena_pista[i];
        }
    }

    palabraAleatoria[i] = '\0';

    forzarEspera(PESO_GENERAR);
}

/*Esta es la funcion que usan los comprobadores para chequear la
palabra que les mandan los generadores. Comprueba la palabra enviada
y retorna los caracteres que coinciden con la cadena verdadera.
Los que no coinciden se marcan con un espacio */
/*const*/ char *comprobarCadena(int longitud_c, char *cadenaVerdadera, char *cadenaCandidata)
{

    char *comprueba = malloc(sizeof(char) * (longitud_c + 1));
    int i;

    for (i = 0; i < longitud_c; ++i)
    {

        if (cadenaVerdadera[i] == cadenaCandidata[i])
            comprueba[i] = cadenaVerdadera[i];
        else
            comprueba[i] = CAR_NE;
    }

    comprueba[longitud_c] = '\0';

    forzarEspera(PESO_COMPROBAR);

    return comprueba;
}

double mygettime(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, 0) < 0)
    {
        perror("oops");
    }
    return (double)tv.tv_sec + (0.000001 * (double)tv.tv_usec);
}

void forzarEspera(unsigned long peso)
{

    unsigned long i;

    for (i = 1; i < peso; i++)
    {
        sqrt(i);
    }
}
