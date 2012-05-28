#!/usr/bin/env python

import argparse
import os, sys

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = 'Convert line number to ' +
            'instruction ID or vice versa')
    parser.add_argument('bc',
            help = 'the path to the input LLVM bitcode')
    parser.add_argument('loc',
            help = 'file:lineno or insid')
    args = parser.parse_args()

    llvm_prefix = os.popen('llvm-config --prefix').readline().strip()
    cmd = 'opt '
    cmd += '-load ' + llvm_prefix + '/lib/ID.so '
    cmd += '-load ' + llvm_prefix + '/lib/SourceLocator.so '
    cmd += '-locate-src '
    cmd += '-input ' + args.loc + ' '
    cmd += '-disable-output '
    cmd += '< ' + args.bc + ' '

    print >> sys.stderr, cmd
    os.system(cmd)
