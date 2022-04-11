/* cyclecheck.h
 *
 * Check for cycles in (what's meant to be) a dag
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.7 01-Apr-2003 18:52 rob:)
 *
 * Authors:
 *    Peter Hartley <peter@empeg.com>
 */

#ifndef included_cyclecheck_h
#define included_cyclecheck_h

#include <map>
#include <set>
#include <list>

template<typename T> class CycleChecker
{
    typedef std::set<T> set_t;
    typedef std::map<T,set_t> graph_t;

    graph_t graph;
    graph_t weak_edges;

    size_t edges;

 protected:
     bool Search( T, T, std::list<T>&, std::set<T>&, bool weak, bool findstrong );

 public:
    CycleChecker() : edges(0) {}
    ~CycleChecker() {}
  
    // Return values from AddEdge()
    enum {
	ADDEDGE_NEW,
	ADDEDGE_OLD,
	ADDEDGE_LOOPS
    };
  
    // True if introducing this edge would form a cycle (the cycle parameter is
    // written with a list of the T's involved), otherwise the edge is added.
    // Weak edges act like strong ones, except that a cycle doesn't count if it
    // involves following two weak edges in a row.
    int AddEdge(T from,T to, std::list<T>& cycle, bool weak=false);

    void DeleteNode( T );

    void Dump();
    void DumpNamed( const std::map<T,const char*> );

    void Clear() { graph.clear(); }
};

// You need to include cyclecheck2.h for the declarations of AddEdge and Dump

#endif
