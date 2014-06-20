HTTP Server and Client
=======

An HTTP 1.1 Server and Client coded in C. This is a bare-bones implementation of an HTTP 1.1 server with POSIX sockets. 

<h4>Features:</h4>
<ul>
	<li>Processes HTTP request and responses</li>
	<li>Dynamically loads web pages</li>
	<li>Parses lines from a configuration file located in "etc/cserver/cserver.conf". In retrospect, it probably would have been a good idea to use an existing library like libconfig instead of reinventing the wheel</li>
	<li>Logs access information in "var/log/cserver.log"</li>
</ul>

To build on Linux:<br/>
```
gcc server.c -o bin/server
chmod +x bin/server
./bin/server
```

```
Usage: server
```