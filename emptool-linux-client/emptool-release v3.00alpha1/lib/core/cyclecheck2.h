/* cyclecheck2.h
 *
 * Extra templated function declarations for cyclecheck.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *    Peter Hartley <peter@empeg.com>
 */

#ifndef included_cyclecheck2_h
#define included_cyclecheck2_h

#ifndef included_cyclecheck_h
#include "cyclecheck.h"
#endif

#ifndef TRACE_H
#include "trace.h"
#endif

#include <list>
#include <iostream>

template<typename T> int CycleChecker<T>::AddEdge( T from, T to,
						   std::list<T>& result,
						   bool weak )
{
    // Quick check for already present (if a weak edge is already present as
    // a strong edge there's no point in adding it)
    const set_t& children = graph[from];
    if ( children.find( to ) != children.end() )
	return ADDEDGE_OLD;

    if ( weak )
    {
	const set_t& children2 = weak_edges[from];

	if ( children2.find( to ) != children2.end() )
	    return ADDEDGE_OLD;
    }

    set_t seen;

    // Is there a path from 'to' to 'from'? If so, introducing this edge would
    // cause a cycle.
    if ( Search( to, from, result, seen, weak, weak ) )
    {
	//cout << "Visit (TL " << to << ") returned true\n";
	result.push_front(to);
	return ADDEDGE_LOOPS;
    }

    // Otherwise add the edge
    if ( weak )
	weak_edges[from].insert(to);
    else
	graph[from].insert(to);

    edges++;

    if ( (edges%100) == 0 )
    {
	TRACE( "Now %d edges\n", edges );
//	Dump();
    }

    return ADDEDGE_NEW;
}

template<typename T> bool CycleChecker<T>::Search( T start,
						   T searchfor,
						   std::list<T>& result,
						   std::set<T>& seen,
						   bool weak,
						   bool findstrong )
{
    const set_t& children = graph[start];

    //cout << "Searching " << start << endl;

    seen.insert( start );

    for ( set_t::const_iterator i = children.begin();
	  i != children.end();
	  ++i )
    {
	if ( seen.find( *i ) == seen.end() )
	{
	    if ( *i == searchfor
		 || Search( *i, searchfor, result, seen, false, findstrong ) )
	    {
		//cout << "found " << searchfor << " in " << start << endl;
		result.push_front( *i );
		return true;
	    }
	}
    }

    // Search weak edges, *if* we didn't come in along a weak edge
    if ( !weak )
    {
	const set_t& weak_children = weak_edges[start];

	for ( set_t::const_iterator i = weak_children.begin();
	      i != weak_children.end();
	      ++i )
	{
	    if ( seen.find( *i ) == seen.end() )
	    {
		if ( Search( *i, searchfor, result, seen, true, findstrong )
		     || (!findstrong && *i == searchfor) )
		{
		    result.push_front( *i );
		    return true;
		}
	    }
	}
    }

    return false;
}

template<typename T> void CycleChecker<T>::Dump()
{
    for ( graph_t::iterator i = graph.begin(); i != graph.end(); ++i )
    {
	cout << i->first << ":\n";

	const set_t& s = i->second;
	for ( set_t::const_iterator j = s.begin(); j != s.end(); ++j )
	{
	    cout << "  -> " << *j << "\n";
	}
    }
}

template<typename T>
void CycleChecker<T>::DumpNamed( const std::map<T,const char*> name_map )
{
    for ( graph_t::const_iterator i = graph.begin(); i != graph.end(); ++i )
    {
	std::cout << i->first << "(" << name_map.find(i->first)->second << "):\n";

	const set_t& s = i->second;
	for ( set_t::const_iterator j = s.begin(); j != s.end(); ++j )
	{
	    std::cout << "  " << *j << "(" << name_map.find(*j)->second << ")";
	}
	std::cout << std::endl;
    }

    for ( graph_t::const_iterator i = weak_edges.begin();
	  i != weak_edges.end();
	  ++i )
    {
	std::cout << i->first << "(" << name_map.find(i->first)->second
	     << "), weak edges:\n";

	const set_t& s = i->second;
	for ( set_t::const_iterator j = s.begin(); j != s.end(); ++j )
	{
	    std::cout << "  " << *j << "(" << name_map.find(*j)->second << ")";
	}
	std::cout << std::endl;
    }
}

template<typename T> void CycleChecker<T>::DeleteNode( T node )
{
    graph_t::iterator it = graph.find( node );
    if ( it != graph.end() )
    {
	edges -= it->second.size();
	graph.erase( it );
    }

    it = weak_edges.find( node );
    if ( it != weak_edges.end() )
    {
	edges -= it->second.size();
	weak_edges.erase( it );
    }
    
    for ( graph_t::iterator i = graph.begin(); i != graph.end(); ++i )
    {
	set_t& s = i->second;
	set_t::iterator j = s.find( node );

	if ( j != s.end() )
	{
	    s.erase( j );
	    edges--;
	}
    }

    for ( graph_t::iterator i = weak_edges.begin(); i != weak_edges.end(); ++i )
    {
	set_t& s = i->second;
	set_t::iterator j = s.find( node );

	if ( j != s.end() )
	{
	    s.erase( j );
	    edges--;
	}
    }
}

#endif
