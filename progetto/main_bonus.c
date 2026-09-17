#include "progetto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>

int main(int argc, char **argv) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

// ---------------------------------------------------------
// FASE 0: Inizializzazione condivisa
// ---------------------------------------------------------

    HashDatabase db = load_and_sort_hashes("hashes.txt", 1000000);

// ---------------------------------------------------------
// FASE 1 (BONUS): Lettura Parallela Indipendente (fseek)
// ---------------------------------------------------------

    long logical_file_size = 0;

    // Solo il root trova il byte esatto in cui cade la parola numero MAX_LIMIT
    if (rank == 0) {
        FILE *file = fopen("dictionary.txt", "r");
        if (!file) {
            perror("Errore apertura dictionary.txt");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        int temp_count = 0;
        char temp_buffer[MAX_WORD_LEN];
        
        // Leggiamo finché non raggiungiamo il muro del MAX_LIMIT
        while (temp_count < MAX_LIMIT && fgets(temp_buffer, MAX_WORD_LEN, file) != NULL) {
            temp_buffer[strcspn(temp_buffer, "\r\n")] = '\0';
            if (strlen(temp_buffer) > 0) {
                temp_count++;
            }
        }
        
        // Salviamo il byte esatto in cui finisce la parola numero 100.000
        logical_file_size = ftell(file);
        fclose(file);
    }

    // Broadcast della dimensione LOGICA in byte a tutti i processi
    MPI_Bcast(&logical_file_size, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    // Calcolo dei limiti logici IN BYTE per il partizionamento indipendente
    long chunk_bytes = logical_file_size / size;
    long start_byte = rank * chunk_bytes;
    // L'ultimo rank si ferma esattamente al byte della centomillesima parola
    long end_byte = (rank == size - 1) ? logical_file_size : start_byte + chunk_bytes;

    // Allocazione del buffer locale
    char (*chunk)[MAX_WORD_LEN] = malloc((end_byte - start_byte + 512) * sizeof(char[MAX_WORD_LEN]));
    if (!chunk) {
        perror("Errore allocazione chunk locale");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    int local_chunk_size = 0;

    FILE *file = fopen("dictionary.txt", "r");
    if (!file) {
        perror("Errore apertura dictionary.txt dai worker");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Salto diretto al byte di partenza assegnato
    fseek(file, start_byte, SEEK_SET); 

    // RIALINEAMENTO SUI CONFINI DI MEMORIA (Look-back)
    // (Nota: Nello screenshot precedente usavi ancora il vecchio while, assicurati di usare questo!)
    if (rank != 0) {
        // Torniamo indietro di 1 singolo byte per vedere dove siamo atterrati
        fseek(file, start_byte - 1, SEEK_SET);
        char c = fgetc(file);
        
        // Se non eravamo esattamente dopo un 'a capo', scartiamo la parola a metà
        if (c != '\n') {
            while ((c = fgetc(file)) != '\n' && c != EOF);
        }
    }

    // LETTURA ESCLUSIVA CON FGETS
    while (ftell(file) < end_byte) {
        if (fgets(chunk[local_chunk_size], MAX_WORD_LEN, file) != NULL) {
            chunk[local_chunk_size][strcspn(chunk[local_chunk_size], "\r\n")] = '\0';
            if (strlen(chunk[local_chunk_size]) > 0) {
                local_chunk_size++;
            }
        } else {
            break;
        }
    }
    fclose(file);

    // Calcolo del numero totale di parole lette da tutti i processi
    int global_total_words = 0;
    
    // Sommiamo le parole lette da ciascun worker per ottenere il totale complessivo
    MPI_Reduce(&local_chunk_size, &global_total_words, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {

        printf("Processo ROOT: Caricate %d parole totali (Limite: %d). Chunk medio = %d parole per processo.\n", 
               global_total_words, MAX_LIMIT, global_total_words / size);
    }

    MPI_Barrier(MPI_COMM_WORLD);

// ---------------------------------------------------------
// FASE 2: OpenMP Parallel For
// ---------------------------------------------------------

    int total_original = 0;
    int total_mutated = 0;
    int local_pair_count = 0;
    
    char (*local_pairs)[PAIR_STR_LEN] = calloc(MAX_PAIRS_PER_PROC, sizeof(char[PAIR_STR_LEN]));

    double start_time = MPI_Wtime();

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < local_chunk_size; i++) {
        process_word(chunk[i], &db, &total_original, &total_mutated, local_pairs, &local_pair_count);
    }

    double end_time = MPI_Wtime();
    double local_duration = end_time - start_time;

// ---------------------------------------------------------
// FASE 3: Riduzione e Metriche
// ---------------------------------------------------------

    int global_original = 0;
    int global_mutated = 0;
    double max_duration = 0.0;
    
    MPI_Reduce(&total_original, &global_original, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&total_mutated, &global_mutated, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_duration, &max_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Conteggio effettivo del carico di calcolo distribuito
    long local_hashes = (long)local_chunk_size * 7; 
    long global_hashes = 0;
    MPI_Reduce(&local_hashes, &global_hashes, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double throughput = global_hashes / max_duration;

        printf("\n--- METRICHE (VERSIONE BONUS - I/O PARALLELO) ---\n");
        printf("Tempo calcolo max: %f sec | Hash eseguiti totali: %ld | Throughput: %.2f hash/sec\n", 
               max_duration, global_hashes, throughput);
        printf("Trovate %d (base) + %d (mutate) = %d password totali.\n", 
               global_original, global_mutated, global_original + global_mutated);
    }

// ---------------------------------------------------------
// FASE 4: MPI_Gather (Ricomposizione finale)
// ---------------------------------------------------------

    char (*global_pairs)[PAIR_STR_LEN] = NULL;
    
    if (rank == 0) {
        global_pairs = calloc(size * MAX_PAIRS_PER_PROC, sizeof(char[PAIR_STR_LEN]));
    }

    MPI_Gather(local_pairs, MAX_PAIRS_PER_PROC * PAIR_STR_LEN, MPI_CHAR,
               global_pairs, MAX_PAIRS_PER_PROC * PAIR_STR_LEN, MPI_CHAR,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\n--- PASSWORD RECUPERATE DAL CLUSTER ---\n");
        for (int p = 0; p < size; p++) {
            for (int i = 0; i < MAX_PAIRS_PER_PROC; i++) {
                int idx = p * MAX_PAIRS_PER_PROC + i;
                if (global_pairs[idx][0] != '\0') {
                    printf("[Rank %d] %s\n", p, global_pairs[idx]);
                }
            }
        }
        printf("---------------------------------------\n\n");
        free(global_pairs);
    }

    free(local_pairs);
    free(chunk);
    free_database(&db);
    
    MPI_Finalize();
    return 0;
}
