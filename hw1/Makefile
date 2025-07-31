CFLAGS = -Wall -Werror -Wextra -g

bbcp: bbcp.c
	gcc $(CFLAGS) -o bbcp bbcp.c

test: bbcp
	./testcp.sh -p /home/tym/src/apue/hw1/bbcp

clean:
	rm -f bbcp
