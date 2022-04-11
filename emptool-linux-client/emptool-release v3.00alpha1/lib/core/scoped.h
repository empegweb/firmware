/* scoped.h
 *
 * (C) 2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 *
 * Authors:
 *   Roger Lipscombe <roger@empeg.com>
 *
 */

#ifndef SCOPED_H
#define SCOPED_H 1

/** \page scoped_release Using scoped<T> to manage object lifetime.

Rather than invent a class just so that you can have constructor and
destructor semantics for something out of your jurisdiction (e.g. FILE *) you
can use scoped<T> instead.

For example:

  FILE *f = fopen("foo.txt", "r");
  scoped<FILE *> close_me(f);

  // do something with f.  Don't worry about closing it.

scoped looks (something) like this:

  template <typename Resource_Type,
	    typename Resource_Releaser = scoped_release<Resource_Type> >
  class scoped { ... };

You can use it by either providing a functor as the second template argument,
or by providing a specialisation of scoped_release.

For example:

    template <>
    struct scoped_release<FILE *> {
	void operator() (FILE *f) { fclose(f); }
    };

 */

template <typename Resource_Type>
struct scoped_release
{
};

template <typename Resource_Type,
	  typename Resource_Releaser = scoped_release<Resource_Type> >
class scoped
{
    const Resource_Type &m_resource;

public:
    scoped(const Resource_Type &resource)
	: m_resource(resource) { }

    ~scoped()
    {
	Resource_Releaser()(m_resource);
    }
};

#ifndef WIN32
// This matches any pointer type
template <class T>
struct scoped_release<T*>
{
    void operator() (T *t) { delete t; t = NULL; }
};
#endif

class Connection;

template <>
struct scoped_release<Connection *>
{
    void operator() (Connection *pConnection) { pConnection->Close(); }
};

template <>
struct scoped_release<FILE *>
{
    void operator() (FILE *f) { fclose(f); }
};

#endif /* SCOPED_H */
