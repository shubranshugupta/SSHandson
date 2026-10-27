MYSQL_CFLAGS = $(shell mysql_config --cflags)
MYSQL_LIBS = $(shell mysql_config --libs)

SERVER_MODULES = src/server/database.c src/server/auth.c \
	src/server/users.c src/server/customer.c \
	src/server/employee.c src/server/manager.c src/server/admin.c

.PHONY: all clean

all: bank_client.out bank_server.out bank_init.out

bank_client.out: src/client.c src/common.c src/client/*.c include/client.h include/common.h
	gcc -Iinclude src/client.c src/common.c src/client/*.c -o bank_client.out

bank_server.out: src/server.c src/common.c $(SERVER_MODULES) include/server.h include/common.h
	gcc -Iinclude $(MYSQL_CFLAGS) src/server.c src/common.c $(SERVER_MODULES) -o bank_server.out $(MYSQL_LIBS) -pthread

bank_init.out: src/server/init_admin.c src/server/database.c src/common.c include/server.h include/common.h
	gcc -Iinclude $(MYSQL_CFLAGS) src/server/init_admin.c src/server/database.c src/common.c -o bank_init.out $(MYSQL_LIBS)

clean:
	rm -f bank_client.out bank_server.out bank_init.out