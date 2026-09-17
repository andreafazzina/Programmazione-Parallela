#include "progetto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>

int main(int argc, char **argv) {
    int rank, size;

    // Inizializzazione di MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

// ---------------------------------------------------------
// FASE 0: Inizializzazione condivisa
// ---------------------------------------------------------

    // Tutti i processi devono caricare in memoria il database degli hash
    // (perché ogni processo dovrà fare ricerche sul database completo)
    HashDatabase db = load_and_sort_hashes("hashes.txt", 1000000);

// ---------------------------------------------------------
// FASE 1: MPI_Scatter (Distribuzione del dizionario)
// ---------------------------------------------------------

    char (*chunk)[MAX_WORD_LEN] = NULL;
    char (*full_dictionary)[MAX_WORD_LEN] = NULL;
    int chunk_size = 0;
    int total_words = 0;

    // Solo il processo root accede al file system per leggere il dizionario
    if (rank == 0) {

        FILE *file = fopen("dictionary.txt", "r");
        if (!file) {
            perror("Errore nell'apertura di dictionary.txt");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        // 1. Allochiamo la memoria per contentere tutte le parole del dizionario (fino a MAX_LIMIT)
        full_dictionary = malloc(MAX_LIMIT * sizeof(char[MAX_WORD_LEN]));
        if (!full_dictionary) {
            perror("Errore allocazione dizionario completo");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // 2. Leggiamo il file e carichiamo i dati in UN SOLO PASSAGGIO
        // Il ciclo si ferma se raggiunge la fine del file OPPURE se tocca MAX_LIMIT
        while (total_words < MAX_LIMIT && fgets(full_dictionary[total_words], MAX_WORD_LEN, file) != NULL) {
            
            // Puliamo l'a capo
            full_dictionary[total_words][strcspn(full_dictionary[total_words], "\r\n")] = '\0';
            
            // Se la riga non è vuota incrementiamo
            if (strlen(full_dictionary[total_words]) > 0) {
                total_words++;
            }
        }
        fclose(file);

        // 3. Calcoliamo quante parole spettano a ciascun processo.
        chunk_size = total_words / size;                    // Numero di parole per processo (arrotondato per difetto)
        int scatter_total_words = chunk_size * size;        // Numero totale di parole che verranno effettivamente distribuite (potrebbe essere < total_words)
        
        printf("Processo ROOT: Caricate %d parole (Limite: %d). Chunk size = %d parole per processo.\n", 
               scatter_total_words, MAX_LIMIT, chunk_size);
    }

    // Il root comunica a TUTTI gli altri processi la dimensione del chunk tramite Broadcast
    // In questo modo, i processi "worker" sanno quanta memoria devono allocare
    MPI_Bcast(&chunk_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Ora TUTTI i processi (incluso il root) allocano il proprio spazio ricevente contiguo
    chunk = malloc(chunk_size * sizeof(char[MAX_WORD_LEN]));
    if (!chunk) {
        perror("Errore allocazione chunk");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Distribuzione dei dati
    // Moltiplichiamo chunk_size * MAX_WORD_LEN : X parole di lungheza Y.
    MPI_Scatter(
        full_dictionary, chunk_size * MAX_WORD_LEN, MPI_CHAR, 
        chunk, chunk_size * MAX_WORD_LEN, MPI_CHAR,           
        0, MPI_COMM_WORLD                                     
    );

    // Il root ha inviato i dati, ora ha il suo 'chunk' personale. 
    // La memoria del dizionario intero non gli serve più!
    if (rank == 0) {
        free(full_dictionary);
        full_dictionary = NULL;
    }

// ---------------------------------------------------------
// FASE 2: OpenMP Parallel For
// ---------------------------------------------------------

    int total_original = 0;
    int total_mutated = 0;
    int local_pair_count = 0;
    
    // calloc alloca la memoria e la imposta a zero (fondamentale per MPI_Gather)
    // local_pairs conterrà le stringhe formattate "hash : password" trovate da questo processo
    char (*local_pairs)[PAIR_STR_LEN] = calloc(MAX_PAIRS_PER_PROC, sizeof(char[PAIR_STR_LEN]));

    double start_time = MPI_Wtime();

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < chunk_size; i++) {
        process_word(chunk[i], &db, &total_original, &total_mutated, local_pairs, &local_pair_count);
    }

    double end_time = MPI_Wtime();
    double local_duration = end_time - start_time;

// ---------------------------------------------------------
// FASE 3: Riduzione dei contatori e del tempo
// ---------------------------------------------------------

    int global_original = 0;
    int global_mutated = 0;
    double max_duration = 0.0;
    
    MPI_Reduce(&total_original, &global_original, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&total_mutated, &global_mutated, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_duration, &max_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        long total_hashes = (long)chunk_size * size * 7;   // numero totale di hash calcolati di tutti i processi
        double throughput = total_hashes / max_duration;

        printf("\n--- METRICHE ---\n");
        printf("Tempo calcolo: %f sec | Hash processati: %ld | Throughput: %.2f hash/sec\n", 
               max_duration, total_hashes, throughput);
        printf("Trovate %d (base) + %d (mutate) = %d password totali.\n", 
               global_original, global_mutated, global_original + global_mutated);
    }

// ---------------------------------------------------------
// FASE 4: MPI_Gather (Raccolta delle stringhe reali)
// ---------------------------------------------------------

    char (*global_pairs)[PAIR_STR_LEN] = NULL;
    
    if (rank == 0) {
        // Il Root prepara un buffer abbastanza grande da ricevere i dati da TUTTI i processi
        global_pairs = calloc(size * MAX_PAIRS_PER_PROC, sizeof(char[PAIR_STR_LEN]));
    }

    // Tutti i processi inviano i loro array a Root
    MPI_Gather(local_pairs, MAX_PAIRS_PER_PROC * PAIR_STR_LEN, MPI_CHAR,
               global_pairs, MAX_PAIRS_PER_PROC * PAIR_STR_LEN, MPI_CHAR,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\n--- PASSWORD RECUPERATE DAL CLUSTER ---\n");
        // Root ispeziona il buffer gigante appena ricevuto
        for (int p = 0; p < size; p++) {
            for (int i = 0; i < MAX_PAIRS_PER_PROC; i++) {
                int idx = p * MAX_PAIRS_PER_PROC + i;
                // Stampiamo solo se il primo carattere non è vuoto
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
    
    MPI_Finalize();
    return 0;
}