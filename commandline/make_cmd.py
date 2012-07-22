#!/usr/bin/env python
"""
Reads a file that contains 4 bytes each line, written in hex and comma seaprated.
Using this file, composes a hidtool command line to write this to the faerii
"""

HIDCMD="./hidtool write"

import sys

def main(args):
  inputf = args[0]

  hexbytes = []
  with open(inputf) as f:
    for line in f.readlines():
      line = line.strip()
      hexbytes += line.split(' ')

    try:
      # validate the data
      for hb in hexbytes:
        int(hb,16)
      print HIDCMD,','.join(hexbytes)
      return 0
    except Exception, ex:
      print "non-hex value encountered: ", hb
      return 1

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
