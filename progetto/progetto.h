#ifndef PROGETTO_H
#define PROGETTO_H

// Definizioni globali
#define MD5_LEN 33              // 32 caratteri esadecimali + '\0'
#define MAX_WORD_LEN 256        // Dimensione sicura per ospitare parola + eventuali suffissi

#define MAX_PAIRS_PER_PROC 500  // Massimo di password salvabili da un singolo processo
#define PAIR_STR_LEN 300        // Lunghezza massima per la stringa "hash : password"

#define MAX_LIMIT 100000         // Limite massimo di password da leggere dal dizionario

// Struttura che rappresenta il nostro array di hash
typedef struct {
    char (*hashes)[MD5_LEN];    // Puntatore a un blocco contiguo di stringhe
    int count;                  // Numero totale di hash caricati
} HashDatabase;

// --- Prototipi delle funzioni ---

// Da DBHash.c
HashDatabase load_and_sort_hashes(const char *filename, int max_hashes);
int search_hash(HashDatabase *db, const char *target_hash);
void free_database(HashDatabase *db);

// Da GenHash.c
void compute_md5(const char *str, char output_buffer[33]);

// Da mutazioni.c
void mutate_capitalize(const char *original, char *mutated);
void mutate_uppercase(const char *original, char *mutated);
void mutate_leetspeak(const char *original, char *mutated);
void mutate_suffix(const char *original, const char *suffix, char *mutated);

// Da logica.c
void process_word(const char *word, HashDatabase *db, int *found_original, int *found_mutated, char local_pairs[][PAIR_STR_LEN], int *pair_count);

#endif
