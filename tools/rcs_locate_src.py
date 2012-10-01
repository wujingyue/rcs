#!/usr/bin/env python

import argparse
import os, sys
import rcs_utils

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = 'Convert line number to ' +
            'instruction ID or vice versa')
    parser.add_argument('bc',
            help = 'the path to the input LLVM bitcode')
    parser.add_argument('loc',
            help = 'file:lineno, i<ins ID>, or v<value ID>')
    args = parser.parse_args()

    cmd = rcs_utils.load_all_plugins('opt')
    cmd += ' -locate-src'
    cmd += ' -pos ' + args.loc
    cmd += ' -disable-output'
    cmd += ' < ' + args.bc

    rcs_utils.invoke(cmd)
