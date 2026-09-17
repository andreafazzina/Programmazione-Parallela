#!/bin/bash

# ==========================================
# Script per generare hashes.txt di test (RANDOM)
# ==========================================

DICT_FILE="dictionary.txt"
OUT_FILE="hashes.txt"

# Controlla se il file del dizionario esiste
if [ ! -f "$DICT_FILE" ]; then
    echo "Errore: Il file $DICT_FILE non esiste nella cartella corrente."
    exit 1
fi

echo "Inizio generazione degli hash casuali da $DICT_FILE..."

# Svuota il file di output se esiste già, oppure lo crea
> "$OUT_FILE"

# Estraiamo 20 parole CASUALI da tutto il dizionario
shuf -n 20 "$DICT_FILE" | while IFS= read -r word; do
    
    # Rimuoviamo eventuali caratteri di 'a capo' di Windows (\r)
    clean_word=$(echo "$word" | tr -d '\r')

    # Saltiamo eventuali righe vuote estratte dal random
    if [ -z "$clean_word" ]; then
        continue
    fi

    # 1. Hash della parola originale
    echo -n "$clean_word" | md5sum | awk '{print $1}' >> "$OUT_FILE"

    # 2. Hash della parola con prima lettera maiuscola (Capitalize)
    cap_word="${clean_word^}"
    echo -n "$cap_word" | md5sum | awk '{print $1}' >> "$OUT_FILE"

    # 3. Hash della parola con suffisso '123'
    echo -n "${clean_word}123" | md5sum | awk '{print $1}' >> "$OUT_FILE"

done

# Contiamo quanti hash sono stati generati
NUM_HASHES=$(wc -l < "$OUT_FILE")
echo "Fatto! Sono stati generati $NUM_HASHES hash casuali e salvati in $OUT_FILE."