import os
import sys
import string

def get_cmd_result(cmd):
    # TODO: popen is deprecated. Use subprocess.
    return os.popen(cmd).read().strip()

def get_bindir():
    return get_cmd_result('llvm-config --bindir')

def get_libdir():
    return get_cmd_result('llvm-config --libdir')

def load_plugin(cmd, plugin):
    return string.join((cmd, '-load', get_libdir() + '/' + plugin + '.so'))

def load_all_plugins(cmd):
    cmd = load_plugin(cmd, 'RCSID')
    cmd = load_plugin(cmd, 'RCSCFG')
    cmd = load_plugin(cmd, 'RCSPointerAnalysis')
    cmd = load_plugin(cmd, 'RCSSourceLocator')
    cmd = load_plugin(cmd, 'RCSAATester')
    return cmd

def invoke(cmd):
    sys.stderr.write('\n\033[0;34m')
    print >> sys.stderr, cmd
    sys.stderr.write('\033[m')
    ret = os.system(cmd)
    if ret != 0:
        sys.exit(ret)
