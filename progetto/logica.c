#include "progetto.h"
#include <stdio.h>
#include <string.h>

// Funzione di supporto per salvare le stringhe e aggiornare i contatori in sicurezza
void register_match(const char *password, const char *hash_str, int is_mutated, int *found_original, int *found_mutated, char local_pairs[][PAIR_STR_LEN], int *pair_count) {
    // Sezione critica: solo un thread alla volta può eseguire questo blocco
    #pragma omp critical
    {
        if (is_mutated) {
            (*found_mutated)++;
        } else {
            (*found_original)++;
        }
        
        // Salviamo la stringa formattata nell'array se c'è ancora spazio
        if (*pair_count < MAX_PAIRS_PER_PROC) {
            snprintf(local_pairs[*pair_count], PAIR_STR_LEN, "%s : %s", hash_str, password);
            (*pair_count)++;
        }
    }
}

void process_word(const char *word, HashDatabase *db, int *found_original, int *found_mutated, char local_pairs[][PAIR_STR_LEN], int *pair_count) {
    char mutated[MAX_WORD_LEN];
    char hash_str[MD5_LEN];
    const char *suffixes[] = {"1", "123", "2026"};
    int num_suffixes = 3;

    // 1. Originale
    compute_md5(word, hash_str);
    if (search_hash(db, hash_str)) 
        register_match(word, hash_str, 0, found_original, found_mutated, local_pairs, pair_count);

    // 2. Prima lettera maiuscola
    mutate_capitalize(word, mutated);
    if (strcmp(word, mutated) != 0) {
        compute_md5(mutated, hash_str);
        if (search_hash(db, hash_str)) 
            register_match(mutated, hash_str, 1, found_original, found_mutated, local_pairs, pair_count);
    }

    // 3. Tutto maiuscolo
    mutate_uppercase(word, mutated);
    if (strcmp(word, mutated) != 0) {
        compute_md5(mutated, hash_str);
        if (search_hash(db, hash_str)) 
            register_match(mutated, hash_str, 1, found_original, found_mutated, local_pairs, pair_count);
    }

    // 4. Leetspeak base
    mutate_leetspeak(word, mutated);
    if (strcmp(word, mutated) != 0) {
        compute_md5(mutated, hash_str);
        if (search_hash(db, hash_str)) 
            register_match(mutated, hash_str, 1, found_original, found_mutated, local_pairs, pair_count);
    }

    // 5. Suffissi numerici
    for (int i = 0; i < num_suffixes; i++) {
        mutate_suffix(word, suffixes[i], mutated);
        compute_md5(mutated, hash_str);
        if (search_hash(db, hash_str)) 
            register_match(mutated, hash_str, 1, found_original, found_mutated, local_pairs, pair_count);
    }
}
