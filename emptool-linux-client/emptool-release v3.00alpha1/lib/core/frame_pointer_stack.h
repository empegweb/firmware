/* frame_pointer_stack.cpp
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#ifndef FRAME_POINTER_STACK_H
#define FRAME_POINTER_STACK_H	1

#ifdef ARCH_EMPEG

class FramePointerStack
{
    bool m_valid;
    unsigned m_fp, m_sp;
    unsigned m_function, m_caller;

    bool GetFrameInfo();
    
public:
    FramePointerStack();

    bool Start();
    bool Next();

    unsigned GetFunction() const;
    unsigned GetCaller() const;
};

#endif

#endif
