AC_DEFUN([RCS_COMMON_CONFIG_PROJECT],
  [AC_ARG_WITH([rcscommonsrc],
    AS_HELP_STRING([--with-rcscommonsrc],[Location of RCS Common Source Code]),
    [rcs_common_src="$withval"],[rcs_common_src="]$1["])
  AC_SUBST(RCS_COMMON_SRC,$rcs_common_src)
  AC_ARG_WITH([rcscommonobj],
    AS_HELP_STRING([--with-rcscommonobj],[Location of RCS Common Object Code]),
    [rcs_common_obj="$withval"],[rcs_common_obj="]$2["])
  AC_SUBST(RCS_COMMON_OBJ,$rcs_common_obj)
])
