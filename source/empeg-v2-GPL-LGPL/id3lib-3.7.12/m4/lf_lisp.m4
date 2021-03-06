dnl Copyright (C) 1998 Eleftherios Gkioulekas <lf@amath.washington.edu>
dnl  
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software 
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
dnl 
dnl As a special exception to the GNU General Public License, if you 
dnl distribute this file as part of a program that contains a configuration 
dnl script generated by Autoconf, you may include it under the same 
dnl distribution terms that you use for the rest of that program.

AC_DEFUN(LF_PATH_LISPDIR,[
  dnl If set to t, that means we are running in a shell under Emacs.
  dnl If you have an Emacs named "t", then use the full path.
  test "$EMACS" = t && EMACS=
  AC_PATH_PROG(EMACS, emacs xemacs, no)
  if test "$EMACS" != "no" 
  then
    AC_MSG_CHECKING([where .elc files should go])
    dnl Emulate dirname with awk to obtain the directory in which emacs
    dnl was installed
    lf_emacs_prefix=`echo $EMACS | 
                     awk -F/ '{ for (i=1; i<NF; i++) printf("%s/",$i) }' |
                     sed 's/\/bin\///g'`
    dnl Now check whether the site-lisp directory is under 
    dnl lf_emacs_prefix/share/emacs/site-lisp
    dnl lf_emacs_prefix/lib/emacs/site-lisp
    lispdir="${lf_emacs_prefix}/share/emacs/site-lisp"
    AC_MSG_RESULT($lispdir)
  fi
  AC_SUBST(lispdir)
])
