# Program for test UDP Hole Punching

## Build

```
mkdir build
cd build
cmake ..
make
```

## Programs

server:  
Used for exchanging ip/port infomation between clients.

client:  
The Peer to try Punching Hole with each other.

check_server:__
Just for client to send massages, and print the client's ip/port infomation. You can use it or not.

## Usage

* Server

On a public host, run ./server

Optionally, you can run ./check_server on host you like.

* Client

You need run exact 2 Clients which are best on different hosts between different NATs.

run:

```
./pr_client <server_ip> [check_server_ip] [client_port]

```

check_server_ip and client_port both are optional. But if you really want to specify client_port, then the check_server_ip is also needed, you can modify the code to change this limit. 
