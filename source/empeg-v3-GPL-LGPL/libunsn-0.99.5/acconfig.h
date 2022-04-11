#ifndef UNSN_CONFIG_H
#define UNSN_CONFIG_H 1
@TOP@

/* Define to int if not defined by headers. */
#undef gid_t

/* Define to uint32_t if not defined by headers. */
#undef in_addr_t

/* Define to uint16_t if not defined by headers. */
#undef in_port_t

/* Define to char if not defined by headers. */
#undef int8_t

/* Define to short if not defined by headers. */
#undef int16_t

/* Define to long if not defined by headers. */
#undef int32_t

/* Define to int if not defined by headers. */
#undef pid_t

/* Define to uint16_t if not defined by headers. */
#undef sa_family_t

/* Define to unsigned if not defined by headers. */
#undef size_t

/* Define appropriately if not defined by headers. */
#undef socklen_t

/* Define to int if not defined by headers. */
#undef ssize_t

/* Define to int if not defined by headers. */
#undef uid_t

/* Define to u_int8_t if not defined by headers. */
#undef uint8_t

/* Define to u_int16_t if not defined by headers. */
#undef uint16_t

/* Define to u_int32_t if not defined by headers. */
#undef uint32_t

/* Define to unsigned char if not defined by headers. */
#undef u_int8_t

/* Define to unsigned short if not defined by headers. */
#undef u_int16_t

/* Define to unsigned long if not defined by headers. */
#undef u_int32_t

/* Define if you have struct hostent. */
#undef HAVE_STRUCT_HOSTENT

/* Define if you have struct in_addr. */
#undef HAVE_STRUCT_IN_ADDR

/* Define if you have struct in6_addr. */
#undef HAVE_STRUCT_IN6_ADDR

/* Define if you have struct sigaction. */
#undef HAVE_STRUCT_SIGACTION

/* Define if you have struct sockaddr. */
#undef HAVE_STRUCT_SOCKADDR

/* Define if struct sockaddr has a member named sa_len. */
#undef HAVE_STRUCTMEM_SOCKADDR_SA_LEN

/* Define if you have struct sockaddr_in. */
#undef HAVE_STRUCT_SOCKADDR_IN

/* Define if you have struct sockaddr_in6. */
#undef HAVE_STRUCT_SOCKADDR_IN6

/* Define if struct sockaddr_in6 has a member named sin6_scope_id. */
#undef HAVE_STRUCTMEM_SOCKADDR_IN6_SIN6_SCOPE_ID

/* Define if you have struct sockaddr_un. */
#undef HAVE_STRUCT_SOCKADDR_UN

/* Define if the basename function is declared by headers. */
#undef HAVE_PROTOTYPE_BASENAME

/* Define if the bind function is declared by headers. */
#undef HAVE_PROTOTYPE_BIND

/* Define if the prototype for bind has a non-const pointer argument. */
#undef BIND_HAS_NONCONST_PROTOTYPE

/* Define if the bsearch function is declared by headers. */
#undef HAVE_PROTOTYPE_BSEARCH

/* Define if the connect function is declared by headers. */
#undef HAVE_PROTOTYPE_CONNECT

/* Define if the prototype for connect has a non-const pointer argument. */
#undef CONNECT_HAS_NONCONST_PROTOTYPE

/* Define if the errno variable is declared by headers. */
#undef HAVE_DECLARATION_ERRNO

/* Define if the exit function is declared by headers. */
#undef HAVE_PROTOTYPE_EXIT

/* Define if the free function is declared by headers. */
#undef HAVE_PROTOTYPE_FREE

/* Define according to the style of the gethostbyaddr_r function. */
#undef FUNC_FLAVOR_GETHOSTBYADDR_R

/* Define according to the style of the gethostbyname_r function. */
#undef FUNC_FLAVOR_GETHOSTBYNAME_R

/* Define according to the style of the gethostbyname2_r function. */
#undef FUNC_FLAVOR_GETHOSTBYNAME2_R

/* Define if the getopt function is declared by headers. */
#undef HAVE_PROTOTYPE_GETOPT

/* Define if the getpeername function is declared by headers. */
#undef HAVE_PROTOTYPE_GETPEERNAME

/* Define according to the style of the getprotobyname_r function. */
#undef FUNC_FLAVOR_GETPROTOBYNAME_R

/* Define according to the style of the getprotobynumber_r function. */
#undef FUNC_FLAVOR_GETPROTOBYNUMBER_R

/* Define according to the style of the getservbyname_r function. */
#undef FUNC_FLAVOR_GETSERVBYNAME_R

/* Define according to the style of the getservbyport_r function. */
#undef FUNC_FLAVOR_GETSERVBYPORT_R

/* Define if the getsockname function is declared by headers. */
#undef HAVE_PROTOTYPE_GETSOCKNAME

/* Define if you have the in6addr_any variable. */
#undef HAVE_VAR_IN6ADDR_ANY

/* Define if the in6addr_any variable is declared by headers. */
#undef HAVE_DECLARATION_IN6ADDR_ANY

/* Define if you have the in6addr_loopback variable. */
#undef HAVE_VAR_IN6ADDR_LOOPBACK

/* Define if the in6addr_loopback variable is declared by headers. */
#undef HAVE_DECLARATION_IN6ADDR_LOOPBACK

/* Define if the malloc function is declared by headers. */
#undef HAVE_PROTOTYPE_MALLOC

/* Define if the memchr function is declared by headers. */
#undef HAVE_PROTOTYPE_MEMCHR

/* Define if the memcpy function is declared by headers. */
#undef HAVE_PROTOTYPE_MEMCPY

/* Define if the memmove function is declared by headers. */
#undef HAVE_PROTOTYPE_MEMMOVE

/* Define if the optarg variable is declared by headers. */
#undef HAVE_DECLARATION_OPTARG

/* Define if the opterr variable is declared by headers. */
#undef HAVE_DECLARATION_OPTERR

/* Define if the optind variable is declared by headers. */
#undef HAVE_DECLARATION_OPTIND

/* Define if the optopt variable is declared by headers. */
#undef HAVE_DECLARATION_OPTOPT

/* Define if the poll function is declared by headers. */
#undef HAVE_PROTOTYPE_POLL

/* Define if the qsort function is declared by headers. */
#undef HAVE_PROTOTYPE_QSORT

/* Define if the realloc function is declared by headers. */
#undef HAVE_PROTOTYPE_REALLOC

/* Define if the setregid function is declared by headers. */
#undef HAVE_PROTOTYPE_SETREGID

/* Define if the setresgid function is declared by headers. */
#undef HAVE_PROTOTYPE_SETRESGID

/* Define if the setreuid function is declared by headers. */
#undef HAVE_PROTOTYPE_SETREUID

/* Define if the setresuid function is declared by headers. */
#undef HAVE_PROTOTYPE_SETRESUID

/* Define if the strchr function is declared by headers. */
#undef HAVE_PROTOTYPE_STRCHR

/* Define if the strcmp function is declared by headers. */
#undef HAVE_PROTOTYPE_STRCMP

/* Define if the strcpy function is declared by headers. */
#undef HAVE_PROTOTYPE_STRCPY

/* Define if the strerror function is declared by headers. */
#undef HAVE_PROTOTYPE_STRERROR

/* Define if the strrchr function is declared by headers. */
#undef HAVE_PROTOTYPE_STRRCHR

/* Define if the strspn function is declared by headers. */
#undef HAVE_PROTOTYPE_STRSPN

/* Define if the strtoul function is declared by headers. */
#undef HAVE_PROTOTYPE_STRTOUL

/* Define if the wait3 function is declared by headers. */
#undef HAVE_PROTOTYPE_WAIT3

/* Define if the wait4 function is declared by headers. */
#undef HAVE_PROTOTYPE_WAIT4

/* Define if the waitpid function is declared by headers. */
#undef HAVE_PROTOTYPE_WAITPID

@BOTTOM@

#endif /* !UNSN_CONFIG_H */
