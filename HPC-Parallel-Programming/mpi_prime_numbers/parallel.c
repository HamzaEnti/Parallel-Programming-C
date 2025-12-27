#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int is_prime(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    int limit = (int)sqrt((double)n);
    for (int i = 3; i <= limit; i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int check_args(int argc, char **argv, int rank, int size) {
    if (argc < 2) {
        if (rank == 0)
            fprintf(stderr, "Ús: mpirun -n <num_procs> ./program <tamany_array>\n");
        return -1;
    }

    int total_size = atoi(argv[1]);
    if (total_size <= 0) {
        if (rank == 0)
            fprintf(stderr, "Error: el tamany ha de ser positiu.\n");
        return -1;
    }

    if (total_size % size != 0) {
        if (rank == 0)
            fprintf(stderr, "Error: el tamany %d no és divisible entre %d processos.\n", total_size, size);
        return -1;
    }
    return total_size;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size, token;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int total_size = check_args(argc, argv, rank, size);

    double start, end; // capturar temps

    if (total_size < 0) {
        MPI_Finalize();
        return 1;
    }

    int chunk_size = total_size / size;
    int *recvbuf = (int*)malloc(chunk_size * sizeof(int));
    int *data = NULL;

    // No modificar res d'aixo
    if (rank == 0) {
        data = (int*)malloc(total_size * sizeof(int));
        printf("DATA - 100 primers\n");
        for (int i = 0; i < total_size; i++) { 
            data[i] = i;  // [0..N-1]
            if (i < 100) printf("%d ", data[i]); // nomes es mostren els 100 primers
        }
        printf("\n\n");
    }

    MPI_Barrier(MPI_COMM_WORLD); // 

    if (rank == 0) start = MPI_Wtime(); //Obtenim el temps actual per posteriorment fer el calcul del temps 
                                        //en el que acaba menys el d'ara i  aixi obtenir el temps que ha tardat 
                                        //en executar tot el programa.
    
    // TODO: Repartir feina a tots els processos
    MPI_Scatter(data, chunk_size, MPI_INT, recvbuf, chunk_size, MPI_INT, 0, MPI_COMM_WORLD); //Repartim "data" el qual es l'array total, 
                                                                                             //en troços "chunk_size" per a cada 
                                                                                             //procés incluint el root(procés 0)
    
    int local_prime_count = 0; // Iniciem el contador local de numeros prims en 0
    
    // TODO: Omplir for de la feina a realitzar per cada process
    for (int i = 0; i < chunk_size; i++) {  //Cada procés fa aquest bucle, el qual dura fins arribal al tamany del troç "chunk_size" que hem repartit abans.
        if (is_prime(recvbuf[i]))
            local_prime_count++;
    }

    int total_prime_count = 0;
    MPI_Reduce(&local_prime_count, &total_prime_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) { //TODO: capturem el temps final
        end = MPI_Wtime();
        printf("Temps: %.6f segons\n", end - start);
    }
    
    // RING
    if (rank == 0) {
        printf("Total de nombres primers %d\n", total_prime_count); // resultat numeros primers trobats
        token = 0; //Iniciem el valor token a 0
        if (size > 1) { // si el nomero de processos es major a 1
            MPI_Send(&token, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);  // Enviem el token a al procés 1
            MPI_Recv(&token, 1, MPI_INT, size - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //El procñes 0 espera fins rebre el token del procés "size - 1"(es a dir l'ultim procés)
        }
        printf("Token is %d\n", token);
    } 
    else { //si rank no es 0
        MPI_Recv(&token, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //rep el token del rank anterior
        if (rank % 2 == 0) { //si rank es parell suma a token 2*rank
            token += 2 * rank; 
        } else { // si es imparell li suma rank al token
            token += rank;
        }
        MPI_Send(&token, 1, MPI_INT, (rank + 1) % size, 0, MPI_COMM_WORLD); // Envia token al seguent rank
    }

    free(recvbuf);
    if (rank == 0) free(data);

    MPI_Finalize();
    return 0;
}