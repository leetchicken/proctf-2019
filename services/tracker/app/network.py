import logging
import socket

SOCK_DCCP = 6
IPROTO_DCCP = 33
SOL_DCCP = 269
DCCP_SOCKOPT_SERVICE = 2


def create_server_socket(host: str, port: int):
    server = socket.socket(socket.AF_INET, SOCK_DCCP, IPROTO_DCCP)
    server.setsockopt(SOL_DCCP, DCCP_SOCKOPT_SERVICE, True)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(8)
    server.setblocking(False)

    logging.info("Listening %s:%d ...", host, port)
    return server
