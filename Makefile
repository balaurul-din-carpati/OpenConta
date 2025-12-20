CC = gcc
# Adaugam -I. pentru a gasi header-ele in folderul curent sau subfoldere
CFLAGS = -Wall -I/usr/include/postgresql -I./apps/api
LIBS = -lpq -pthread

# Lista modulelor
OBJS = apps/api/main.o \
       apps/api/router.o \
       apps/api/routes.o \
       apps/api/response.o \
       apps/api/mod_auth.o \
       apps/api/mod_accounting.o

all: server

server: $(OBJS)
	$(CC) $(CFLAGS) -o apps/api/openconta_server $(OBJS) $(LIBS)
	@echo "--- Compilare Reusita ---"

# Regula generica: Orice .o depinde de .c-ul corespunzator
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f apps/api/*.o apps/api/openconta_server
	@echo "--- Curatenie Efectuata ---"

# Omoara procesele vechi inainte sa porneasca cel nou
run: server
	@echo "--- Restarting Server ---"
	@sudo killall openconta_server 2>/dev/null || true
	./apps/api/openconta_server
