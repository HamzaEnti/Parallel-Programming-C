#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

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

int check_args(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Ús: ./program <tamany_array>\n");
        return -1;
    }

    int total_size = atoi(argv[1]);
    if (total_size <= 0) {
        fprintf(stderr, "Error: el tamany ha de ser positiu.\n");
        return -1;
    }

    if (total_size % 2 != 0) {
        fprintf(stderr, "Error: el tamany %d ha de ser un nombre parell.\n", total_size);
        return -1;
    }
    return total_size;
}

int main(int argc, char** argv) {
    int total_size = check_args(argc, argv);

    struct timespec start, end; // variables per capturar temps (sense MPI)

    int *data = NULL;
    data = (int*)malloc(total_size * sizeof(int));

    printf("DATA - 100 primers\n");
    for (int i = 0; i < total_size; i++) { 
        data[i] = i;  // [0..N-1]
        if (i < 100) printf("%d ", data[i]); // nomes es mostren els 100 primers
    }
    printf("\n\n");

    //Capturar temps inici
    clock_gettime(CLOCK_MONOTONIC, &start); 

    int total_prime_numbers = 0;
    for (int i = 0; i < total_size; i++) {
        if (is_prime(data[i])) total_prime_numbers++;
    }

    //Capturar temps final
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec)
                   + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Temps: %.6f segons\n", elapsed);

    printf("Total numeros primers %d\n", total_prime_numbers);

    return 0;
}
