# Définir le compilateur et les options de compilation
CC = gcc
CFLAGS = -Wall -Wextra -pthread

# Définir les fichiers sources et les fichiers objets
SRCS = proxy.c simpleSocketAPI.c
OBJS = $(SRCS:.c=.o)

# Définir la cible par défaut
all: proxy

# Règle pour créer l'exécutable proxy
proxy: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Règle pour compiler les fichiers objets
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Règle pour nettoyer les fichiers objets et l'exécutable
clean:
	rm -f $(OBJS) proxy

# Règle pour exécuter le proxy
run: proxy
	./proxy