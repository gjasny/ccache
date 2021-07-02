#!/usr/bin/env python3

import sys
import time
import urllib.request

def run(url, timeout):
    deadline = time.time() + timeout
    req = urllib.request.Request(url, method="HEAD")
    while True:
        try:
            response = urllib.request.urlopen(req)
            print(f"Connection successful (code: {response.getcode()})")
            break
        except urllib.error.URLError as e:
            print(e.reason)
            if time.time() > deadline:
                print(f"All connection attempts failed within {timeout} seconds.")
                sys.exit(1)
            time.sleep(0.5)

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--timeout', '-t', metavar='TIMEOUT',
                        default=10, type=int,
                        help='Maximum seconds to wait for successful connection attempt '
                             '[default: 10 seconds]')
    parser.add_argument('url',
                        type=str,
                        help='URL to connect to')
    args = parser.parse_args()

    run(
        url=args.url,
        timeout=args.timeout,
    )
