CLIENT = posix_ipc-client
SERVER = posix_ipc-server

CC = gcc
CFLAGS = -Wall -Wextra

all: $(CLIENT) $(SERVER)

$(CLIENT): posix_ipc-client.c
	$(CC) $(CFLAGS) posix_ipc-client.c -o $(CLIENT)

$(SERVER): posix_ipc-server.c
	$(CC) $(CFLAGS) posix_ipc-server.c -o $(SERVER)

clean:
	rm -f $(CLIENT) $(SERVER)
