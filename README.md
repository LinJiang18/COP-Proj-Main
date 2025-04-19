# COP5570 Spring 2024 Team Project: P2P Gomoku Game
## Group members: Lin Jiang, Dahai Yu

## Working Condition
This demo is based on UDP protocol, and works only when

1) both of peers are behind `Cone NAT`.
2) if both peers are behind the same NAT, then this NAT must support `loopback transmission` to forward message.

## How to use it ?
1. make.
```
make
```
2. Get the ip of server `linprog1`.
```
hostname -I
```
3. Run the server on `linprog1`.
```
./server [PORT]
```
4. Run peer 1 on `linprog2`. You can use `help` for more information.
```
./client [IP]:[PORT]
```
5. Run peer 2 on `linprog3`.
```
./client [IP]:[PORT]
```
6. List all the peers.
```
list
```
7. Punch.
```
punch [INDEX]
```
8. Chat.
```
send [CONTENT]
```
9. Invite peer to gomoku.
```
game
```
10. set on (x,y).
```
+x,y
```
