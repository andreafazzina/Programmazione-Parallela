// Generazione delgi hash MD5

#include "progetto.h"

#include <stdio.h>
#include <string.h>
#include <openssl/md5.h>

// Calcola l'MD5 di una stringa e lo salva in un buffer formattato
void compute_md5(const char *str, char output_buffer[33]) {
    // Array per ospitare i 16 byte grezzi generati dall'algoritmo
    unsigned char digest[MD5_DIGEST_LENGTH]; 
    
    // Funzione crittografica di OpenSSL
    MD5((unsigned const char *)str, strlen(str), digest);
    
    // Conversione dei 16 byte grezzi in 32 caratteri esadecimali leggibili
    for (int i = 0; i < 16; i++) {
        sprintf(&output_buffer[i * 2], "%02x", (unsigned int)digest[i]);
    }
}