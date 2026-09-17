// Database degli hash per immagazzinare e cercare i vari hash

#include "progetto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Funzione di comparazione per qsort e bsearch
int compare_hashes(const void *a, const void *b) {
    // Cast dei puntatori void ai tipi corretti (stringhe) e confronto
    return strcmp((const char *)a, (const char *)b);
}

// Funzione per inizializzare il database
HashDatabase load_and_sort_hashes(const char *filename, int max_hashes) {
    HashDatabase db;
    db.count = 0;
    
    // Allocazione dinamica sullo heap di un blocco di memoria contiguo
    db.hashes = malloc(max_hashes * sizeof(char[MD5_LEN]));
    if (db.hashes == NULL) {
        perror("Errore di allocazione memoria per gli hash");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Errore nell'apertura del file hashes.txt");
        free(db.hashes);
        exit(EXIT_FAILURE);
    }

    // Lettura degli hash dal file
    while (db.count < max_hashes && fscanf(file, "%32s", db.hashes[db.count]) == 1) {
        db.count++;
    }
    fclose(file);

    // Ordinamento dell'array (O(N log N)) per prepararlo alla ricerca binaria
    qsort(db.hashes, db.count, sizeof(char[MD5_LEN]), compare_hashes);
    
    //printf("Caricati e ordinati %d hash con successo.\n", db.count);
    return db;
}

// Funzione wrapper per la ricerca
int search_hash(HashDatabase *db, const char *target_hash) {
    // bsearch restituisce un puntatore all'elemento trovato, oppure NULL se non esiste
    void *result = bsearch(target_hash, db->hashes, db->count, sizeof(char[MD5_LEN]), compare_hashes);
    return (result != NULL); // Ritorna 1 (Vero) se trovato, 0 (Falso) altrimenti
}

// Funzione per liberare la memoria a fine programma
void free_database(HashDatabase *db) {
    free(db->hashes);
    db->hashes = NULL;
    db->count = 0;
}
