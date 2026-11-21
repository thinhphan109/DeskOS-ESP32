#!/usr/bin/env python3
"""Serve a prepared static update directory for local review; never flashes devices."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


class Handler(SimpleHTTPRequestHandler):
    def end_headers(self):
        if self.path.split("?",1)[0].startswith("/channels/"):
            self.send_header("Cache-Control","no-cache, max-age=0, must-revalidate")
        elif self.path.split("?",1)[0].startswith("/releases/"):
            self.send_header("Cache-Control","public, max-age=31536000, immutable")
        else:
            self.send_header("Cache-Control","no-store")
        super().end_headers()

    def list_directory(self,path):
        self.send_error(403,"Directory listing is disabled");return None

    def log_message(self, *_):
        pass


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory",type=Path)
    parser.add_argument("--bind",default="127.0.0.1",help="Loopback by default; choose LAN IP explicitly to test a board later")
    parser.add_argument("--port",type=int,default=8780)
    args=parser.parse_args()
    root=args.directory.resolve()
    if not (root/"channels").is_dir() or not (root/"releases").is_dir():
        parser.error("Expected the publisher's channels/ and releases/ directories")
    handler=partial(Handler,directory=str(root))
    server=ThreadingHTTPServer((args.bind,args.port),handler)
    print(f"mimi.desk staging: http://{args.bind}:{args.port}/channels/preview.json",flush=True)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close()


if __name__=="__main__":main()
