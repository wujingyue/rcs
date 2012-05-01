#!/usr/bin/env python

# Author: Jingyue

import argparse
import os
import sys

def load_plugin(cmd, plugin):
    return ' '.join((cmd, '-load $LLVM_ROOT/install/lib/' + plugin + '.so'))

def get_base_cmd():
    base_cmd = 'opt'
    base_cmd = load_plugin(base_cmd, 'ID')
    base_cmd = load_plugin(base_cmd, 'AATester')
    return base_cmd

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            description = 'Run the specified AA on two values')
    parser.add_argument('bc', help = 'the bitcode of the program')
    aa_choices = ['tbaa', 'basicaa', 'no-aa', 'ds-aa', 'anders-aa', 'bc2bdd-aa']
    parser.add_argument('aa',
            help = 'the underlying alias analysis: ' + str(aa_choices),
            metavar = 'aa',
            choices = aa_choices)
    parser.add_argument('id1', help = 'the first ID', type = int)
    parser.add_argument('id2', help = 'the second ID', type = int)
    parser.add_argument('--ins', action = 'store_true',
            help = 'Set it if <id1> and <id2> are instruction IDs ' \
                    'instead of value IDs (default: false)')
    args = parser.parse_args()

    cmd = get_base_cmd()
    # Some AAs require additional plugins.
    # TODO: Should be specified in a configuration file.
    if args.aa == 'ds-aa':
        cmd = load_plugin(cmd, 'LLVMDataStructure')
    elif args.aa == 'anders-aa':
        cmd = load_plugin(cmd, 'Andersens')
    elif args.aa == 'bc2bdd-aa':
        if not os.path.exists('bc2bdd.conf'):
            sys.stderr.write('\033[1;31m')
            print >> sys.stderr, 'Error: bc2bdd-aa requires bc2bdd.conf, ' \
                    'which cannot be found in the current directory.'
            sys.stderr.write('\033[m')
            sys.exit(1)
        cmd = load_plugin(cmd, 'bc2bdd')

    cmd = ' '.join((cmd, '-' + args.aa))
    cmd = ' '.join((cmd, '-test-aa'))
    if not args.ins:
        cmd = ' '.join((cmd, '-value'))
    cmd = ' '.join((cmd, '-id1', str(args.id1)))
    cmd = ' '.join((cmd, '-id2', str(args.id2)))
    cmd = ' '.join((cmd, '-disable-output', '<', args.bc))

    ret = os.system(cmd)
    sys.exit(ret)
