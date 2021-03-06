/* $Id: hesiod.h,v 1.1 1997/09/16 00:16:33 drepper Exp $ */

/*
 * Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef HESIOD__INCLUDED
#define HESIOD__INCLUDED

#include <sys/types.h>
#include <pwd.h>
#include <netdb.h>

/* Application-visible define to signal that we have the new interfaces. */
#define HESIOD_INTERFACES

struct hesiod_postoffice {
  char *hesiod_po_type;
  char *hesiod_po_host;
  char *hesiod_po_name;
};

int hesiod_init(void **context);
void hesiod_end(void *context);
char *hesiod_to_bind(void *context, const char *name, const char *type);
char **hesiod_resolve(void *context, const char *name, const char *type);
void hesiod_free_list(void *context, char **list);
struct passwd *hesiod_getpwnam(void *context, const char *name);
struct passwd *hesiod_getpwuid(void *context, uid_t uid);
void hesiod_free_passwd(void *context, struct passwd *pw);
struct servent *hesiod_getservbyname(void *context, const char *name,
				     const char *proto);
void hesiod_free_servent(void *context, struct servent *serv);
struct hesiod_postoffice *hesiod_getmailhost(void *context, const char *user);
void hesiod_free_postoffice(void *context, struct hesiod_postoffice *po);

/* Compatibility stuff. */

#define HES_ER_UNINIT	-1	/* uninitialized */
#define HES_ER_OK	0	/* no error */
#define HES_ER_NOTFOUND	1	/* Hesiod name not found by server */
#define HES_ER_CONFIG	2	/* local problem (no config file?) */
#define HES_ER_NET	3	/* network problem */

struct hes_postoffice {
  char *po_type;
  char *po_host;
  char *po_name;
};

int hes_init(void);
char *hes_to_bind(const char *name, const char *type);
char **hes_resolve(const char *name, const char *type);
int hes_error(void);
struct passwd *hes_getpwnam(const char *name);
struct passwd *hes_getpwuid(uid_t uid);
struct servent *hes_getservbyname(const char *name, const char *proto);
struct hes_postoffice *hes_getmailhost(const char *name);

#endif
