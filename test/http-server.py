#!/usr/bin/env python3

# This ia a simple HTTP Server based on the HTTPServer and
# SimpleHTTPRequestHandler. It has been extended with PUT
# and DELETE functionality to store results.
#
# See: https://github.com/python/cpython/blob/main/Lib/http/server.py

from functools import partial
from http import HTTPStatus
from http.server import HTTPServer, SimpleHTTPRequestHandler

import os
import socket
import sys

class PUTEnabledHTTPRequestHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def do_PUT(self):
        path = self.translate_path(self.path)
        try:
            file_length = int(self.headers['Content-Length'])
            with open(path, 'wb') as output_file:
                output_file.write(self.rfile.read(file_length))
            self.send_response(HTTPStatus.CREATED)
            self.end_headers()
        except OSError:
            self.send_error(HTTPStatus.INTERNAL_SERVER_ERROR, "Cannot open file for writing")

    def do_DELETE(self):
        path = self.translate_path(self.path)
        try:
            os.remove(path)
            self.send_response(HTTPStatus.OK)
            self.end_headers()
        except OSError:
            self.send_error(HTTPStatus.INTERNAL_SERVER_ERROR, "Cannot delete file")

def _get_best_family(*address):
    infos = socket.getaddrinfo(
        *address,
        type=socket.SOCK_STREAM,
        flags=socket.AI_PASSIVE,
    )
    family, type, proto, canonname, sockaddr = next(iter(infos))
    return family, sockaddr

def run(HandlerClass, ServerClass, port, bind):
    HandlerClass.protocol_version = "HTTP/1.1"
    ServerClass.address_family, addr = _get_best_family(bind, port)

    with ServerClass(addr, handler_class) as httpd:
        host, port = httpd.socket.getsockname()[:2]
        url_host = f'[{host}]' if ':' in host else host
        print(
            f"Serving HTTP on {host} port {port} "
            f"(http://{url_host}:{port}/) ..."
        )
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nKeyboard interrupt received, exiting.")
            sys.exit(0)

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--bind', '-b', metavar='ADDRESS',
                        help='Specify alternate bind address '
                             '[default: all interfaces]')
    parser.add_argument('port', action='store',
                        default=8080, type=int,
                        nargs='?',
                        help='Specify alternate port [default: 8080]')
    args = parser.parse_args()

    handler_class = partial(PUTEnabledHTTPRequestHandler)

    run(
        HandlerClass=PUTEnabledHTTPRequestHandler,
        ServerClass=HTTPServer,
        port=args.port,
        bind=args.bind,
    )