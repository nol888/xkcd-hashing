#!/usr/bin/env python

import sys
import subprocess
try:
    import http.client as httplib
except ImportError:
    import httplib
try:
    from urllib import urlencode, quote
except ImportError:
    from urllib.parse import urlencode, quote

FAST_SKEIN = './xkcd'
server = 'almamater.xkcd.com'
node_name = 'berkeley.edu'

if __name__ == '__main__':
    proc = subprocess.Popen([FAST_SKEIN], stdout=subprocess.PIPE, bufsize=1)

    print("cracking.")

    while True:
        num, text = proc.stdout.readline().decode('utf-8').strip().split(' ')
        num = int(num)
        print(str(num) + ' ' + text)
        try:
            h = httplib.HTTPConnection(server)
            h.request('POST', '/?edu=' + quote(node_name), urlencode({'hashable': text}), {}) 
            h.getresponse().read()
            h.close()
        except Exception as e:
            print("!!! submission failed: " + str(e))