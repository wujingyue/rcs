Build RCS Common Utilities
==========================

Build LLVM 3.0/3.1 and clang 3.0/3.1 from source code.

Put LLVM's installation directory (specified as `--prefix`) in your `PATH`.

Build RCS:

    ./configure --prefix=`llvm-config --prefix`
    make
    make install
