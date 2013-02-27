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
    cmd = load_plugin(cmd, 'libRCSID')
    cmd = load_plugin(cmd, 'libRCSCFG')
    cmd = load_plugin(cmd, 'libRCSPointerAnalysis')
    cmd = load_plugin(cmd, 'RCSSourceLocator')
    cmd = load_plugin(cmd, 'RCSAATester')
    return cmd

def get_linking_flags(prog):
    linking_flags = []
    if prog.startswith('pbzip2'):
        linking_flags.extend(['-pthread', '-lbz2'])
    if prog.startswith('ferret'):
        linking_flags.extend(['-pthread', '-lgsl', '-lblas'])
    if prog.startswith('gpasswd'):
        linking_flags.extend(['-pthread', '-lcrypt'])
    if prog.startswith('cvs'):
        linking_flags.extend(['-pthread', '-lcrypt'])
    if prog.startswith('httpd'):
        linking_flags.extend(['-pthread', '-lcrypt'])
    if prog.startswith('mysqld'):
        linking_flags.extend(['-pthread', '-lcrypt', '-ldl', '-lz'])
    if prog.startswith('wget'):
        linking_flags.extend(['-pthread', '-lrt', '-lgnutls', '-lidn'])
    return linking_flags

def invoke(cmd, exit_on_failure = True):
    sys.stderr.write('\n\033[0;34m')
    print >> sys.stderr, cmd
    sys.stderr.write('\033[m')
    ret = os.WEXITSTATUS(os.system(cmd))
    if exit_on_failure and ret != 0:
        sys.exit(ret)
    return ret
