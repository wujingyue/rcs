AC_DEFUN([RCS_CONFIG_PROJECT],
  [AC_ARG_WITH([rcssrc],
    AS_HELP_STRING([--with-rcssrc],[Location of RCS Source Code]),
    [rcs_src="$withval"],[rcs_src="]$1["])
  AC_SUBST(RCS_SRC,$rcs_src)
  AC_ARG_WITH([rcsobj],
    AS_HELP_STRING([--with-rcsobj],[Location of RCS Object Code]),
    [rcs_obj="$withval"],[rcs_obj="]$2["])
  AC_SUBST(RCS_OBJ,$rcs_obj)
])
