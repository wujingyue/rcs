# Author: Jingyue

import os
import string

def load_plugin(cmd, plugin):
    llvm_prefix = os.popen('llvm-config --prefix').readline().strip()
    return string.join((cmd, llvm_prefix + '/lib/' + plugin + '.so'))

def load_all_plugins(cmd):
    cmd = load_plugin(cmd, 'RCSID')
    cmd = load_plugin(cmd, 'RCSPointerAnalysis')
    cmd = load_plugin(cmd, 'DynAAAnalyses')
    cmd = load_plugin(cmd, 'DynAACheckers')
    cmd = load_plugin(cmd, 'DynAAInstrumenters')
    return cmd

def invoke(cmd):
    print >> sys.stderr, cmd
    ret = os.system(cmd)
    if ret != 0:
        sys.exit(ret)
