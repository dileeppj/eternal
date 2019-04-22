INC_DIR=/home/r00t/programming_ws/cpp/eternal

all: ser cli

ser: 	eternal_server.c
			gcc eternal_server.c -lrt -o eternal_server

cli:	eternal_client.c
			gcc eternal_client.c -lrt -o eternal_client


clean:
	rm -rf eternal_client eternal_server
