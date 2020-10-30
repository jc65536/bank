curl -L "https://downloads.sourceforge.net/project/asio/asio/1.18.0%20%28Stable%29/asio-1.18.0.tar.gz" --output asio-1.18.0.tar.gz
tar xzf asio-1.18.0.tar.gz
rm asio-1.18.0.tar.gz
g++ src/server.cpp -o server.exe -I asio-1.18.0/include -l pthread
g++ src/client.cpp -o client.exe -I asio-1.18.0/include -l pthread