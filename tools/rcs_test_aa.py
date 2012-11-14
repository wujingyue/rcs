#!/usr/bin/env python

import argparse
import os
import sys
import string
import rcs_utils

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
    parser.add_argument('--debug', action = 'store_true',
            help = 'Set it if you want debug output (default: false)')
    args = parser.parse_args()

    cmd = rcs_utils.load_all_plugins('opt')
    # Some AAs require additional plugins.
    # TODO: Should be specified in a configuration file.
    if args.aa == 'ds-aa':
        cmd = rcs_utils.load_plugin(cmd, 'LLVMDataStructure')
        # cmd = string.join((cmd, '-debug'))
    elif args.aa == 'anders-aa':
        cmd = rcs_utils.load_plugin(cmd, 'RCSAndersens')
    elif args.aa == 'bc2bdd-aa':
        if not os.path.exists('bc2bdd.conf'):
            sys.stderr.write('\033[1;31m')
            print >> sys.stderr, 'Error: bc2bdd-aa requires bc2bdd.conf, ' \
                    'which cannot be found in the current directory.'
            sys.stderr.write('\033[m')
            sys.exit(1)
        cmd = rcs_utils.load_plugin(cmd, 'bc2bdd')

    cmd = string.join((cmd, '-' + args.aa))
    cmd = string.join((cmd, '-test-aa'))
    if not args.ins:
        cmd = string.join((cmd, '-value'))
    if args.debug:
        cmd = string.join((cmd, '-debug'))
    cmd = string.join((cmd, '-id1', str(args.id1)))
    cmd = string.join((cmd, '-id2', str(args.id2)))
    cmd = string.join((cmd, '-disable-output', '<', args.bc))

    rcs_utils.invoke(cmd)
