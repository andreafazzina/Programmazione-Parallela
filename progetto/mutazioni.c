// Implementazione delle diverse regole di mutazione

#include "progetto.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>

// 1. Prima lettera maiuscola (es. password -> Password)
void mutate_capitalize(const char *original, char *mutated) {
    strcpy(mutated, original);
    if (mutated[0] != '\0') {
        // toupper richiede un unsigned char per evitare comportamenti indefiniti
        mutated[0] = toupper((unsigned char)mutated[0]);
    }
}

// 2. Tutto maiuscolo (es. password -> PASSWORD)
void mutate_uppercase(const char *original, char *mutated) {
    int i = 0;
    while (original[i] != '\0') {
        mutated[i] = toupper((unsigned char)original[i]);
        i++;
    }
    mutated[i] = '\0';
}

// 3. Leetspeak base (es. password -> p4ssw0rd)
void mutate_leetspeak(const char *original, char *mutated) {
    int i = 0;
    while (original[i] != '\0') {
        // Convertiamo in minuscolo per semplificare il controllo
        char c = tolower((unsigned char)original[i]);
        switch (c) {
            case 'a': mutated[i] = '4'; break;
            case 'e': mutated[i] = '3'; break;
            case 'i': mutated[i] = '1'; break;
            case 'o': mutated[i] = '0'; break;
            case 's': mutated[i] = '5'; break;
            default:  mutated[i] = original[i]; // Mantieni il carattere originale
        }
        i++;
    }
    mutated[i] = '\0'; // Terminatore di stringa fondamentale
}

// 4. Suffisso numerico (es. password -> password123)
void mutate_suffix(const char *original, const char *suffix, char *mutated) {
    // snprintf è più sicuro di strcat perché tronca in automatico 
    // se superiamo MAX_WORD_LEN, evitando buffer overflow
    snprintf(mutated, MAX_WORD_LEN, "%s%s", original, suffix);
}
