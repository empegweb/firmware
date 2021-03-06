/*
 * pass1b.c --- Pass #1b of e2fsck
 *
 * This file contains pass1B, pass1C, and pass1D of e2fsck.  They are
 * only invoked if pass 1 discovered blocks which are in use by more
 * than one inode.
 * 
 * Pass1B scans the data blocks of all the inodes again, generating a
 * complete list of duplicate blocks and which inodes have claimed
 * them.
 *
 * Pass1C does a tree-traversal of the filesystem, to determine the
 * parent directories of these inodes.  This step is necessary so that
 * e2fsck can print out the pathnames of affected inodes.
 *
 * Pass1D is a reconciliation pass.  For each inode with duplicate
 * blocks, the user is prompted if s/he would like to clone the file
 * (so that the file gets a fresh copy of the duplicated blocks) or
 * simply to delete the file.
 * 
 * Copyright (C) 1993, 1994, 1995, 1996, 1997 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 * 
 */

#include <time.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <et/com_err.h>
#include "e2fsck.h"

#include "problem.h"

/*
 * This is structure is allocated for each time that a block is
 * claimed by more than one file.  So if a particular block is claimed
 * by 3 files, then three copies of this structure will be allocated,
 * one for each conflict.
 *
 * The linked list structure is as follows:
 *
 * dup_blk -->  block #34  --> block #35  --> block #47
 * 		inode #12      inode #14      inode #17
 * 		num_bad = 3    num_bad = 2    num_bad = 2
 * 		  |              |               |
 * 		  V              V               V
 * 		block #34      block #35      block #47
 * 		inode #14      inode #15      inode #23
 * 		  |
 * 		  V
 * 		block #34
 * 		inode #15
 *
 * The num_bad field indicates how many inodes are sharing a
 * particular block, and is only stored in the first element of the
 * linked list for a particular block.  As the block conflicts are
 * resolved, num_bad is decremented; when it reaches 1, then we no
 * longer need to worry about that block.
 */
struct dup_block {
	blk_t		block;		/* Block number */
	ino_t		ino;		/* Inode number */
	int		num_bad;
	/* Pointer to next dup record with different block */
	struct dup_block *next_block;
	/* Pointer to next dup record with different inode */
	struct dup_block *next_inode;
};

/*
 * This structure stores information about a particular inode which
 * is sharing blocks with other inodes.  This information is collected
 * to display to the user, so that the user knows what files he or she
 * is dealing with, when trying to decide how to resolve the conflict
 * of multiply-claimed blocks.
 */
struct dup_inode {
	ino_t			ino, dir;
	int			num_dupblocks;
	struct ext2_inode	inode;
	struct dup_inode	*next;
};

static int process_pass1b_block(ext2_filsys fs, blk_t	*blocknr,
				int	blockcnt, void	*priv_data);
static void delete_file(e2fsck_t ctx, struct dup_inode *dp,
			char *block_buf);
static int clone_file(e2fsck_t ctx, struct dup_inode *dp, char* block_buf);
static void pass1b(e2fsck_t ctx, char *block_buf);
static void pass1c(e2fsck_t ctx, char *block_buf);
static void pass1d(e2fsck_t ctx, char *block_buf);

static struct dup_block *dup_blk = 0;
static struct dup_inode *dup_ino = 0;
static int dup_inode_count = 0;

static ext2fs_inode_bitmap inode_dup_map;

/*
 * Main procedure for handling duplicate blocks
 */
void e2fsck_pass1_dupblocks(e2fsck_t ctx, char *block_buf)
{
	ext2_filsys 		fs = ctx->fs;
	struct dup_block	*p, *q, *next_p, *next_q;
	struct dup_inode	*r, *next_r;
	struct problem_context	pctx;

	clear_problem_context(&pctx);
	
	pctx.errcode = ext2fs_allocate_inode_bitmap(fs,
		      "multiply claimed inode map", &inode_dup_map);
	if (pctx.errcode) {
		fix_problem(ctx, PR_1B_ALLOCATE_IBITMAP_ERROR, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
	
	pass1b(ctx, block_buf);
	pass1c(ctx, block_buf);
	pass1d(ctx, block_buf);

	/*
	 * Time to free all of the accumulated data structures that we
	 * don't need anymore.
	 */
	ext2fs_free_inode_bitmap(inode_dup_map); inode_dup_map = 0;
	ext2fs_free_block_bitmap(ctx->block_dup_map); ctx->block_dup_map = 0;
	for (p = dup_blk; p; p = next_p) {
		next_p = p->next_block;
		for (q = p; q; q = next_q) {
			next_q = q->next_inode;
			ext2fs_free_mem((void **) &q);
		}
	}
	for (r = dup_ino; r; r = next_r) {
		next_r = r->next;
		ext2fs_free_mem((void **) &r);
	}
}

/*
 * Scan the inodes looking for inodes that contain duplicate blocks.
 */
struct process_block_struct {
	ino_t	ino;
	int	dup_blocks;
	e2fsck_t ctx;
	struct problem_context *pctx;
};

static void pass1b(e2fsck_t ctx, char *block_buf)
{
	ext2_filsys fs = ctx->fs;
	ino_t	ino;
	struct ext2_inode inode;
	ext2_inode_scan	scan;
	errcode_t	retval;
	struct process_block_struct pb;
	struct dup_inode *dp;
	struct problem_context pctx;

	clear_problem_context(&pctx);
	
	fix_problem(ctx, PR_1B_PASS_HEADER, &pctx);
	pctx.errcode = ext2fs_open_inode_scan(fs, ctx->inode_buffer_blocks,
					      &scan);
	if (pctx.errcode) {
		fix_problem(ctx, PR_1B_ISCAN_ERROR, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
	pctx.errcode = ext2fs_get_next_inode(scan, &ino, &inode);
	if (pctx.errcode) {
		fix_problem(ctx, PR_1B_ISCAN_ERROR, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
	ctx->stashed_inode = &inode;
	pb.ctx = ctx;
	pb.pctx = &pctx;
	while (ino) {
		pctx.ino = ctx->stashed_ino = ino;
		if ((ino != EXT2_BAD_INO) &&
		    (!ext2fs_test_inode_bitmap(ctx->inode_used_map, ino) ||
		     !ext2fs_inode_has_valid_blocks(&inode)))
			goto next;

		pb.ino = ino;
		pb.dup_blocks = 0;
		retval = ext2fs_block_iterate(fs, ino, 0, block_buf,
					      process_pass1b_block, &pb);
		if (pb.dup_blocks) {
			end_problem_latch(ctx, PR_LATCH_DBLOCK);
			dp = (struct dup_inode *) e2fsck_allocate_memory(ctx,
				    sizeof(struct dup_inode),
				    "duplicate inode record");
			dp->ino = ino;
			dp->dir = 0;
			dp->inode = inode;
			dp->num_dupblocks = pb.dup_blocks;
			dp->next = dup_ino;
			dup_ino = dp;
			if (ino != EXT2_BAD_INO)
				dup_inode_count++;
		}
		if (retval)
			com_err(ctx->program_name, retval,
				"while calling ext2fs_block_iterate in pass1b");
		
	next:
		pctx.errcode = ext2fs_get_next_inode(scan, &ino, &inode);
		if (pctx.errcode == EXT2_ET_BAD_BLOCK_IN_INODE_TABLE)
			goto next;
		if (pctx.errcode) {
			fix_problem(ctx, PR_1B_ISCAN_ERROR, &pctx);
			ctx->flags |= E2F_FLAG_ABORT;
			return;
		}
	}
	ext2fs_close_inode_scan(scan);
	fs->get_blocks = 0;
	fs->check_directory = 0;
}

int process_pass1b_block(ext2_filsys fs,
			 blk_t	*block_nr,
			 int blockcnt,
			 void *priv_data)
{
	struct process_block_struct *p;
	struct dup_block *dp, *q, *r;
	int i;
	e2fsck_t ctx;

	if (!*block_nr)
		return 0;
	p = (struct process_block_struct *) priv_data;
	ctx = p->ctx;
	
	if (ext2fs_test_block_bitmap(ctx->block_dup_map, *block_nr)) {
		/* OK, this is a duplicate block */
		if (p->ino != EXT2_BAD_INO) {
			p->pctx->blk = *block_nr;
			fix_problem(ctx, PR_1B_DUP_BLOCK, p->pctx);
		}
		p->dup_blocks++;
		ext2fs_mark_block_bitmap(ctx->block_dup_map, *block_nr);
		ext2fs_mark_inode_bitmap(inode_dup_map, p->ino);
		dp = (struct dup_block *) e2fsck_allocate_memory(ctx,
					    sizeof(struct dup_block),
					    "duplicate block record");
		dp->block = *block_nr;
		dp->ino = p->ino;
		dp->num_bad = 0;
		q = dup_blk;
		while (q) {
			if (q->block == *block_nr)
				break;
			q = q->next_block;
		}
		if (q) {
			dp->next_inode = q->next_inode;
			q->next_inode = dp;
		} else {
			dp->next_block = dup_blk;
			dup_blk = dp;
		}
	}
	/*
	 * Set the num_bad field
	 */
	for (q = dup_blk; q; q = q->next_block) {
		i = 0;
		for (r = q; r; r = r->next_inode)
			i++;
		q->num_bad = i;
	}
	return 0;
}

/*
 * Pass 1c: Scan directories for inodes with duplicate blocks.  This
 * is used so that we can print pathnames when prompting the user for
 * what to do.
 */
struct search_dir_struct {
	int		count;
	ino_t		first_inode;
	ino_t		max_inode;
};

static int search_dirent_proc(ino_t dir, int entry,
			      struct ext2_dir_entry *dirent,
			      int offset, int blocksize,
			      char *buf, void *priv_data)
{
	struct search_dir_struct *sd;
	struct dup_inode	*p;

	sd = (struct search_dir_struct *) priv_data;

	if (dirent->inode > sd->max_inode)
		/* Should abort this inode, but not everything */
		return 0;	

	if (!dirent->inode || (entry < DIRENT_OTHER_FILE) ||
	    !ext2fs_test_inode_bitmap(inode_dup_map, dirent->inode))
		return 0;

	for (p = dup_ino; p; p = p->next) {
		if ((p->ino >= sd->first_inode) && 
		    (p->ino == dirent->inode))
			break;
	}

	if (!p || p->dir)
		return 0;

	p->dir = dir;
	sd->count--;

	return(sd->count ? 0 : DIRENT_ABORT);
}


static void pass1c(e2fsck_t ctx, char *block_buf)
{
	ext2_filsys fs = ctx->fs;
	struct dup_inode	*p;
	int	inodes_left = dup_inode_count;
	struct search_dir_struct sd;
	struct problem_context pctx;

	clear_problem_context(&pctx);

	fix_problem(ctx, PR_1C_PASS_HEADER, &pctx);

	/*
	 * First check to see if any of the inodes with dup blocks is
	 * a special inode.  (Note that the bad block inode isn't
	 * counted.)
	 */
	for (p = dup_ino; p; p = p->next) {
		if ((p->ino < EXT2_FIRST_INODE(fs->super)) &&
		    (p->ino != EXT2_BAD_INO))
			inodes_left--;
	}

	/*
	 * Search through all directories to translate inodes to names
	 * (by searching for the containing directory for that inode.)
	 */
	sd.count = inodes_left;
	sd.first_inode = EXT2_FIRST_INODE(fs->super);
	sd.max_inode = fs->super->s_inodes_count;
	ext2fs_dblist_dir_iterate(fs->dblist, 0, block_buf,
				  search_dirent_proc, &sd);
}	

static void pass1d(e2fsck_t ctx, char *block_buf)
{
	ext2_filsys fs = ctx->fs;
	struct dup_inode	*p, *s;
	struct dup_block	*q, *r;
	ino_t	*shared;
	int	shared_len;
	int	i;
	int	file_ok;
	int	meta_data = 0;
	struct problem_context pctx;

	clear_problem_context(&pctx);
	
	fix_problem(ctx, PR_1D_PASS_HEADER, &pctx);
	e2fsck_read_bitmaps(ctx);

	pctx.num = dup_inode_count;
	fix_problem(ctx, PR_1D_NUM_DUP_INODES, &pctx);
	shared = (ino_t *) e2fsck_allocate_memory(ctx,
				sizeof(ino_t) * dup_inode_count,
				"Shared inode list");
	for (p = dup_ino; p; p = p->next) {
		shared_len = 0;
		file_ok = 1;
		if (p->ino == EXT2_BAD_INO)
			continue;

		/*
		 * Search through the duplicate records to see which
		 * inodes share blocks with this one
		 */
		for (q = dup_blk; q; q = q->next_block) {
			/*
			 * See if this block is used by this inode.
			 * If it isn't, continue.
			 */
			for (r = q; r; r = r->next_inode)
				if (r->ino == p->ino)
					break;
			if (!r)
				continue;
			if (q->num_bad > 1)
				file_ok = 0;
			if (ext2fs_test_block_bitmap(ctx->block_illegal_map,
						     q->block)) {
				file_ok = 0;
				meta_data = 1;
			}
			
			/*
			 * Add all inodes used by this block to the
			 * shared[] --- which is a unique list, so
			 * if an inode is already in shared[], don't
			 * add it again.
			 */
			for (r = q; r; r = r->next_inode) {
				if (r->ino == p->ino)
					continue;
				for (i = 0; i < shared_len; i++)
					if (shared[i] == r->ino)
						break;
				if (i == shared_len) {
					shared[shared_len++] = r->ino;
				}
			}
		}

		/*
		 * Report the inode that we are working on
		 */
		pctx.inode = &p->inode;
		pctx.ino = p->ino;
		pctx.dir = p->dir;
		pctx.blkcount = p->num_dupblocks;
		pctx.num = meta_data ? shared_len+1 : shared_len;
		fix_problem(ctx, PR_1D_DUP_FILE, &pctx);
		pctx.blkcount = 0;
		pctx.num = 0;
		
		if (meta_data)
			fix_problem(ctx, PR_1D_SHARE_METADATA, &pctx);
		
		for (i = 0; i < shared_len; i++) {
			for (s = dup_ino; s; s = s->next)
				if (s->ino == shared[i])
					break;
			if (!s)
				continue;
			/*
			 * Report the inode that we are sharing with
			 */
			pctx.inode = &s->inode;
			pctx.ino = s->ino;
			pctx.dir = s->dir;
			fix_problem(ctx, PR_1D_DUP_FILE_LIST, &pctx);
		}
		if (file_ok) {
			fix_problem(ctx, PR_1D_DUP_BLOCKS_DEALT, &pctx);
			continue;
		}
		if (fix_problem(ctx, PR_1D_CLONE_QUESTION, &pctx)) {
			pctx.errcode = clone_file(ctx, p, block_buf);
			if (pctx.errcode)
				fix_problem(ctx, PR_1D_CLONE_ERROR, &pctx);
			else
				continue;
		}
		if (fix_problem(ctx, PR_1D_DELETE_QUESTION, &pctx))
			delete_file(ctx, p, block_buf);
		else
			ext2fs_unmark_valid(fs);
	}
	ext2fs_free_mem((void **) &shared);
}

static int delete_file_block(ext2_filsys fs,
			     blk_t	*block_nr,
			     int blockcnt,
			     void *priv_data)
{
	struct process_block_struct *pb;
	struct dup_block *p;
	e2fsck_t ctx;

	pb = (struct process_block_struct *) priv_data;
	ctx = pb->ctx;

	if (!*block_nr)
		return 0;

	if (ext2fs_test_block_bitmap(ctx->block_dup_map, *block_nr)) {
		for (p = dup_blk; p; p = p->next_block)
			if (p->block == *block_nr)
				break;
		if (p) {
			p->num_bad--;
			if (p->num_bad == 1)
				ext2fs_unmark_block_bitmap(ctx->block_dup_map,
							   *block_nr);
		} else
			com_err("delete_file_block", 0,
				"internal error; can't find dup_blk for %d\n",
				*block_nr);
	} else {
		ext2fs_unmark_block_bitmap(ctx->block_found_map, *block_nr);
		ext2fs_unmark_block_bitmap(fs->block_map, *block_nr);
	}
		
	return 0;
}
		
static void delete_file(e2fsck_t ctx, struct dup_inode *dp, char* block_buf)
{
	ext2_filsys fs = ctx->fs;
	errcode_t	retval;
	struct process_block_struct pb;
	struct ext2_inode	inode;

	pb.ino = dp->ino;
	pb.dup_blocks = dp->num_dupblocks;
	pb.ctx = ctx;
	
	retval = ext2fs_block_iterate(fs, dp->ino, 0, block_buf,
				      delete_file_block, &pb);
	if (retval)
		com_err("delete_file", retval,
			"while calling ext2fs_block_iterate for inode %d",
			dp->ino);
	ext2fs_unmark_inode_bitmap(ctx->inode_used_map, dp->ino);
	ext2fs_unmark_inode_bitmap(ctx->inode_dir_map, dp->ino);
	if (ctx->inode_bad_map)
		ext2fs_unmark_inode_bitmap(ctx->inode_bad_map, dp->ino);
	ext2fs_unmark_inode_bitmap(fs->inode_map, dp->ino);
	ext2fs_mark_ib_dirty(fs);
	ext2fs_mark_bb_dirty(fs);
	e2fsck_read_inode(ctx, dp->ino, &inode, "delete_file");
	inode.i_links_count = 0;
	inode.i_dtime = time(0);
	e2fsck_write_inode(ctx, dp->ino, &inode, "delete_file");
}

struct clone_struct {
	errcode_t	errcode;
	ino_t		dir;
	char	*buf;
	e2fsck_t ctx;
};

static int clone_file_block(ext2_filsys fs,
			    blk_t	*block_nr,
			    int blockcnt,
			    void *priv_data)
{
	struct dup_block *p;
	blk_t	new_block;
	errcode_t	retval;
	struct clone_struct *cs = (struct clone_struct *) priv_data;
	e2fsck_t ctx;

	ctx = cs->ctx;
	
	if (!*block_nr)
		return 0;

	if (ext2fs_test_block_bitmap(ctx->block_dup_map, *block_nr)) {
		for (p = dup_blk; p; p = p->next_block)
			if (p->block == *block_nr)
				break;
		if (p) {
			retval = ext2fs_new_block(fs, 0, ctx->block_found_map,
						  &new_block);
			if (retval) {
				cs->errcode = retval;
				return BLOCK_ABORT;
			}
			if (cs->dir) {
				retval = ext2fs_set_dir_block(fs->dblist,
				      cs->dir, new_block, blockcnt);
				if (retval) {
					cs->errcode = retval;
					return BLOCK_ABORT;
				}
			}
			retval = io_channel_read_blk(fs->io, *block_nr, 1,
						     cs->buf);
			if (retval) {
				cs->errcode = retval;
				return BLOCK_ABORT;
			}
			retval = io_channel_write_blk(fs->io, new_block, 1,
						      cs->buf);
			if (retval) {
				cs->errcode = retval;
				return BLOCK_ABORT;
			}
			p->num_bad--;
			if (p->num_bad == 1 &&
			    !ext2fs_test_block_bitmap(ctx->block_illegal_map,
						      *block_nr))
				ext2fs_unmark_block_bitmap(ctx->block_dup_map,
							   *block_nr);
			*block_nr = new_block;
			ext2fs_mark_block_bitmap(ctx->block_found_map,
						 new_block);
			ext2fs_mark_block_bitmap(fs->block_map, new_block);
			return BLOCK_CHANGED;
		} else
			com_err("clone_file_block", 0,
				"internal error; can't find dup_blk for %d\n",
				*block_nr);
	}
	return 0;
}
		
static int clone_file(e2fsck_t ctx, struct dup_inode *dp, char* block_buf)
{
	ext2_filsys fs = ctx->fs;
	errcode_t	retval;
	struct clone_struct cs;

	cs.errcode = 0;
	cs.dir = 0;
	cs.ctx = ctx;
	retval = ext2fs_get_mem(fs->blocksize, (void **) &cs.buf);
	if (retval)
		return retval;

	if (ext2fs_test_inode_bitmap(ctx->inode_dir_map, dp->ino))
		cs.dir = dp->ino;
	
	retval = ext2fs_block_iterate(fs, dp->ino, 0, block_buf,
				      clone_file_block, &cs);
	ext2fs_mark_bb_dirty(fs);
	ext2fs_free_mem((void **) &cs.buf);
	if (retval) {
		com_err("clone_file", retval,
			"while calling ext2fs_block_iterate for inode %d",
			dp->ino);
		return retval;
	}
	if (cs.errcode) {
		com_err("clone_file", cs.errcode,
			"returned from clone_file_block");
		return retval;
	}
	return 0;
}
