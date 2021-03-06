/*
 * write_bb_file.c --- write a list of bad blocks to a FILE *
 *
 * Copyright (C) 1994, 1995 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

#include <stdio.h>

#if EXT2_FLAT_INCLUDES
#include "ext2_fs.h"
#else
#include <linux/ext2_fs.h>
#endif

#include "ext2fs.h"

errcode_t ext2fs_write_bb_FILE(ext2_badblocks_list bb_list,
			       unsigned int flags,
			       FILE *f)
{
	badblocks_iterate	bb_iter;
	blk_t			blk;
	errcode_t		retval;

	retval = ext2fs_badblocks_list_iterate_begin(bb_list, &bb_iter);
	if (retval)
		return retval;

	while (ext2fs_badblocks_list_iterate(bb_iter, &blk)) {
		fprintf(f, "%d\n", blk);
	}
	ext2fs_badblocks_list_iterate_end(bb_iter);
	return 0;
}
