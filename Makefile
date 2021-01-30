myweb: myweb.c
	gcc -o myweb myweb.c -lmicrohttpd
install: myweb
	cp -f myweb /usr/local/sbin/
