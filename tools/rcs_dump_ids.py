#!/usr/bin/env python

import argparse
import os, sys
import rcs_utils

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = 'Dump IDs')
    parser.add_argument('id',
                        help = 'the type of the IDs you want to dump (iid/vid)',
                        choices = ['iid', 'vid'])
    parser.add_argument('bc',
                        help = 'the path to the input LLVM bitcode')
    args = parser.parse_args()

    cmd = rcs_utils.load_all_plugins('opt')
    cmd += ' -assign-id'
    if args.id == 'iid':
        cmd += ' -print-insts'
    elif args.id == 'vid':
        cmd += ' -print-values'
    cmd += ' -analyze'
    cmd += ' < ' + args.bc

    rcs_utils.invoke(cmd)
