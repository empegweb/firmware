/* magic.h
 *
 * Magic number support
 *
 * (C) 2000-2001 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 01-Apr-2003 18:52 rob:)
 *
 * These macros provide a convenient way of adding magic number
 * support to classes. This is a useful way of checking that a pointer
 * (including the this pointer) is actually pointing at a real object
 * and not somewhere into the yonder, at a different object or at an
 * object that has been destructed.
 */

#ifndef MAGIC_H
#define MAGIC_H 1

#define EMPEG_ENABLE_MAGIC (DEBUG>0)

/** Magic support

    Use it like this:

    class Hamster {
	DECLARE_MAGIC(0xfedcba02);

    public:
	Hamster() { INITIALISE_MAGIC(); }
	~Hamster() { INVALIDATE_MAGIC(); }
	void AssertValid() { ASSERT_MAGIC(); }
    };

 */
#if EMPEG_ENABLE_MAGIC
# define DECLARE_MAGIC(Value) \
    enum MagicType { MAGIC_VALUE = (Value), MAGIC_INVERSE = (~(Value)), } m_magic

# define INITIALISE_MAGIC() \
    m_magic = MAGIC_VALUE

# define INVALIDATE_MAGIC() \
    do { ASSERT_MAGIC(); m_magic = MAGIC_INVERSE; } while(0)
# define ASSERT_MAGIC() \
    do { ASSERT_PTR(this); ASSERT_EX(m_magic == MAGIC_VALUE, "magic=%d, expected=%d\n", m_magic, MAGIC_VALUE); } while(0)
#else // non-debug
# define DECLARE_MAGIC(Value)
# define INITIALISE_MAGIC() NOP
# define INVALIDATE_MAGIC() NOP
# define ASSERT_MAGIC() NOP
#endif // DEBUG>2

#endif // MAGIC_H
