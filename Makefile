# ==========================================
# Makefile per Attacco a Dizionario MPI+OpenMP
# ==========================================

# 1. Scelta del compilatore
CC = mpicc

# 2. Flag di compilazione
CFLAGS = -O3 -fopenmp -Wall -Wextra -Wno-deprecated-declarations

# 3. Flag del linker (Librerie esterne)
LDFLAGS = -lcrypto

# 4. Nomi degli eseguibili finali
TARGET = cracker
TARGET_BONUS = cracker_bonus

# 5. Elenco dei file sorgente
SRCS = main.c hashGen.c hashDB.c logica.c mutazioni.c
SRCS_BONUS = main_bonus.c hashGen.c hashDB.c logica.c mutazioni.c

# Generazione automatica dei nomi dei file oggetto (.o)
OBJS = $(SRCS:.c=.o)
OBJS_BONUS = $(SRCS_BONUS:.c=.o)

# ==========================================
# Regole di compilazione
# ==========================================

# Regola di default eseguita digitando semplicemente "make" (compila solo la versione base)
all: $(TARGET)

# Regola per il link finale (Versione Base)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Regola per il link finale (Versione Bonus con fseek)
bonus: $(OBJS_BONUS)
	$(CC) $(CFLAGS) -o $(TARGET_BONUS) $(OBJS_BONUS) $(LDFLAGS)

# Regola generica per compilare i singoli file .c in file .o
# Se 'progetto.h' viene modificato, tutti i file verranno ricompilati in automatico
%.o: %.c progetto.h
	$(CC) $(CFLAGS) -c $< -o $@

# Regola per pulire la cartella di lavoro digitando "make clean"
clean:
	rm -f *.o $(TARGET) $(TARGET_BONUS)