CC = gcc
CFLAGS = -m32 -Wall -Wextra -pedantic
LDFLAGS = -m32 -lpcre

SRC_DIR = src
OBJ_DIR = obj
INC_DIR = inc

CFILES = $(wildcard $(SRC_DIR)/*.c)
OFILES =  $(CFILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

SERVER = ser

all: server

server: $(CFILES) $(SERVER)

$(SERVER): $(OFILES)
	$(CC) $(OFILES) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@ -I$(INC_DIR)

clean:
	rm -f $(OBJ_DIR)/*.o $(SERVER) *~ */*~
