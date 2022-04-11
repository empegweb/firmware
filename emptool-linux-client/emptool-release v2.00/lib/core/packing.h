/* packing.h
 *
 * For when structures need to be byte-packed for compatibility
 *
 * (C) 2001 empeg Ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.6 01-Apr-2003 18:52 rob:)
 */

#ifdef _MSC_VER
 #define USE_PACK_HEADERS 1
 #define PACKED
 #define PACKED_PRE
 #define PACKED_POST
#else
 #define USE_PACK_HEADERS 0
 #ifdef __GNUC__
  #define PACKED_PRE
  #define PACKED_POST __attribute__((packed))
 #else
  #define PACKED_PRE  __packed
  #define PACKED_POST
 #endif
#endif
