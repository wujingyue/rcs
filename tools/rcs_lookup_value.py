#!/usr/bin/env python

# Author: Jingyue

import argparse
import os
import sys
import string
import rcs_utils

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = 'Look up value ID by ' + \
                                                   'name or vice versa')
    parser.add_argument('--func', type = str,
                        help = 'function name. Leave it blank if you are ' + \
                                'looking for a global value')
    parser.add_argument('--value', type = str, help = 'value name')
    parser.add_argument('--vid', type = int, help = 'value id')
    parser.add_argument('bc', help = 'the bitcode file')
    args = parser.parse_args()

    cmd = rcs_utils.load_all_plugins('opt')
    cmd = string.join((cmd, '-lookup-id'))
    if not args.vid is None:
        cmd = string.join((cmd, '-value-id', str(args.vid)))
    else:
        assert args.value is not None
        if not args.func is None:
           cmd = string.join((cmd, '-func-name', args.func))
        cmd = string.join((cmd, '-value-name', args.value))
    cmd = string.join((cmd, '-disable-output', '<', args.bc))

    rcs_utils.invoke(cmd)
