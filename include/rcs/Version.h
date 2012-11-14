#ifndef __RCS_VERSION_H
#define __RCS_VERSION_H

#include "rcs/config.h"

#define LLVM_VERSION(major, minor) (((major) << 8) | (minor))
#define LLVM_VERSION_CODE LLVM_VERSION(LLVM_VERSION_MAJOR, LLVM_VERSION_MINOR)

#endif
