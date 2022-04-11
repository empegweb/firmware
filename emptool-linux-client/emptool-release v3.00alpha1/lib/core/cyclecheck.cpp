/* cyclecheck.cpp
 *
 * Check for cycles in (what's meant to be) a dag
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.4 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *    Peter Hartley <peter@empeg.com>
 */

#include "cyclecheck.h"
#include "cyclecheck2.h"

#include <iostream>
#include <algorithm>

// oh hang on, there's nothing here 'cos it's all a template

#ifdef TEST

#include <stdio.h>

CycleChecker<int> cc;

static void test( int from, int to )
{
    std::list<int> vi;

    if ( cc.AddEdge(from,to,vi) )
    {
	printf("Cycle exists, ");
	for ( std::list<int>::iterator i = vi.begin(); i != vi.end(); ++i )
	{
	    printf( "%d->", *i );
	}
	printf( "%d\n", to );
    }
    else
	printf("No cycle\n");
}

int main(void)
{
    test(0,1);
    test(1,2);
    test(0,3);

    cc.Dump();

    test(0,3);

    cc.Dump();

    test(2,1);

    cc.Dump();

    cc.Clear();

    test(0,1);
    test(1,2);
    test(0,3);

    cc.Dump();

    test(3,1);

    cc.Dump();

    test(2,0);

    cc.Dump();

    return 0;
}

#endif

/* eof */
