/* frame_pointer_stack.cpp
 *
 * (C) 2002 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.5 13-Mar-2003 18:15 rob:)
 */

#include "config.h"
#include "trace.h"

#ifdef ARCH_EMPEG

#include "frame_pointer_stack.h"

/* APCS frames look like this:
 *
 * { fp, sp, lr, pc }
 *               ^^------ r13 (fp) points here
 *
 * You can trace back to the caller's address and frame by grabbing
 * the fp and lr and recursing.
 */

FramePointerStack::FramePointerStack()
    : m_valid(false),
      m_fp(0),
      m_sp(0),
      m_function(0),
      m_caller(0)
{
}

bool FramePointerStack::Start()
{
    m_valid = false;
    
    unsigned fp, sp;
    __asm__ volatile(" mov %0, r11\n"
		     " mov %1, r13\n"
		     : "=r" (fp), "=r" (sp));
    if(fp & 3)
    {
	TRACE_WARN("Frame pointer is unaligned (0x%08x)\n", fp);
	return false;
    }
    if(sp & 3)
    {
	TRACE_WARN("Stack pointer is unaligned (0x%08x)\n", sp);
	return false;
    }

    // Get the first load of frame info
    m_fp = fp;
    m_sp = sp;
    return GetFrameInfo();
}

bool FramePointerStack::GetFrameInfo()
{
#ifdef ECOS
    // Must be in RAM
    unsigned check_addr = m_fp - 16;
    if(check_addr < 0x1000 || check_addr >= 16 * 1048576)
	return false;
#endif
    // Check frame pointer's idea of "sp" is at m_fp + 4
    unsigned frame_sp = *(const unsigned *) (m_fp - 8);
    if(frame_sp != m_fp + 4)
    {
	TRACE_WARN("Frame sp doesn't agree with frame pointer + 4\n");
	TRACE_WARN("Frame sp = 0x%08x, fp = 0x%08x\n", frame_sp, m_fp);
	m_valid = false;
	return false;
    }
    // Ok, looks good, grab some info from this frame
    m_function = (*(const unsigned *) m_fp) - 16;	// offset by 4
    m_caller = (*(const unsigned *) (m_fp - 4)) - 4;	// offset by 1
    m_valid = true;
    return true;
}

bool FramePointerStack::Next()
{
    ASSERT(m_valid);
    // Recurse to the next frame pointer
    m_fp = *(const unsigned *) (m_fp - 12);
    // Get another load of frame info
    return GetFrameInfo();
}

unsigned FramePointerStack::GetFunction() const
{
    ASSERT(m_valid);
    return m_function;
}
    
unsigned FramePointerStack::GetCaller() const
{
    ASSERT(m_valid);
    return m_caller;
}

#endif

#ifdef TEST
/** @todo Tests */
int main(int ac, char *av[])
{
    return 0;
}
#endif
