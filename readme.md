# Bank
Robotics team assignment

Requirements: [Asio](https://think-async.com/Asio/) (just the headers)

If you just want to test the client, you don't need to compile the server. Just compile the client and it will connect to my server (206.189.165.91 port 4567) by default.
## How to compile (Windows)
Using: [Mingw-w64](http://mingw-w64.org/doku.php) with POSIX threads
### Server
```
g++ src/server.cpp -o server.exe -I path/to/asio/include -l ws2_32 -l wsock32
```
### Client
```
g++ src/server.cpp -o server.exe -I path/to/asio/include -l ws2_32 -l wsock32
```
## How to compile (Linux)
### Server
```
g++ src/server.cpp -o server -I path/to/asio/include -l pthread
```
### Client
```
g++ src/client.cpp -o client -I path/to/asio/include -l pthread
```