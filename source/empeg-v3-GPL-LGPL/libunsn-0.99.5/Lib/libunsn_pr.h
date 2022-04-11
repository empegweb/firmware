/*
 * Lib/libunsn_pr.h -- private interface header for libunsn
 * Copyright (C) 2000  Andrew Main
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef UNSN_LIBUNSN_PR_H
#define UNSN_LIBUNSN_PR_H 1

#include <libunsn.h>
#include <libunsn_f.h>
#include <Compat/ip.h>

/** Character classification strings **/

#define STR_LOWER "abcdefghijklmnopqrstuvwxyz"
#define STR_UPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define STR_ALPHA STR_LOWER STR_UPPER
#define STR_DIGIT "0123456789"
#define STR_ALNUM STR_ALPHA STR_DIGIT
#define STR_GRAPH STR_ALNUM "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"

/** Memory block chains **/

struct chain {
	struct chain *next;
};

#define freechain unsn_private_freechain
void freechain(struct chain * /*ch*/);

/** Length-counted strings **/

struct sstring {
	struct chain chain;
	size_t length;
	char string[1];   /* struct hack; string has a NUL appended */
};

#define cmpsstrings unsn_private_cmpsstrings
int cmpsstrings(struct sstring const * /*s1*/, struct sstring const * /*s2*/);

#define cmpsstrstr unsn_private_cmpsstrstr
int cmpsstrstr(struct sstring const * /*s1*/, char const * /*s2*/);

/** Sequence-following parsed UNSN structures **/

/* Option value alternatives are chained together. */

struct unsn_s_value_alt {
	struct chain chain;
	struct unsn_s_value_alt *alt;
	struct sstring *value;
};

/* Option sequences and alternatives are represented by a network of option
   and option_alt structures.  Conceptually, there is a graph consisting of
   nodes which are sets of option_alt structs (chained through `alt') and
   edges containing option structs.  Edges are linked at both ends through
   `next' members.  Each option sequence begins with a single option_alt
   set and ends with an option_alt whose `next' member is NULL.  Each path
   through this graph is a set of options represented. */

struct unsn_s_option {
	struct chain chain;
	struct unsn_s_option_alt *next;
	struct sstring *name;
	struct unsn_s_value_alt *values;
};

struct unsn_s_option_alt {
	struct chain chain;
	struct unsn_s_option_alt *alt;
	struct unsn_s_option *next;
};

/* Layer sequences are handled similarly to option sequences.  See above. */

struct unsn_s_layer {
	struct chain chain;
	struct unsn_s_layer_alt *next;
	struct sstring *protocol;
	struct unsn_s_option_alt *options;
};

struct unsn_s_layer_alt {
	struct chain chain;
	struct unsn_s_layer_alt *alt;
	struct unsn_s_layer *next;
};

/** alt_iterators **/

/* Iteration context */

struct it_elem {
	int type;
	union {
		struct unsn_s_layer_alt *l;
		struct unsn_s_option_alt *o;
		struct unsn_s_value_alt *v;
	} u;
};

struct unsn_alt_iterator {
	struct chain *chain;
	struct unsn_s_layer_alt *start;
	struct it_elem *endelems;
	struct it_elem elements[1];
};

/* Generate an iterator from a parsed UNSN structure.  On lack of memory,
   returns NULL with errno set to ENOMEM. */

struct unsn_alt_iterator *unsn_private_mkaltiterator(size_t /*length*/,
	struct chain * /*chain*/, struct unsn_s_layer_alt * /*start*/);

/** Single-alternative UNSN structures **/

struct unsn_a_option {
	struct sstring const *name;
	struct sstring const *value;
};

/* Within a layer, the options are sorted by name. */

struct unsn_a_layer {
	struct sstring const *protocol;
	size_t noptions;
	struct unsn_a_option options[1];   /* struct hack */
};

struct unsn_a_layerset {
	size_t nlayers;
	struct unsn_a_layer *layers[1];   /* struct hack */
};

/* Deallocate an alternative structure, not including its strings. */

void unsn_private_freealt(struct unsn_a_layerset * /*a*/);

/* Get a single alternative from a UNSN.  On lack of memory,
   returns NULL with errno set to ENOMEM. */

struct unsn_a_layerset *unsn_private_getalt(
	struct unsn_alt_iterator const * /*i*/);

/** Interpreted protocol layers **/

struct ilayer {
	struct ilayer *next;
	int protocol;
	void (*free)(struct ilayer *);
};

void unsn_private_freeilayers(struct ilayer * /*pl*/);

/* Check a layer for semantic validity, and parse it into ilayer
   structures.  Returns an ilayer structure for the layer.  On error,
   returns NULL, setting errno to indicate the most primitive error that
   makes the layer invalid. */

struct ilayer *unsn_private_interplayer(struct unsn_a_layer const * /*l*/);

/* Check a layer set for semantic validity, and parse it into ilayer
   structures.  Returns ilayer structure.  On error, returns NULL,
   setting errno to indicate the most primitive error that makes the layer
   set invalid. */

struct ilayer *unsn_private_interplayerset(
	struct unsn_a_layerset const * /*ls*/);

/* Specific ilayer type functions */

int unsn_private_ilayer_ip_getprotocol(struct ilayer const * /*il*/);
struct ilayer_ip_iter;
struct ilayer_ip_addr {
	int type;   /* 0 = null, -1 = error, [46] = IPv[46], -[46] = none */
	union {
		int protocol;
		int error;
	} pe;
	union {
		struct in_addr addr4;
		struct in6_addr addr6;
	} addr;
};
struct ilayer_ip_iter *unsn_private_ilayer_ip_mkiter(
	struct ilayer const * /*il*/);
void unsn_private_ilayer_ip_iter_free(struct ilayer_ip_iter * /*i*/);
struct ilayer_ip_addr unsn_private_ilayer_ip_iter_getaddr(
	struct ilayer_ip_iter const * /*i*/);
void unsn_private_ilayer_ip_iter_nullify(struct ilayer_ip_iter * /*i*/);
struct ilayer_ip_addr unsn_private_ilayer_ip_iter_advance(
	struct ilayer_ip_iter * /*i*/);

#ifdef SUPPORT_SOCK_LOCAL
struct sockaddr_un;
void unsn_private_ilayer_local_setsai(struct ilayer const * /*il*/,
	struct unsn_sockaddrinfo * /*sai*/, struct sockaddr_un * /*saun*/);
#endif /* SUPPORT_SOCK_LOCAL */

long unsn_private_ilayer_tcpudp_getport(struct ilayer const * /*il*/);

#ifdef SUPPORT_SOCKETS

/** sai_iterators **/

/* Generate an iterator from an alt_iterator.  On lack of memory,
   returns NULL with errno set to ENOMEM. */

struct unsn_sai_iterator *unsn_private_mksaiiterator(
	struct unsn_alt_iterator * /*ai*/);

/** Socket operations **/

/* Get the socket address structure for the address to which a socket is
   bound or connected.  The address is returned by filling in
   addr_ret->sai_addr and addr_ret->sai_addrlen.  addr_ret must be
   non-null.  On success, zero is returned.  On error, -1 is returned
   and errno set to indicate the error. */

int unsn_private_getspaddr(int /*sock_fd*/,
	int (* /*getname*/)(int /*sock_fd*/,
		struct sockaddr * /*addr*/, socklen_t * /*addrlen*/),
	struct unsn_sockaddrinfo * /*addr_ret*/);

/* Get canonical UNSN for the address to which a socket is bound or connected.
   Some of the socket parameters have to be guessed.  The name is
   returned in malloced storage.  On error, NULL is returned and errno
   set to indicate the error. */

char *unsn_private_getspunsn(int /*sock_fd*/,
	int (* /*getname*/)(int /*sock_fd*/,
		struct sockaddr * /*addr*/, socklen_t * /*addrlen*/),
	unsigned /*opts*/);

/* Get canonical UNSN for the address to which a socket is bound or connected.
   hints->sai_family, hints->sai_type and hints->sai_protocol are used
   to complete the address.  The name is returned in malloced storage.
   On error, NULL is returned and errno set to indicate the error. */

char *unsn_private_getspunsn_withhints(int /*sock_fd*/,
	int (* /*getname*/)(int /*sock_fd*/,
		struct sockaddr * /*addr*/, socklen_t * /*addrlen*/),
	struct unsn_sockaddrinfo const *hints, unsigned /*opts*/);

#endif /* SUPPORT_SOCKETS */

#endif
