# Bank
Robotics team assignment

Requirements: [Asio](https://think-async.com/Asio/) (just the headers)

If you just want to test the client, you don't need to compile the server. Just compile the client and it will connect to my server (206.189.165.91 port 4567) by default.
## How to compile (Windows)
Needs: [Mingw-w64](http://mingw-w64.org/doku.php) with POSIX threads
Just use `install.bat`. Or if you don't need everything:
### Server
```
g++ src/server.cpp -o server.exe -I path/to/asio/include -l ws2_32 -l wsock32
```
### Client
```
g++ src/server.cpp -o server.exe -I path/to/asio/include -l ws2_32 -l wsock32
```
## How to compile (Linux)
Just use `install.sh`. Or if you don't need everything:
### Server
```
g++ src/server.cpp -o server -I path/to/asio/include -l pthread
```
### Client
```
g++ src/client.cpp -o client -I path/to/asio/include -l pthread
```