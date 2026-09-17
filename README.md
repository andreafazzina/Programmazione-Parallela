# Password Cracker Parallelo (MD5)

## 📌 Introduzione
Questo progetto implementa un password cracker ad alte prestazioni in linguaggio C, progettato per decifrare hash MD5 tramite un attacco a dizionario. Il software sfrutta un'architettura ibrida **MPI + OpenMP** per distribuire il carico di calcolo su cluster multi-nodo e massimizzare il throughput sui core locali. 

Il progetto affronta le sfide classiche dell'High Performance Computing (HPC) fornendo due implementazioni distinte:
1. **Versione Standard**: Distribuzione centralizzata del dizionario tramite `MPI_Scatter`.
2. **Versione Bonus**: Architettura scalabile con I/O Parallelo distribuito, partizionamento asimmetrico in byte (`fseek`) e algoritmo di *Look-back* per garantire l'esplorazione del 100% del dizionario eliminando l'overhead di rete sul nodo Root.

Durante l'esecuzione, il motore crittografico genera dinamicamente varianti (mutazioni) per ogni parola letta dal disco, effettuando una ricerca dicotomica super-veloce su un database di hash bersaglio pre-ordinato in RAM.

---

## 📂 Struttura del Progetto

Di seguito l'elenco dei file sorgente e il loro ruolo all'interno dell'architettura:

*   **`main.c`**: Entry-point della versione Standard. Il processo Root (Rank 0) legge l'intero dizionario entro un limite prefissato e lo distribuisce ai worker calcolando partizioni simmetriche tramite `MPI_Scatter`.
*   **`main_bonus.c`**: Entry-point della versione avanzata. I processi aprono simultaneamente il file su disco calcolando quote di partizionamento basate sui byte. Implementa l'algoritmo di riallineamento ai bordi di memoria (*Look-back*) per evitare letture duplicate e saltare l'arrotondamento per difetto.
*   **`logica.c`**: Motore centrale dell'elaborazione. Gestisce i thread worker di OpenMP applicando lo `schedule(dynamic)`, esegue le mutazioni crittografiche, richiama l'hashing e protegge il salvataggio dei risultati tramite direttive `#pragma omp critical`.
*   **`mutazioni.c`**: Modulo dedicato all'espansione del dizionario a runtime. Applica regole deterministiche (Capitalize, Uppercase, Leetspeak, Suffissi numerici) alle stringhe in input per ampliare lo spazio di ricerca in memoria centrale senza penalizzare l'I/O.
*   **`hashGen.c`**: Wrapper per la libreria `OpenSSL`. Calcola il digest binario grezzo (16 byte) dell'algoritmo MD5 e lo converte in formato esadecimale leggibile (32 caratteri).
*   **`hashDB.c`**: Gestisce la struttura dati in RAM per i bersagli. Si occupa dell'allocazione contigua, dell'ordinamento iniziale (`qsort`) e della ricerca dicotomica (`bsearch`) per l'abbattimento dei tempi di confronto.
*   **`progetto.h`**: Header file contenente la definizione delle struct, i limiti hardware costanti e i prototipi di tutte le funzioni.
*   **`generate_hashes.sh`**: Script Bash di utility. Estrae casualmente campioni dal file di dizionario (`dictionary.txt`) e genera la wordlist di bersagli crittografati (`hashes.txt`) utilizzando l'utility nativa `md5sum`.
*   **`Makefile`**: Script per l'automazione del processo di compilazione e per la pulizia dell'ambiente di lavoro.

---

## 🚀 Come Compilare ed Eseguire

### Prerequisiti
Per compilare ed eseguire il progetto è necessario avere installati nel proprio ambiente:
*   Compilatore GCC
*   Libreria OpenMPI
*   Libreria OpenMP
*   Libreria OpenSSL (`libssl-dev`)

### 1. Preparazione dell'ambiente di test
Prima di lanciare il programma, assicurati di avere un file `dictionary.txt` nella directory principale. Successivamente, genera il file degli hash bersaglio (`hashes.txt`) eseguendo lo script Bash:
```bash
chmod +x generate_hashes.sh
./generate_hashes.sh
