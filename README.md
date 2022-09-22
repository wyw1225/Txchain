# Txchain
Txchain, a blockchain service called, where a client issues a request for finding their current amount of txcoins in their account, transfers them to another client and provides a file statement with all the transactions in order. These requests will be sent to a Central Server which in turn interacts with three other backend servers for pulling information and data processing.
https://i.postimg.cc/MHh99rDN/2022-09-22-12-18-22.png
The TCP and UDP base code come from https://beej.us/guide/bgnet/
examples/
TCP uses "client.c" and "server.c", while UDP uses "talker.c" and "listener.c".
