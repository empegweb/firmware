/*
 * Lib/errcmp.c -- error number comparison functions
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

#include <libunsn_pr.h>

/* list of error numbers, in order of advancement */
static int const errtable[] = {
#include <errcmp.l.ic>
	0
};

int unsn_errno_cmp(int errnum1, int errnum2)
{
	int e;
	int const *ep;
	/* check for equality */
	if(errnum1 == errnum2)
		return 0;
	/* 0 (no error) compares greater than any error condition */
	if(!errnum1)
		return 1;
	if(!errnum2)
		return -1;
	/* check for known errors */
	for(ep = errtable; e = *ep; ep++) {
		if(e == errnum1)
			return -1;
		else if(e == errnum2)
			return 1;
	}
	/* as a last resort, with two unrecognized errors, consider lower
	   numbers to be more basic errors */
	return errnum1 < errnum2 ? -1 : 1;
}

int unsn_errno_min(int errnum1, int errnum2)
{
	return unsn_errno_cmp(errnum1, errnum2) < 0 ? errnum1 : errnum2;
}

int unsn_errno_max(int errnum1, int errnum2)
{
	return unsn_errno_cmp(errnum1, errnum2) < 0 ? errnum2 : errnum1;
}
