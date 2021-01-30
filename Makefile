myweb: myweb.c
	gcc -o myweb myweb.c -lmicrohttpd
install: myweb
	cp -f myweb /usr/local/sbin/
	chown root:root /usr/local/sbin/myweb
	cp -f myweb.service /etc/systemd/system/
	chown root:root /etc/systemd/system/myweb.service
	systemctl daemon-reload
	systemctl restart myweb.service
	cp -rf www /var
	chown -R root:root /var/www



