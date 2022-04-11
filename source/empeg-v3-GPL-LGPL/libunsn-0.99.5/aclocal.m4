dnl
dnl aclocal.m4 -- additional autoconf macros
dnl Copyright (C) 2000  Andrew Main
dnl
dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public
dnl License as published by the Free Software Foundation; either
dnl version 2 of the License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
dnl

dnl zefram_PROTECT_INCLUDES(varname, <headers>)
dnl Generates in $varname text to #include the <headers>.  Any header
dnl whose existence has been tested will be included only if it exists.
AC_DEFUN(zefram_PROTECT_INCLUDES, [
$1=
for zefram_hdr in $2 ; do
	if eval 'test ${ac_cv_header_'`echo $zefram_hdr | sed 'y|/.|__|'`'-yes} = yes'; then
		$1="$$1
#include <$zefram_hdr>"
	fi
done
])

dnl zefram_CHECK_TYPE(typename, default, <headers>)
dnl Looks through the <headers> to determine whether typename is defined.
dnl Defines typename to default if it is not typedefed.
dnl If default is blank, does not define anything.

AC_DEFUN(zefram_CHECK_TYPE, [
AC_MSG_CHECKING(for $1)
AC_CACHE_VAL(zefram_cv_type_defined_$1, [
zefram_PROTECT_INCLUDES(zefram_hdr_text, $3)
AC_EGREP_CPP([[^a-zA-Z_0-9]$1[^a-zA-Z_0-9]], [${zefram_hdr_text}],
zefram_cv_type_defined_$1=yes, zefram_cv_type_defined_$1=no)
])
AC_MSG_RESULT($zefram_cv_type_defined_$1)
ifelse($2, [], [], [dnl
if test $zefram_cv_type_defined_$1 = no; then
	AC_DEFINE($1, $2)
fi
])dnl
])

dnl zefram_CHECK_TYPEHDR(typename, <headers>)
dnl Looks through the <headers> to determine which one defines typename.
dnl Sets $zefram_cv_type_header_typename to the name of the first header
dnl defining it, or "no" if none of them defines it.

AC_DEFUN(zefram_CHECK_TYPEHDR, [
if test ${zefram_cv_type_defined_$1-yes} = yes; then
	AC_MSG_CHECKING(which header defines $1)
	AC_CACHE_VAL(zefram_cv_type_header_$1, [
	for zefram_hdr in $2 no; do
		AC_EGREP_CPP([[^a-zA-Z_0-9]$1[^a-zA-Z_0-9]],
		[#include <$zefram_hdr>], break, :)
	done
	zefram_cv_type_header_$1=$zefram_hdr
	])
	AC_MSG_RESULT($zefram_cv_type_header_$1)
else
	zefram_cv_type_header_$1=no
fi
])

dnl zefram_CHECK_STRUCT(structname, <headers>)
dnl Checks whether struct structname is defined.  <headers> are included
dnl (if they exist) to find the struct.  Defines HAVE_STRUCT_STRUCTNAME
dnl if it exists.

AC_DEFUN(zefram_CHECK_STRUCT, [
AC_MSG_CHECKING(for struct $1)
AC_CACHE_VAL(zefram_cv_struct_$1, [
zefram_PROTECT_INCLUDES(zefram_hdr_text, $2)
AC_TRY_COMPILE([${zefram_hdr_text}], [struct $1 x;],
zefram_cv_struct_$1=yes, zefram_cv_struct_$1=no)
])
AC_MSG_RESULT($zefram_cv_struct_$1)
if test $zefram_cv_struct_$1 = yes; then
	AC_DEFINE(HAVE_STRUCT_[]translit($1, [a-z], [A-Z]))
fi
])

dnl zefram_CHECK_STRUCT_MEMBER(structname, membername, <headers>)
dnl Checks whether struct structname is defined and has a member named
dnl membername.  <headers> are included (if they exist) to define the
dnl struct.  Defines HAVE_STRUCTMEM_STRUCTNAME_MEMBERNAME if the member
dnl exists.

AC_DEFUN(zefram_CHECK_STRUCT_MEMBER, [
if test ${zefram_cv_struct_$1-yes} = yes; then
	AC_MSG_CHECKING(for struct $1 member $2)
	AC_CACHE_VAL(zefram_cv_structmem_$1_$2, [
	zefram_PROTECT_INCLUDES(zefram_hdr_text, $3)
	AC_TRY_COMPILE([${zefram_hdr_text}], [struct $1 x; x.$2;],
	zefram_cv_structmem_$1_$2=yes, zefram_cv_structmem_$1_$2=no)
	])
	AC_MSG_RESULT($zefram_cv_structmem_$1_$2)
	if test $zefram_cv_structmem_$1_$2 = yes; then
		AC_DEFINE(HAVE_STRUCTMEM_[]translit($1_$2, [a-z], [A-Z]))
	fi
else
	zefram_cv_structmem_$1_$2=no
fi
])

dnl zefram_CHECK_VAR(varname, typename, <headers>)
dnl Checks whether the variable varname of type typename exists.
dnl <headers> are included (if they exist) to define the type.
dnl Defines HAVE_VAR_VARNAME if the variable exists.

AC_DEFUN(zefram_CHECK_VAR, [
AC_MSG_CHECKING(for $1)
AC_CACHE_VAL(zefram_cv_var_exists_$1, [
zefram_PROTECT_INCLUDES(zefram_hdr_text, $3)
AC_TRY_LINK([${zefram_hdr_text}], [
extern $2 $1;
puts((char *)&$1);
], zefram_cv_var_exists_$1=yes, zefram_cv_var_exists_$1=no)
])
AC_MSG_RESULT($zefram_cv_var_exists_$1)
if test $zefram_cv_var_exists_$1 = yes; then
	AC_DEFINE(HAVE_VAR_[]translit($1, [a-z], [A-Z]))
fi
])

dnl zefram_CHECK_VAR_DECLARED(varname, <headers>)
dnl Checks whether the varname is declared by the <headers>.
dnl Defines HAVE_DECLARATION_VARNAME if the variable is declared.

AC_DEFUN(zefram_CHECK_VAR_DECLARED, [
AC_MSG_CHECKING(whether $1 is declared)
AC_CACHE_VAL(zefram_cv_var_declared_$1, [
zefram_PROTECT_INCLUDES(zefram_hdr_text, $2)
AC_TRY_COMPILE([${zefram_hdr_text}], [
puts((char *)&$1);
], zefram_cv_var_declared_$1=yes, zefram_cv_var_declared_$1=no)
])
AC_MSG_RESULT($zefram_cv_var_declared_$1)
if test $zefram_cv_var_declared_$1 = yes; then
	AC_DEFINE(HAVE_DECLARATION_[]translit($1, [a-z], [A-Z]))
fi
])

dnl zefram_CHECK_FUNC_DECLARED(funcname, <headers>)
dnl Checks whether the funcname is declared by the <headers>.
dnl Defines HAVE_PROTOTYPE_FUNCNAME if the function is declared.

AC_DEFUN(zefram_CHECK_FUNC_DECLARED, [
AC_MSG_CHECKING(whether $1 is declared)
AC_CACHE_VAL(zefram_cv_func_declared_$1, [
zefram_PROTECT_INCLUDES(zefram_hdr_text, $2)
AC_TRY_COMPILE([${zefram_hdr_text}], [
puts((char *)$1);
], zefram_cv_func_declared_$1=yes, zefram_cv_func_declared_$1=no)
])
AC_MSG_RESULT($zefram_cv_func_declared_$1)
if test $zefram_cv_func_declared_$1 = yes; then
	AC_DEFINE(HAVE_PROTOTYPE_[]translit($1, [a-z], [A-Z]))
fi
])

dnl zefram_CHECK_VAR_DECLARED_IFEXIST(varname, <headers>)
dnl Checks whether the varname is declared by the <headers>.
dnl Defines HAVE_DECLARATION_VARNAME if the variable is declared.
dnl Does nothing if varname doesn't exist.

AC_DEFUN(zefram_CHECK_VAR_DECLARED_IFEXIST, [
if test ${zefram_cv_var_exists_$1-yes} = yes; then
	zefram_CHECK_VAR_DECLARED($1, $2)
fi
])

dnl zefram_CHECK_FUNC_DECLARED_IFEXIST(funcname, <headers>)
dnl Checks whether the funcname is declared by the <headers>.
dnl Defines HAVE_PROTOTYPE_FUNCNAME if the function is declared.
dnl Does nothing if funcname doesn't exist.

AC_DEFUN(zefram_CHECK_FUNC_DECLARED_IFEXIST, [
if test ${ac_cv_func_$1-yes} = yes; then
	zefram_CHECK_FUNC_DECLARED($1, $2)
fi
])

dnl zefram_CHECK_SHLIB
dnl Checks whether shared libraries can be generated.
dnl Sets $zefram_cv_sys_shlib to "yes" if they can, and also sets:
dnl   SHLIB_CFLAGS  additional compilation flags for objects to go in library
dnl   SHLIB_LDFLAGS  additional linker flags to generate shared library
dnl   SHLIB_SONAME_LDFLAG  option to set library soname
dnl   SHLIB_LDRUNPATH_LDFLAG  option to apply $LD_RUN_PATH to link
dnl   SHLIB_EXT  conventional extension for shared libraries

AC_DEFUN(zefram_CHECK_SHLIB, [
AC_MSG_CHECKING(whether shared libraries can be built)
AC_CACHE_VAL(zefram_cv_sys_shlib, [
SHLIB_CFLAGS=
SHLIB_LDFLAGS=
SHLIB_SONAME_LDFLAG=
SHLIB_LDRUNPATH_LDFLAG=
SHLIB_EXT=
zefram_cv_sys_shlib=yes
if test -n "$GCC"; then
	SHLIB_CFLAGS=-fpic
	SHLIB_LDFLAGS=-shared
	if eval "$CC -Wl,--help" 2>&1 | \
			grep 'supported emulations:' >/dev/null; then
		SHLIB_SONAME_LDFLAG=-Wl,-h,
		SHLIB_LDRUNPATH_LDFLAG=
	else
		case $host_cpu-$host_vendor-$host_os in
			*-*-solaris*)
				SHLIB_SONAME_LDFLAG=-Wl,-h,
				SHLIB_LDRUNPATH_LDFLAG=
				;;
			*-*-hpux*)
				SHLIB_SONAME_LDFLAG=-Wl,+h,
				SHLIB_LDRUNPATH_LDFLAG='${LD_RUN_PATH+-Wl,+b,$LD_RUN_PATH}'
				;;
			*)	zefram_cv_sys_shlib=no ;;
		esac
	fi
else
	case $host_cpu-$host_vendor-$host_os in
		*-*-solaris*)
			SHLIB_CFLAGS=-Kpic
			SHLIB_LDFLAGS=-G
			SHLIB_SONAME_LDFLAG='-h ""'
			SHLIB_LDRUNPATH_LDFLAG=
			;;
		*-*-hpux*)
			SHLIB_CFLAGS=+z
			SHLIB_LDFLAGS=-Wl,-b
			SHLIB_SONAME_LDFLAG=-Wl,+h,
			SHLIB_LDRUNPATH_LDFLAG='${LD_RUN_PATH+-Wl,+b,$LD_RUN_PATH}'
			;;
		*)	zefram_cv_sys_shlib=no ;;
	esac
fi
case $host_cpu-$host_vendor-$host_os in
	*-*-hpux*) SHLIB_EXT=sl ;;
	*) SHLIB_EXT=so ;;
esac
if test $zefram_cv_sys_shlib = yes; then
	zefram_SAVE_CFLAGS=$CFLAGS
	zefram_SAVE_LDFLAGS=$LDFLAGS
	CFLAGS="$CFLAGS $SHLIB_CFLAGS"
	LDFLAGS="$LDFLAGS $SHLIB_LDFLAGS ${SHLIB_SONAME_LDFLAG}libconftest.${SHLIB_EXT}.1"
	AC_TRY_LINK([], [], zefram_cv_sys_shlib=yes, zefram_cv_sys_shlib=no)
	CFLAGS=$zefram_SAVE_CFLAGS
	LDFLAGS=$zefram_SAVE_LDFLAGS
fi
zefram_cv_sys_shlib_SHLIB_CFLAGS=$SHLIB_CFLAGS
zefram_cv_sys_shlib_SHLIB_LDFLAGS=$SHLIB_LDFLAGS
zefram_cv_sys_shlib_SHLIB_SONAME_LDFLAG=$SHLIB_SONAME_LDFLAG
zefram_cv_sys_shlib_SHLIB_LDRUNPATH_LDFLAG=$SHLIB_LDRUNPATH_LDFLAG
zefram_cv_sys_shlib_SHLIB_EXT=$SHLIB_EXT
])
AC_MSG_RESULT($zefram_cv_sys_shlib)
SHLIB_CFLAGS=$zefram_cv_sys_shlib_SHLIB_CFLAGS
SHLIB_LDFLAGS=$zefram_cv_sys_shlib_SHLIB_LDFLAGS
SHLIB_SONAME_LDFLAG=$zefram_cv_sys_shlib_SHLIB_SONAME_LDFLAG
SHLIB_LDRUNPATH_LDFLAG=$zefram_cv_sys_shlib_SHLIB_LDRUNPATH_LDFLAG
SHLIB_EXT=$zefram_cv_sys_shlib_SHLIB_EXT
AC_SUBST(SHLIB_CFLAGS)dnl
AC_SUBST(SHLIB_LDFLAGS)dnl
AC_SUBST(SHLIB_SONAME_LDFLAG)dnl
AC_SUBST(SHLIB_LDRUNPATH_LDFLAG)dnl
AC_SUBST(SHLIB_EXT)dnl
])

dnl zefram_CHECK_ASCII
dnl Checks whether the character set is ASCII.
dnl Sets $zefram_cv_sys_ascii to "yes" if it is, "no" if it isn't,
dnl or "hope so" if cross-compiling.

AC_DEFUN(zefram_CHECK_ASCII, [
AC_CACHE_CHECK(whether character set is ASCII-compatible, zefram_cv_sys_ascii,
[AC_TRY_RUN(
[int main()
{
	int n;
	char *chars = (char *)" !\\"#\$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\\\]^_\`abcdefghijklmnopqrstuvwxyz{|}~";
	for(n = 32; *chars; n++, chars++)
		if(*chars != n)
			exit(1);
	exit(0);
}
],
zefram_cv_sys_ascii=yes, zefram_cv_sys_ascii=no,
zefram_cv_sys_ascii='hope so')])
])

dnl zefram_GET_PROTOTYPE(resultvar, funcname, <headers>)
dnl Extracts the prototype of function funcname from <headers>.
dnl Sets $resultvar to the canonical form of the prototype, or to "no"
dnl if no prototype could be found.
dnl Does not output any messages or do any caching.

AC_DEFUN(zefram_GET_PROTOTYPE, [
zefram_PROTECT_INCLUDES(zefram_hdr_text, $3)
echo "$zefram_hdr_text" > conftest.$ac_ext
if eval "$ac_cpp conftest.$ac_ext" > conftest.i 2>&5; then
	sed '/^#/d' < conftest.i | sed -n '
		:1
		/^[[*&a-zA-Z0-9_ 	]]*[[* 	]]$2[[ 	]]*$/{
			N
			s/\n/ /
			b1
		}
		/^[[*&a-zA-Z0-9_ 	]]*[[* 	]]$2[[ 	]]*(/{
			:2
			/[[;{]]/{
				s/[[;{]].*$//
				p
				q
			}
			N
			s/\n/ /
			b2
		}
	' | sed '
		s/\([[^a-zA-Z0-9_]]\)restrict\([[^a-zA-Z0-9_]]\)/\1\2/g
		s/\([[^a-zA-Z0-9_]]\)restrict\([[^a-zA-Z0-9_]]\)/\1\2/g
		s/[[ 	]][[ 	]]*/@/g
		s/\([[a-zA-Z0-9_]]\)@\([[a-zA-Z0-9_]]\)/\1 \2/g
		s/@//g
		s/^__//
		s/\([[^a-zA-Z0-9_]]\)__/\1/g
		s/\([[^a-zA-Z0-9_]]struct \)/\1@/g
		s/\([[^a-zA-Z0-9_]]union \)/\1@/g
		s/\([[^a-zA-Z0-9_]]char\)\([[^a-zA-Z0-9_]]\)/\1@\2/g
		s/\([[^a-zA-Z0-9_]]double\)\([[^a-zA-Z0-9_]]\)/\1@\2/g
		s/\([[^a-zA-Z0-9_]]float\)\([[^a-zA-Z0-9_]]\)/\1@\2/g
		s/\([[^a-zA-Z0-9_]]int\)\([[^a-zA-Z0-9_]]\)/\1@\2/g
		s/\([[^a-zA-Z0-9_]]long\)\([[^a-zA-Z0-9_]]\)/\1@\2/g
		s/\([[^a-zA-Z0-9_]]short\)\([[^a-zA-Z0-9_]]\)/\1@\2/g
		s/\([[^a-zA-Z0-9_]]signed\)\([[^a-zA-Z0-9_]]\)/\1@\2/g
		s/\([[^a-zA-Z0-9_]]unsigned\)\([[^a-zA-Z0-9_]]\)/\1@\2/g
		s/ [[a-zA-Z0-9_]]*\([[,)]]\)/\1/g
		s/\([[*&]]\)[[a-zA-Z0-9_]]*\([[,)]]\)/\1\2/g
		s/@//g
		s/^extern //
		s/^inline //
	' > conftest.pro
	$1="`cat conftest.pro`"
	test -z "$$1" && $1=no
else
	$1=no
fi
rm -f conftest.$ac_ext conftest.i conftest.pro
])

dnl zefram_CHECK_BINDCONNECT_PROTOTYPE(funcname)
dnl Checks the prototype of funcname, which is supposed to have the prototype
dnl of bind()/connect().  If the constness in the prototype is wrong,
dnl define FUNCNAME_HAS_NONCONST_PROTOTYPE.

AC_DEFUN(zefram_CHECK_BINDCONNECT_PROTOTYPE, [
if test ${zefram_cv_func_declared_$1-yes} = yes; then
	AC_MSG_CHECKING(prototype of $1)
	AC_CACHE_VAL(zefram_cv_func_bindconnect_prototype_$1, [
	zefram_GET_PROTOTYPE(zefram_proto, $1, sys/types.h sys/socket.h)
	case "$zefram_proto" in
		no) zefram_proto="cryptic" ;;
		*" const*"* | *",const "*) zefram_proto="good" ;;
		*",struct "*) zefram_proto="broken" ;;
		*) zefram_proto="cryptic" ;;
	esac
	zefram_cv_func_bindconnect_prototype_$1="$zefram_proto"
	])
	AC_MSG_RESULT($zefram_cv_func_bindconnect_prototype_$1)
	if test $zefram_cv_func_bindconnect_prototype_$1 = broken; then
		AC_DEFINE(translit($1, [a-z], [A-Z])_HAS_NONCONST_PROTOTYPE)
	fi
else
	zefram_cv_func_bindconnect_prototype_$1="non-existent"
fi
])

dnl zefram_GUESS_SOCKLEN_T
dnl Work out what socklen_t should be.

AC_DEFUN(zefram_GUESS_SOCKLEN_T, [
if test ${zefram_cv_type_defined_socklen_t-yes} = yes; then
	zefram_cv_type_socklen_t="already defined"
else
	AC_MSG_CHECKING(what socklen_t should be)
	AC_CACHE_VAL(zefram_cv_type_socklen_t, [
	zefram_GET_PROTOTYPE(zefram_proto, getsockname,
		sys/types.h sys/socket.h)
	case "$zefram_proto" in
		*",int*)") zefram_proto="int" ;;
		*",size_t*)") zefram_proto="size_t" ;;
		*) zefram_proto="defaulting to int" ;;
	esac
	zefram_cv_type_socklen_t="$zefram_proto"
	])
	AC_MSG_RESULT($zefram_cv_type_socklen_t)
	case $zefram_cv_type_socklen_t in "already defined") ;; *)
	AC_DEFINE_UNQUOTED(socklen_t, `echo $zefram_cv_type_socklen_t | sed 's/^.* //'`)
	;; esac
fi
])

dnl zefram_CHECK_FUNC_FLAVOR_NETDBR(getXXbyYY)
dnl Check the style of getXXbyYY.

AC_DEFUN(zefram_CHECK_FUNC_FLAVOR_NETDBR, [
AC_MSG_CHECKING(flavor of $1)
AC_CACHE_VAL(zefram_cv_func_flavor_$1, [
zefram_GET_PROTOTYPE(zefram_proto, $1, sys/types.h sys/socket.h netdb.h)
case "$zefram_proto" in
	no) zefram_proto="unknown" ;;
	# GNU
	'int gethostent_r(struct hostent*,char*,size_t,struct hostent**,int*)'|\
	'int gethostbyaddr_r(const char*,size_t,int,struct hostent*,char*,size_t,struct hostent**,int*)'|\
	'int gethostbyaddr_r(const char*,int,int,struct hostent*,char*,size_t,struct hostent**,int*)'|\
	'int gethostbyname_r(const char*,struct hostent*,char*,size_t,struct hostent**,int*)'|\
	'int gethostbyname2_r(const char*,int,struct hostent*,char*,size_t,struct hostent**,int*)'|\
	'int getnetent_r(struct netent*,char*,size_t,struct netent**,int*)'|\
	'int getnetbyaddr_r(unsigned long int,int,struct netent*,char*,size_t,struct netent**,int*)'|\
	'int getnetbyname_r(const char*,struct netent*,char*,size_t,struct netent**,int*)'|\
	'int getservent_r(struct servent*,char*,size_t,struct servent**)'|\
	'int getservbyname_r(const char*,const char*,struct servent*,char*,size_t,struct servent**)'|\
	'int getservbyport_r(int,const char*,struct servent*,char*,size_t,struct servent**)'|\
	'int getprotoent_r(struct protoent*,char*,size_t,struct protoent**)'|\
	'int getprotobyname_r(const char*,struct protoent*,char*,size_t,struct protoent**)'|\
	'int getprotobynumber_r(int,struct protoent*,char*,size_t,struct protoent**)'\
	) zefram_proto="GNU" ;;
	# Solaris
	'struct hostent*gethostent_r(struct hostent*,char*,int,int*)'|\
	'struct hostent*gethostbyaddr_r(const char*,int,int,struct hostent*,char*,int,int*)'|\
	'struct hostent*gethostbyname_r(const char*,struct hostent*,char*,int,int*)'|\
	'struct hostent*gethostbyname2_r(const char*,int,struct hostent*,char*,int,int*)'|\
	'struct netent*getnetent_r(struct netent*,char*,int)'|\
	'struct netent*getnetbyaddr_r(long,int,struct netent*,char*,int)'|\
	'struct netent*getnetbyname_r(const char*,struct netent*,char*,int)'|\
	'struct servent*getservent_r(struct servent*,char*,int)'|\
	'struct servent*getservbyname_r(const char*,const char*,struct servent*,char*,int)'|\
	'struct servent*getservbyport_r(int,const char*,struct servent*,char*,int)'|\
	'struct protoent*getprotoent_r(struct protoent*,char*,int)'|\
	'struct protoent*getprotobyname_r(const char*,struct protoent*,char*,int)'|\
	'struct protoent*getprotobynumber_r(int,struct protoent*,char*,int)'\
	) zefram_proto="Solaris" ;;
	# HP-UX
	'int gethostent_r(struct hostent*,struct hostent_data*)'|\
	'int gethostbyaddr_r(const char*,int,int,struct hostent*,struct hostent_data*)'|\
	'int gethostbyname_r(const char*,struct hostent*,struct hostent_data*)'|\
	'int gethostbyname2_r(const char*,int,struct hostent*,struct hostent_data*)'|\
	'int getnetent_r(struct netent*,struct netent_data*)'|\
	'int getnetbyaddr_r(int,int,struct netent*,struct netent_data*)'|\
	'int getnetbyname_r(const char*,struct netent*,struct netent_data*)'|\
	'int getservent_r(struct servent*,struct servent_data*)'|\
	'int getservbyname_r(const char*,const char*,struct servent*,struct servent_data*)'|\
	'int getservbyport_r(int,const char*,struct servent*,struct servent_data*)'|\
	'int getprotoent_r(struct protoent*,struct protoent_data*)'|\
	'int getprotobyname_r(const char*,struct protoent*,struct protoent_data*)'|\
	'int getprotobynumber_r(int,struct protoent*,struct protoent_data*)'\
	) zefram_proto="HP-UX" ;;
	*) zefram_proto="unrecognized" ;;
esac
zefram_cv_func_flavor_$1="$zefram_proto"
])
AC_MSG_RESULT($zefram_cv_func_flavor_$1)
case $zefram_cv_func_flavor_$1 in
	"GNU") zefram_proto=GNU ;;
	"HP-UX") zefram_proto=HPUX ;;
	"Solaris") zefram_proto=SOLARIS ;;
	*) zefram_proto= ;;
esac
if test -n "$zefram_proto"; then
	AC_DEFINE_UNQUOTED(FUNC_FLAVOR_[]translit($1, [a-z], [A-Z]), FLAVOR_$zefram_proto)
fi
])

dnl zefram_CHECK_FUNC_FLAVOR_NETDBR_IFEXIST(getXXbyYY)
dnl Check the style of getXXbyYY.
dnl Does nothing if getXXbyYY doesn't exist.

AC_DEFUN(zefram_CHECK_FUNC_FLAVOR_NETDBR_IFEXIST, [
if test ${ac_cv_func_$1-yes} = yes; then
	zefram_CHECK_FUNC_FLAVOR_NETDBR($1)
fi
])
