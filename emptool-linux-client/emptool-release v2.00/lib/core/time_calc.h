/* time_calc.h
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 01-Apr-2003 18:52 rob:)
 */

#ifndef TIME_CALC_H
#define TIME_CALC_H

namespace TimeCalc
{
    int Remaining(struct timeval *till);		// return msecs
    void At(struct timeval *result, int offset_ms);	// store in *result
};

#endif
