/*
 * pass1.c -- pass #1 of e2fsck: sequential scan of the inode table
 * 
 * Copyright (C) 1993, 1994, 1995, 1996, 1997 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 * 
 * Pass 1 of e2fsck iterates over all the inodes in the filesystems,
 * and applies the following tests to each inode:
 *
 * 	- The mode field of the inode must be legal.
 * 	- The size and block count fields of the inode are correct.
 * 	- A data block must not be used by another inode
 *
 * Pass 1 also gathers the collects the following information:
 *
 * 	- A bitmap of which inodes are in use.		(inode_used_map)
 * 	- A bitmap of which inodes are directories.	(inode_dir_map)
 * 	- A bitmap of which inodes are regular files.	(inode_reg_map)
 * 	- A bitmap of which inodes have bad fields.	(inode_bad_map)
 * 	- A bitmap of which inodes are in bad blocks.	(inode_bb_map)
 * 	- A bitmap of which inodes are imagic inodes.	(inode_imagic_map)
 * 	- A bitmap of which blocks are in use.		(block_found_map)
 * 	- A bitmap of which blocks are in use by two inodes	(block_dup_map)
 * 	- The data blocks of the directory inodes.	(dir_map)
 *
 * Pass 1 is designed to stash away enough information so that the
 * other passes should not need to read in the inode information
 * during the normal course of a filesystem check.  (Althogh if an
 * inconsistency is detected, other passes may need to read in an
 * inode to fix it.)
 *
 * Note that pass 1B will be invoked if there are any duplicate blocks
 * found.
 */

#include <time.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "e2fsck.h"
#include "problem.h"

#ifdef NO_INLINE_FUNCS
#define _INLINE_
#else
#define _INLINE_ inline
#endif

static int process_block(ext2_filsys fs, blk_t	*blocknr,
			 e2_blkcnt_t blockcnt, blk_t ref_blk, 
			 int ref_offset, void *priv_data);
static int process_bad_block(ext2_filsys fs, blk_t *block_nr,
			     e2_blkcnt_t blockcnt, blk_t ref_blk,
			     int ref_offset, void *priv_data);
static void check_blocks(e2fsck_t ctx, struct problem_context *pctx,
			 char *block_buf);
static void mark_table_blocks(e2fsck_t ctx);
static void alloc_bad_map(e2fsck_t ctx);
static void alloc_bb_map(e2fsck_t ctx);
static void alloc_imagic_map(e2fsck_t ctx);
static void handle_fs_bad_blocks(e2fsck_t ctx);
static void process_inodes(e2fsck_t ctx, char *block_buf);
static EXT2_QSORT_TYPE process_inode_cmp(const void *a, const void *b);
static errcode_t scan_callback(ext2_filsys fs, ext2_inode_scan scan,
				  dgrp_t group, void * priv_data);
/* static char *describe_illegal_block(ext2_filsys fs, blk_t block); */

struct process_block_struct {
	ino_t		ino;
	int		is_dir:1, clear:1, suppress:1, fragmented:1;
	blk_t		num_blocks;
	e2_blkcnt_t	last_block;
	int		num_illegal_blocks;
	blk_t		previous_block;
	struct ext2_inode *inode;
	struct problem_context *pctx;
	e2fsck_t	ctx;
};

struct process_inode_block {
	ino_t	ino;
	struct ext2_inode inode;
};

struct scan_callback_struct {
	e2fsck_t	ctx;
	char		*block_buf;
};

/*
 * For the inodes to process list.
 */
static struct process_inode_block *inodes_to_process;
static int process_inode_count;

static __u64 ext2_max_sizes[4];

/*
 * Free all memory allocated by pass1 in preparation for restarting
 * things.
 */
static void unwind_pass1(ext2_filsys fs)
{
	ext2fs_free_mem((void **) &inodes_to_process);
	inodes_to_process = 0;
}

/*
 * Check to make sure a device inode is real.  Returns 1 if the device
 * checks out, 0 if not.
 *
 * Note: this routine is now also used to check FIFO's and Sockets,
 * since they have the same requirement; the i_block fields should be
 * zero. 
 */
int e2fsck_pass1_check_device_inode(struct ext2_inode *inode)
{
	int	i;

	/*
	 * We should be able to do the test below all the time, but
	 * because the kernel doesn't forcibly clear the device
	 * inode's additional i_block fields, there are some rare
	 * occasions when a legitimate device inode will have non-zero
	 * additional i_block fields.  So for now, we only complain
	 * when the immutable flag is set, which should never happen
	 * for devices.  (And that's when the problem is caused, since
	 * you can't set or clear immutable flags for devices.)  Once
	 * the kernel has been fixed we can change this...
	 */
	if (inode->i_flags & EXT2_IMMUTABLE_FL) {
		for (i=4; i < EXT2_N_BLOCKS; i++) 
			if (inode->i_block[i])
				return 0;
	}
	return 1;
}

/*
 * If the immutable flag is set on the inode, offer to clear it.
 */
static void check_immutable(e2fsck_t ctx, struct problem_context *pctx)
{
	if (!(pctx->inode->i_flags & EXT2_IMMUTABLE_FL))
		return;

	if (!fix_problem(ctx, PR_1_SET_IMMUTABLE, pctx))
		return;

	pctx->inode->i_flags &= ~EXT2_IMMUTABLE_FL;
	e2fsck_write_inode(ctx, pctx->ino, pctx->inode, "pass1");
}


void e2fsck_pass1(e2fsck_t ctx)
{
	int	i;
	__u64	max_sizes;
	ext2_filsys fs = ctx->fs;
	ino_t	ino;
	struct ext2_inode inode;
	ext2_inode_scan	scan;
	char		*block_buf;
#ifdef RESOURCE_TRACK
	struct resource_track	rtrack;
#endif
	unsigned char	frag, fsize;
	struct		problem_context pctx;
	struct		scan_callback_struct scan_struct;
	struct ext2fs_sb *sb;
	int		imagic_fs;
	
#ifdef RESOURCE_TRACK
	init_resource_track(&rtrack);
#endif
	clear_problem_context(&pctx);

	if (!(ctx->options & E2F_OPT_PREEN))
		fix_problem(ctx, PR_1_PASS_HEADER, &pctx);

#ifdef MTRACE
	mtrace_print("Pass 1");
#endif

#define EXT2_BPP(bits) (1UL << ((bits) - 2))

	for (i=0; i < 4; i++) {
		max_sizes = EXT2_NDIR_BLOCKS + EXT2_BPP(10+i);
		max_sizes = max_sizes + EXT2_BPP(10+i) * EXT2_BPP(10+i);
		max_sizes = (max_sizes +
			     (__u64) EXT2_BPP(10+i) * EXT2_BPP(10+i) *
			     EXT2_BPP(10+i));
		max_sizes = (max_sizes * (1UL << (10+i))) - 1;
		ext2_max_sizes[i] = max_sizes;
	}
#undef EXT2_BPP

	sb = (struct ext2fs_sb *) fs->super;
	imagic_fs = (sb->s_feature_compat & EXT2_FEATURE_COMPAT_IMAGIC_INODES);

	/*
	 * Allocate bitmaps structures
	 */
	pctx.errcode = ext2fs_allocate_inode_bitmap(fs, "in-use inode map",
					      &ctx->inode_used_map);
	if (pctx.errcode) {
		pctx.num = 1;
		fix_problem(ctx, PR_1_ALLOCATE_IBITMAP_ERROR, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
	pctx.errcode = ext2fs_allocate_inode_bitmap(fs, "directory inode map",
					      &ctx->inode_dir_map);
	if (pctx.errcode) {
		pctx.num = 2;
		fix_problem(ctx, PR_1_ALLOCATE_IBITMAP_ERROR, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
	pctx.errcode = ext2fs_allocate_inode_bitmap(fs,
						    "regular file inode map",
					      &ctx->inode_reg_map);
	if (pctx.errcode) {
		pctx.num = 6;
		fix_problem(ctx, PR_1_ALLOCATE_IBITMAP_ERROR, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
	pctx.errcode = ext2fs_allocate_block_bitmap(fs, "in-use block map",
					      &ctx->block_found_map);
	if (pctx.errcode) {
		pctx.num = 1;
		fix_problem(ctx, PR_1_ALLOCATE_BBITMAP_ERROR, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
	pctx.errcode = ext2fs_allocate_block_bitmap(fs, "illegal block map",
					      &ctx->block_illegal_map);
	if (pctx.errcode) {
		pctx.num = 2;
		fix_problem(ctx, PR_1_ALLOCATE_BBITMAP_ERROR, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
	pctx.errcode = ext2fs_create_icount2(fs, 0, 0, 0,
					     &ctx->inode_link_info);
	if (pctx.errcode) {
		fix_problem(ctx, PR_1_ALLOCATE_ICOUNT, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
	inodes_to_process = (struct process_inode_block *)
		e2fsck_allocate_memory(ctx,
				       (ctx->process_inode_size *
					sizeof(struct process_inode_block)),
				       "array of inodes to process");
	process_inode_count = 0;

	pctx.errcode = ext2fs_init_dblist(fs, 0);
	if (pctx.errcode) {
		fix_problem(ctx, PR_1_ALLOCATE_DBCOUNT, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}

	mark_table_blocks(ctx);
	block_buf = (char *) e2fsck_allocate_memory(ctx, fs->blocksize * 3,
						    "block interate buffer");
	e2fsck_use_inode_shortcuts(ctx, 1);
	ehandler_operation("doing inode scan");
	pctx.errcode = ext2fs_open_inode_scan(fs, ctx->inode_buffer_blocks, 
					      &scan);
	if (pctx.errcode) {
		fix_problem(ctx, PR_1_ISCAN_ERROR, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
	ext2fs_inode_scan_flags(scan, EXT2_SF_SKIP_MISSING_ITABLE, 0);
	pctx.errcode = ext2fs_get_next_inode(scan, &ino, &inode);
	if (pctx.errcode) {
		fix_problem(ctx, PR_1_ISCAN_ERROR, &pctx);
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
	ctx->stashed_inode = &inode;
	scan_struct.ctx = ctx;
	scan_struct.block_buf = block_buf;
	ext2fs_set_inode_callback(scan, scan_callback, &scan_struct);
	if (ctx->progress)
		if ((ctx->progress)(ctx, 1, 0, ctx->fs->group_desc_count))
			return;
	while (ino) {
		pctx.ino = ino;
		pctx.inode = &inode;
		ctx->stashed_ino = ino;
		if (inode.i_links_count) {
			pctx.errcode = ext2fs_icount_store(ctx->inode_link_info, 
					   ino, inode.i_links_count);
			if (pctx.errcode) {
				pctx.num = inode.i_links_count;
				fix_problem(ctx, PR_1_ICOUNT_STORE, &pctx);
				ctx->flags |= E2F_FLAG_ABORT;
				return;
			}
		}
		if (ino == EXT2_BAD_INO) {
			struct process_block_struct pb;
			
			pb.ino = EXT2_BAD_INO;
			pb.num_blocks = pb.last_block = 0;
			pb.num_illegal_blocks = 0;
			pb.suppress = 0; pb.clear = 0; pb.is_dir = 0;
			pb.fragmented = 0;
			pb.inode = &inode;
			pb.pctx = &pctx;
			pb.ctx = ctx;
			pctx.errcode = ext2fs_block_iterate2(fs, ino, 0, 
				     block_buf, process_bad_block, &pb);
			if (pctx.errcode) {
				fix_problem(ctx, PR_1_BLOCK_ITERATE, &pctx);
				ctx->flags |= E2F_FLAG_ABORT;
				return;
			}
			ext2fs_mark_inode_bitmap(ctx->inode_used_map, ino);
			clear_problem_context(&pctx);
			goto next;
		}
		if (ino == EXT2_ROOT_INO) {
			/*
			 * Make sure the root inode is a directory; if
			 * not, offer to clear it.  It will be
			 * regnerated in pass #3.
			 */
			if (!LINUX_S_ISDIR(inode.i_mode)) {
				if (fix_problem(ctx, PR_1_ROOT_NO_DIR, &pctx)) {
					inode.i_dtime = time(0);
					inode.i_links_count = 0;
					ext2fs_icount_store(ctx->inode_link_info,
							    ino, 0);
					e2fsck_write_inode(ctx, ino, &inode,
							   "pass1");
				}
			}
			/*
			 * If dtime is set, offer to clear it.  mke2fs
			 * version 0.2b created filesystems with the
			 * dtime field set for the root and lost+found
			 * directories.  We won't worry about
			 * /lost+found, since that can be regenerated
			 * easily.  But we will fix the root directory
			 * as a special case.
			 */
			if (inode.i_dtime && inode.i_links_count) {
				if (fix_problem(ctx, PR_1_ROOT_DTIME, &pctx)) {
					inode.i_dtime = 0;
					e2fsck_write_inode(ctx, ino, &inode,
							   "pass1");
				}
			}
		}
		if ((ino != EXT2_ROOT_INO) &&
		    (ino < EXT2_FIRST_INODE(fs->super))) {
			ext2fs_mark_inode_bitmap(ctx->inode_used_map, ino);
			if (((ino == EXT2_BOOT_LOADER_INO) &&
			     LINUX_S_ISDIR(inode.i_mode)) ||
			    ((ino != EXT2_BOOT_LOADER_INO) &&
			     (inode.i_mode != 0))) {
				if (fix_problem(ctx,
					    PR_1_RESERVED_BAD_MODE, &pctx)) {
					inode.i_mode = 0;
					e2fsck_write_inode(ctx, ino, &inode,
							   "pass1");
				}
			}
			check_blocks(ctx, &pctx, block_buf);
			goto next;
		}
		/*
		 * This code assumes that deleted inodes have
		 * i_links_count set to 0.  
		 */
		if (!inode.i_links_count) {
			if (!inode.i_dtime && inode.i_mode) {
				if (fix_problem(ctx,
					    PR_1_ZERO_DTIME, &pctx)) {
					inode.i_dtime = time(0);
					e2fsck_write_inode(ctx, ino, &inode,
							   "pass1");
				}
			}
			goto next;
		}
		/*
		 * n.b.  0.3c ext2fs code didn't clear i_links_count for
		 * deleted files.  Oops.
		 *
		 * Since all new ext2 implementations get this right,
		 * we now assume that the case of non-zero
		 * i_links_count and non-zero dtime means that we
		 * should keep the file, not delete it.
		 * 
		 */
		if (inode.i_dtime) {
			if (fix_problem(ctx, PR_1_SET_DTIME, &pctx)) {
				inode.i_dtime = 0;
				e2fsck_write_inode(ctx, ino, &inode, "pass1");
			}
		}
		
		ext2fs_mark_inode_bitmap(ctx->inode_used_map, ino);
		switch (fs->super->s_creator_os) {
		    case EXT2_OS_LINUX:
			frag = inode.osd2.linux2.l_i_frag;
			fsize = inode.osd2.linux2.l_i_fsize;
			break;
		    case EXT2_OS_HURD:
			frag = inode.osd2.hurd2.h_i_frag;
			fsize = inode.osd2.hurd2.h_i_fsize;
			break;
		    case EXT2_OS_MASIX:
			frag = inode.osd2.masix2.m_i_frag;
			fsize = inode.osd2.masix2.m_i_fsize;
			break;
		    default:
			frag = fsize = 0;
		}
		
		if (inode.i_faddr || frag || fsize
		    || inode.i_file_acl ||
		    (LINUX_S_ISDIR(inode.i_mode) && inode.i_dir_acl)) {
			if (!ctx->inode_bad_map)
				alloc_bad_map(ctx);
			ext2fs_mark_inode_bitmap(ctx->inode_bad_map, ino);
		}
		if (inode.i_flags & EXT2_IMAGIC_FL) {
			if (imagic_fs) {
				if (!ctx->inode_imagic_map)
					alloc_imagic_map(ctx);
				ext2fs_mark_inode_bitmap(ctx->inode_imagic_map,
							 ino);
			} else {
				if (fix_problem(ctx, PR_1_SET_IMAGIC, &pctx)) {
					inode.i_flags &= ~EXT2_IMAGIC_FL;
					e2fsck_write_inode(ctx, ino,
							   &inode, "pass1");
				}
			}
		}
		
		if (LINUX_S_ISDIR(inode.i_mode)) {
			ext2fs_mark_inode_bitmap(ctx->inode_dir_map, ino);
			e2fsck_add_dir_info(ctx, ino, 0);
			ctx->fs_directory_count++;
		} else if (LINUX_S_ISREG (inode.i_mode)) {
			ext2fs_mark_inode_bitmap(ctx->inode_reg_map, ino);
			ctx->fs_regular_count++;
		} else if (LINUX_S_ISCHR (inode.i_mode) &&
			   e2fsck_pass1_check_device_inode(&inode)) {
			check_immutable(ctx, &pctx);
			ctx->fs_chardev_count++;
		} else if (LINUX_S_ISBLK (inode.i_mode) &&
			   e2fsck_pass1_check_device_inode(&inode)) {
			check_immutable(ctx, &pctx);
			ctx->fs_blockdev_count++;
		} else if (LINUX_S_ISLNK (inode.i_mode)) {
			ctx->fs_symlinks_count++;
			if (!inode.i_blocks) {
				ctx->fs_fast_symlinks_count++;
				goto next;
			}
		}
		else if (LINUX_S_ISFIFO (inode.i_mode) &&
			 e2fsck_pass1_check_device_inode(&inode)) {
			check_immutable(ctx, &pctx);
			ctx->fs_fifo_count++;
		} else if ((LINUX_S_ISSOCK (inode.i_mode)) &&
			   e2fsck_pass1_check_device_inode(&inode)) {
			check_immutable(ctx, &pctx);
		        ctx->fs_sockets_count++;
		} else {
			if (!ctx->inode_bad_map)
				alloc_bad_map(ctx);
			ext2fs_mark_inode_bitmap(ctx->inode_bad_map, ino);
		}
		if (inode.i_block[EXT2_IND_BLOCK])
			ctx->fs_ind_count++;
		if (inode.i_block[EXT2_DIND_BLOCK])
			ctx->fs_dind_count++;
		if (inode.i_block[EXT2_TIND_BLOCK])
			ctx->fs_tind_count++;
		if (inode.i_block[EXT2_IND_BLOCK] ||
		    inode.i_block[EXT2_DIND_BLOCK] ||
		    inode.i_block[EXT2_TIND_BLOCK]) {
			inodes_to_process[process_inode_count].ino = ino;
			inodes_to_process[process_inode_count].inode = inode;
			process_inode_count++;
		} else
			check_blocks(ctx, &pctx, block_buf);

		if (ctx->flags & E2F_FLAG_SIGNAL_MASK)
			return;

		if (process_inode_count >= ctx->process_inode_size) {
			process_inodes(ctx, block_buf);

			if (ctx->flags & E2F_FLAG_SIGNAL_MASK)
				return;
		}
	next:
		pctx.errcode = ext2fs_get_next_inode(scan, &ino, &inode);
		if (ctx->flags & E2F_FLAG_SIGNAL_MASK)
			return;
		if (pctx.errcode == EXT2_ET_BAD_BLOCK_IN_INODE_TABLE) {
			if (!ctx->inode_bb_map)
				alloc_bb_map(ctx);
			ext2fs_mark_inode_bitmap(ctx->inode_bb_map, ino);
			ext2fs_mark_inode_bitmap(ctx->inode_used_map, ino);
			goto next;
		}
		if (pctx.errcode) {
			fix_problem(ctx, PR_1_ISCAN_ERROR, &pctx);
			ctx->flags |= E2F_FLAG_ABORT;
			return;
		}
	}
	process_inodes(ctx, block_buf);
	ext2fs_close_inode_scan(scan);
	ehandler_operation(0);

	if (ctx->invalid_bitmaps)
		handle_fs_bad_blocks(ctx);

	if (ctx->flags & E2F_FLAG_RESTART) {
		/*
		 * Only the master copy of the superblock and block
		 * group descriptors are going to be written during a
		 * restart, so set the superblock to be used to be the
		 * master superblock.
		 */
		ctx->use_superblock = 0;
		unwind_pass1(fs);
		goto endit;
	}

	if (ctx->block_dup_map) {
		if (ctx->options & E2F_OPT_PREEN) {
			clear_problem_context(&pctx);
			fix_problem(ctx, PR_1_DUP_BLOCKS_PREENSTOP, &pctx);
		}
		e2fsck_pass1_dupblocks(ctx, block_buf);
	}
	ext2fs_free_mem((void **) &inodes_to_process);
endit:
	e2fsck_use_inode_shortcuts(ctx, 0);
	
	ext2fs_free_mem((void **) &block_buf);
	ext2fs_free_block_bitmap(ctx->block_illegal_map);
	ctx->block_illegal_map = 0;

	if (ctx->large_files && 
	    !(sb->s_feature_ro_compat & 
	      EXT2_FEATURE_RO_COMPAT_LARGE_FILE)) {
		if (fix_problem(ctx, PR_1_FEATURE_LARGE_FILES, &pctx)) {
			sb->s_feature_ro_compat |= 
				EXT2_FEATURE_RO_COMPAT_LARGE_FILE;
			ext2fs_mark_super_dirty(fs);
		}
	} else if (!ctx->large_files &&
	    (sb->s_feature_ro_compat &
	      EXT2_FEATURE_RO_COMPAT_LARGE_FILE)) {
		if (fs->flags & EXT2_FLAG_RW) {
			sb->s_feature_ro_compat &= 
				~EXT2_FEATURE_RO_COMPAT_LARGE_FILE;
			ext2fs_mark_super_dirty(fs);
		}
	}
	
#ifdef RESOURCE_TRACK
	if (ctx->options & E2F_OPT_TIME2) {
		e2fsck_clear_progbar(ctx);
		print_resource_track("Pass 1", &rtrack);
	}
#endif
}

/*
 * When the inode_scan routines call this callback at the end of the
 * glock group, call process_inodes.
 */
static errcode_t scan_callback(ext2_filsys fs, ext2_inode_scan scan,
			       dgrp_t group, void * priv_data)
{
	struct scan_callback_struct *scan_struct;
	e2fsck_t ctx;

	scan_struct = (struct scan_callback_struct *) priv_data;
	ctx = scan_struct->ctx;
	
	process_inodes((e2fsck_t) fs->priv_data, scan_struct->block_buf);

	if (ctx->progress)
		if ((ctx->progress)(ctx, 1, group+1,
				    ctx->fs->group_desc_count))
			return EXT2_ET_CANCEL_REQUESTED;

	return 0;
}

/*
 * Process the inodes in the "inodes to process" list.
 */
static void process_inodes(e2fsck_t ctx, char *block_buf)
{
	int			i;
	struct ext2_inode	*old_stashed_inode;
	ino_t			old_stashed_ino;
	const char		*old_operation;
	char			buf[80];
	struct problem_context	pctx;
	
#if 0
	printf("begin process_inodes: ");
#endif
	old_operation = ehandler_operation(0);
	old_stashed_inode = ctx->stashed_inode;
	old_stashed_ino = ctx->stashed_ino;
	qsort(inodes_to_process, process_inode_count,
		      sizeof(struct process_inode_block), process_inode_cmp);
	clear_problem_context(&pctx);
	for (i=0; i < process_inode_count; i++) {
		pctx.inode = ctx->stashed_inode = &inodes_to_process[i].inode;
		pctx.ino = ctx->stashed_ino = inodes_to_process[i].ino;
		
#if 0
		printf("%u ", pctx.ino);
#endif
		sprintf(buf, "reading indirect blocks of inode %lu", pctx.ino);
		ehandler_operation(buf);
		check_blocks(ctx, &pctx, block_buf);
		if (ctx->flags & E2F_FLAG_SIGNAL_MASK)
			break;
	}
	ctx->stashed_inode = old_stashed_inode;
	ctx->stashed_ino = old_stashed_ino;
	process_inode_count = 0;
#if 0
	printf("end process inodes\n");
#endif
	ehandler_operation(old_operation);
}

static EXT2_QSORT_TYPE process_inode_cmp(const void *a, const void *b)
{
	const struct process_inode_block *ib_a =
		(const struct process_inode_block *) a;
	const struct process_inode_block *ib_b =
		(const struct process_inode_block *) b;

	return (ib_a->inode.i_block[EXT2_IND_BLOCK] -
		ib_b->inode.i_block[EXT2_IND_BLOCK]);
}

/*
 * This procedure will allocate the inode bad map table
 */
static void alloc_bad_map(e2fsck_t ctx)
{
	struct		problem_context pctx;
	
	clear_problem_context(&pctx);
	
	pctx.errcode = ext2fs_allocate_inode_bitmap(ctx->fs, "bad inode map",
					      &ctx->inode_bad_map);
	if (pctx.errcode) {
		pctx.num = 3;
		fix_problem(ctx, PR_1_ALLOCATE_IBITMAP_ERROR, &pctx);
		/* Should never get here */
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
}

/*
 * This procedure will allocate the inode "bb" (badblock) map table
 */
static void alloc_bb_map(e2fsck_t ctx)
{
	struct		problem_context pctx;
	
	clear_problem_context(&pctx);
	pctx.errcode = ext2fs_allocate_inode_bitmap(ctx->fs,
					      "inode in bad block map",
					      &ctx->inode_bb_map);
	if (pctx.errcode) {
		pctx.num = 4;
		fix_problem(ctx, PR_1_ALLOCATE_IBITMAP_ERROR, &pctx);
		/* Should never get here */
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
}

/*
 * This procedure will allocate the inode imagic table
 */
static void alloc_imagic_map(e2fsck_t ctx)
{
	struct		problem_context pctx;
	
	clear_problem_context(&pctx);
	pctx.errcode = ext2fs_allocate_inode_bitmap(ctx->fs,
					      "imagic inode map",
					      &ctx->inode_imagic_map);
	if (pctx.errcode) {
		pctx.num = 5;
		fix_problem(ctx, PR_1_ALLOCATE_IBITMAP_ERROR, &pctx);
		/* Should never get here */
		ctx->flags |= E2F_FLAG_ABORT;
		return;
	}
}

/*
 * Marks a block as in use, setting the dup_map if it's been set
 * already.  Called by process_block and process_bad_block.
 *
 * WARNING: Assumes checks have already been done to make sure block
 * is valid.  This is true in both process_block and process_bad_block.
 */
static _INLINE_ void mark_block_used(e2fsck_t ctx, blk_t block)
{
	struct		problem_context pctx;
	
	clear_problem_context(&pctx);
	
	if (ext2fs_fast_test_block_bitmap(ctx->block_found_map, block)) {
		if (!ctx->block_dup_map) {
			pctx.errcode = ext2fs_allocate_block_bitmap(ctx->fs,
			      "multiply claimed block map",
			      &ctx->block_dup_map);
			if (pctx.errcode) {
				pctx.num = 3;
				fix_problem(ctx, PR_1_ALLOCATE_BBITMAP_ERROR, 
					    &pctx);
				/* Should never get here */
				ctx->flags |= E2F_FLAG_ABORT;
				return;
			}
		}
		ext2fs_fast_mark_block_bitmap(ctx->block_dup_map, block);
	} else {
		ext2fs_fast_mark_block_bitmap(ctx->block_found_map, block);
	}
}

/*
 * This subroutine is called on each inode to account for all of the
 * blocks used by that inode.
 */
static void check_blocks(e2fsck_t ctx, struct problem_context *pctx,
			 char *block_buf)
{
	ext2_filsys fs = ctx->fs;
	struct process_block_struct pb;
	ino_t		ino = pctx->ino;
	struct ext2_inode *inode = pctx->inode;
	int		bad_size = 0;
	__u64		size;
	struct ext2fs_sb	*sb;
	
	if (!ext2fs_inode_has_valid_blocks(pctx->inode))
		return;
	
	pb.ino = ino;
	pb.num_blocks = pb.last_block = 0;
	pb.num_illegal_blocks = 0;
	pb.suppress = 0; pb.clear = 0;
	pb.fragmented = 0;
	pb.previous_block = 0;
	pb.is_dir = LINUX_S_ISDIR(pctx->inode->i_mode);
	pb.inode = inode;
	pb.pctx = pctx;
	pb.ctx = ctx;
	pctx->ino = ino;
	pctx->errcode = ext2fs_block_iterate2(fs, ino,
				       pb.is_dir ? BLOCK_FLAG_HOLE : 0,
				       block_buf, process_block, &pb);
	if (ctx->flags & E2F_FLAG_SIGNAL_MASK)
		return;
	end_problem_latch(ctx, PR_LATCH_BLOCK);
	if (pctx->errcode)
		fix_problem(ctx, PR_1_BLOCK_ITERATE, pctx);

	if (pb.fragmented && pb.num_blocks < fs->super->s_blocks_per_group)
		ctx->fs_fragmented++;

	if (pb.clear) {
		e2fsck_read_inode(ctx, ino, inode, "check_blocks");
		inode->i_links_count = 0;
		ext2fs_icount_store(ctx->inode_link_info, ino, 0);
		inode->i_dtime = time(0);
		e2fsck_write_inode(ctx, ino, inode, "check_blocks");
		ext2fs_unmark_inode_bitmap(ctx->inode_dir_map, ino);
		ext2fs_unmark_inode_bitmap(ctx->inode_reg_map, ino);
		ext2fs_unmark_inode_bitmap(ctx->inode_used_map, ino);
		/*
		 * The inode was probably partially accounted for
		 * before processing was aborted, so we need to
		 * restart the pass 1 scan.
		 */
		ctx->flags |= E2F_FLAG_RESTART;
		return;
	}

	pb.num_blocks *= (fs->blocksize / 512);
#if 0
	printf("inode %u, i_size = %lu, last_block = %lld, i_blocks=%lu, num_blocks = %lu\n",
	       ino, inode->i_size, pb.last_block, inode->i_blocks,
	       pb.num_blocks);
#endif
	if (!pb.num_blocks && pb.is_dir) {
		if (fix_problem(ctx, PR_1_ZERO_LENGTH_DIR, pctx)) {
			inode->i_links_count = 0;
			ext2fs_icount_store(ctx->inode_link_info, ino, 0);
			inode->i_dtime = time(0);
			e2fsck_write_inode(ctx, ino, inode, "check_blocks");
			ext2fs_unmark_inode_bitmap(ctx->inode_dir_map, ino);
			ext2fs_unmark_inode_bitmap(ctx->inode_reg_map, ino);
			ext2fs_unmark_inode_bitmap(ctx->inode_used_map, ino);
			ctx->fs_directory_count--;
			pb.is_dir = 0;
		}
	}
	if (pb.is_dir) {
		int nblock = inode->i_size >> EXT2_BLOCK_SIZE_BITS(fs->super);
		if ((nblock > (pb.last_block + 1)) ||
		    ((inode->i_size & (fs->blocksize-1)) != 0))
			bad_size = 1;
		else if (nblock < (pb.last_block + 1)) {
			sb = (struct ext2fs_sb *) fs->super;
			if (((pb.last_block + 1) - nblock) >
			    sb->s_prealloc_dir_blocks)
				bad_size = 2;
		}
	} else {
		size = inode->i_size + ((__u64) inode->i_size_high << 32);
		if ((size < pb.last_block * fs->blocksize))
			bad_size = 3;
		else if (size > ext2_max_sizes[fs->super->s_log_block_size])
			bad_size = 4;
	}
	if (bad_size) {
		pctx->num = (pb.last_block+1) * fs->blocksize;
		if (fix_problem(ctx, PR_1_BAD_I_SIZE, pctx)) {
			inode->i_size = pctx->num;
			if (!pb.is_dir)
				inode->i_size_high = pctx->num >> 32;
			e2fsck_write_inode(ctx, ino, inode, "check_blocks");
		}
		pctx->num = 0;
	}
	if (!pb.is_dir && inode->i_size_high)
		ctx->large_files++;
	if (pb.num_blocks != inode->i_blocks) {
		pctx->num = pb.num_blocks;
		if (fix_problem(ctx, PR_1_BAD_I_BLOCKS, pctx)) {
			inode->i_blocks = pb.num_blocks;
			e2fsck_write_inode(ctx, ino, inode, "check_blocks");
		}
		pctx->num = 0;
	}
}

#if 0
/*
 * Helper function called by process block when an illegal block is
 * found.  It returns a description about why the block is illegal
 */
static char *describe_illegal_block(ext2_filsys fs, blk_t block)
{
	blk_t	super;
	int	i;
	static char	problem[80];

	super = fs->super->s_first_data_block;
	strcpy(problem, "PROGRAMMING ERROR: Unknown reason for illegal block");
	if (block < super) {
		sprintf(problem, "< FIRSTBLOCK (%u)", super);
		return(problem);
	} else if (block >= fs->super->s_blocks_count) {
		sprintf(problem, "> BLOCKS (%u)", fs->super->s_blocks_count);
		return(problem);
	}
	for (i = 0; i < fs->group_desc_count; i++) {
		if (block == super) {
			sprintf(problem, "is the superblock in group %d", i);
			break;
		}
		if (block > super &&
		    block <= (super + fs->desc_blocks)) {
			sprintf(problem, "is in the group descriptors "
				"of group %d", i);
			break;
		}
		if (block == fs->group_desc[i].bg_block_bitmap) {
			sprintf(problem, "is the block bitmap of group %d", i);
			break;
		}
		if (block == fs->group_desc[i].bg_inode_bitmap) {
			sprintf(problem, "is the inode bitmap of group %d", i);
			break;
		}
		if (block >= fs->group_desc[i].bg_inode_table &&
		    (block < fs->group_desc[i].bg_inode_table
		     + fs->inode_blocks_per_group)) {
			sprintf(problem, "is in the inode table of group %d",
				i);
			break;
		}
		super += fs->super->s_blocks_per_group;
	}
	return(problem);
}
#endif

/*
 * This is a helper function for check_blocks().
 */
int process_block(ext2_filsys fs,
		  blk_t	*block_nr,
		  e2_blkcnt_t blockcnt,
		  blk_t ref_block,
		  int ref_offset, 
		  void *priv_data)
{
	struct process_block_struct *p;
	struct problem_context *pctx;
	blk_t	blk = *block_nr;
	int	ret_code = 0;
	int	problem = 0;
	e2fsck_t	ctx;

	p = (struct process_block_struct *) priv_data;
	pctx = p->pctx;
	ctx = p->ctx;

	if (blk == 0) {
		if (p->is_dir == 0) {
			/*
			 * Should never happen, since only directories
			 * get called with BLOCK_FLAG_HOLE
			 */
#if DEBUG_E2FSCK
			printf("process_block() called with blk == 0, "
			       "blockcnt=%d, inode %lu???\n",
			       blockcnt, p->ino);
#endif
			return 0;
		}
		if (blockcnt < 0)
			return 0;
		if (blockcnt * fs->blocksize < p->inode->i_size) {
#if 0
			printf("Missing block (#%d) in directory inode %lu!\n",
			       blockcnt, p->ino);
#endif
			goto mark_dir;
		}
		return 0;
	}

#if 0
	printf("Process_block, inode %lu, block %u, #%d\n", p->ino, blk,
	       blockcnt);
#endif
	
	/*
	 * Simplistic fragmentation check.  We merely require that the
	 * file be contiguous.  (Which can never be true for really
	 * big files that are greater than a block group.)
	 */
	if (p->previous_block) {
		if (p->previous_block+1 != blk)
			p->fragmented = 1;
	}
	p->previous_block = blk;
	
	if (blk < fs->super->s_first_data_block ||
	    blk >= fs->super->s_blocks_count)
		problem = PR_1_ILLEGAL_BLOCK_NUM;
#if 0
	else
		if (ext2fs_test_block_bitmap(block_illegal_map, blk))
			problem = PR_1_BLOCK_OVERLAPS_METADATA;
#endif

	if (problem) {
		p->num_illegal_blocks++;
		if (!p->suppress && (p->num_illegal_blocks % 12) == 0) {
			if (fix_problem(ctx, PR_1_TOO_MANY_BAD_BLOCKS, pctx)) {
				p->clear = 1;
				return BLOCK_ABORT;
			}
			if (fix_problem(ctx, PR_1_SUPPRESS_MESSAGES, pctx)) {
				p->suppress = 1;
				set_latch_flags(PR_LATCH_BLOCK,
						PRL_SUPPRESS, 0);
			}
		}
		pctx->blk = blk;
		pctx->blkcount = blockcnt;
		if (fix_problem(ctx, problem, pctx)) {
			blk = *block_nr = 0;
			ret_code = BLOCK_CHANGED;
			goto mark_dir;
		} else
			return 0;
		pctx->blk = 0;
		pctx->blkcount = -1;
	}

	mark_block_used(ctx, blk);
	p->num_blocks++;
	if (blockcnt >= 0)
		p->last_block = blockcnt;
mark_dir:
	if (p->is_dir && (blockcnt >= 0)) {
		pctx->errcode = ext2fs_add_dir_block(fs->dblist, p->ino,
						    blk, blockcnt);
		if (pctx->errcode) {
			pctx->blk = blk;
			pctx->num = blockcnt;
			fix_problem(ctx, PR_1_ADD_DBLOCK, pctx);
			/* Should never get here */
			ctx->flags |= E2F_FLAG_ABORT;
			return BLOCK_ABORT;
		}
	}
	return ret_code;
}

static void bad_block_indirect(e2fsck_t ctx, blk_t blk)
{
	struct problem_context pctx;

	clear_problem_context(&pctx);
	/*
	 * Prompt to see if we should continue or not.
	 */
	if (!fix_problem(ctx, PR_1_BBINODE_BAD_METABLOCK, &pctx))
		ctx->flags |= E2F_FLAG_ABORT;
}

int process_bad_block(ext2_filsys fs,
		      blk_t *block_nr,
		      e2_blkcnt_t blockcnt,
		      blk_t ref_block,
		      int ref_offset,
		      void *priv_data)
{
	struct process_block_struct *p;
	blk_t		blk = *block_nr;
	int		first_block;
	int		i;
	struct problem_context *pctx;
	e2fsck_t	ctx;

	if (!blk)
		return 0;
	
	p = (struct process_block_struct *) priv_data;
	ctx = p->ctx;
	pctx = p->pctx;
	
	pctx->ino = EXT2_BAD_INO;
	pctx->blk = blk;
	pctx->blkcount = blockcnt;

	if ((blk < fs->super->s_first_data_block) ||
	    (blk >= fs->super->s_blocks_count)) {
		if (fix_problem(ctx, PR_1_BB_ILLEGAL_BLOCK_NUM, pctx)) {
			*block_nr = 0;
			return BLOCK_CHANGED;
		} else
			return 0;
	}

	if (blockcnt < 0) {
		if (ext2fs_test_block_bitmap(ctx->block_found_map, blk)) {
			bad_block_indirect(ctx, blk);
			if (ctx->flags & E2F_FLAG_SIGNAL_MASK)
				return BLOCK_ABORT;
		} else
			mark_block_used(ctx, blk);
		return 0;
	}
#if 0 
	printf ("DEBUG: Marking %u as bad.\n", blk);
#endif
	ctx->fs_badblocks_count++;
	/*
	 * If the block is not used, then mark it as used and return.
	 * If it is already marked as found, this must mean that
	 * there's an overlap between the filesystem table blocks
	 * (bitmaps and inode table) and the bad block list.
	 */
	if (!ext2fs_test_block_bitmap(ctx->block_found_map, blk)) {
		ext2fs_mark_block_bitmap(ctx->block_found_map, blk);
		return 0;
	}
	/*
	 * Try to find the where the filesystem block was used...
	 */
	first_block = fs->super->s_first_data_block;
	
	for (i = 0; i < fs->group_desc_count; i++ ) {
		pctx->group = i;
		pctx->blk = blk;
		if (!ext2fs_bg_has_super(fs, i))
			goto skip_super;
		if (blk == first_block) {
			if (i == 0) {
				if (fix_problem(ctx,
						PR_1_BAD_PRIMARY_SUPERBLOCK,
						pctx)) {
					*block_nr = 0;
					return BLOCK_CHANGED;
				}
				return 0;
			}
			fix_problem(ctx, PR_1_BAD_SUPERBLOCK, pctx);
			return 0;
		}
		if ((blk > first_block) &&
		    (blk <= first_block + fs->desc_blocks)) {
			if (i == 0) {
				pctx->blk = *block_nr;
				if (fix_problem(ctx,
			PR_1_BAD_PRIMARY_GROUP_DESCRIPTOR, pctx)) {
					*block_nr = 0;
					return BLOCK_CHANGED;
				}
				return 0;
			}
			fix_problem(ctx, PR_1_BAD_GROUP_DESCRIPTORS, pctx);
			return 0;
		}
	skip_super:
		if (blk == fs->group_desc[i].bg_block_bitmap) {
			if (fix_problem(ctx, PR_1_BB_BAD_BLOCK, pctx)) {
				ctx->invalid_block_bitmap_flag[i]++;
				ctx->invalid_bitmaps++;
			}
			return 0;
		}
		if (blk == fs->group_desc[i].bg_inode_bitmap) {
			if (fix_problem(ctx, PR_1_IB_BAD_BLOCK, pctx)) {
				ctx->invalid_inode_bitmap_flag[i]++;
				ctx->invalid_bitmaps++;
			}
			return 0;
		}
		if ((blk >= fs->group_desc[i].bg_inode_table) &&
		    (blk < (fs->group_desc[i].bg_inode_table +
			    fs->inode_blocks_per_group))) {
			/*
			 * If there are bad blocks in the inode table,
			 * the inode scan code will try to do
			 * something reasonable automatically.
			 */
			return 0;
		}
		first_block += fs->super->s_blocks_per_group;
	}
	/*
	 * If we've gotten to this point, then the only
	 * possibility is that the bad block inode meta data
	 * is using a bad block.
	 */
	if ((blk == p->inode->i_block[EXT2_IND_BLOCK]) ||
	    p->inode->i_block[EXT2_DIND_BLOCK]) {
		bad_block_indirect(ctx, blk);
		if (ctx->flags & E2F_FLAG_SIGNAL_MASK)
			return BLOCK_ABORT;
		return 0;
	}

	pctx->group = -1;

	/* Warn user that the block wasn't claimed */
	fix_problem(ctx, PR_1_PROGERR_CLAIMED_BLOCK, pctx);

	return 0;
}

static void new_table_block(e2fsck_t ctx, blk_t first_block, int group, 
			    const char *name, int num, blk_t *new_block)
{
	ext2_filsys fs = ctx->fs;
	blk_t		old_block = *new_block;
	int		i;
	char		*buf;
	struct problem_context	pctx;

	clear_problem_context(&pctx);

	pctx.group = group;
	pctx.blk = old_block;
	pctx.str = name;

	pctx.errcode = ext2fs_get_free_blocks(fs, first_block,
			first_block + fs->super->s_blocks_per_group,
					num, ctx->block_found_map, new_block);
	if (pctx.errcode) {
		pctx.num = num;
		fix_problem(ctx, PR_1_RELOC_BLOCK_ALLOCATE, &pctx);
		ext2fs_unmark_valid(fs);
		return;
	}
	pctx.errcode = ext2fs_get_mem(fs->blocksize, (void **) &buf);
	if (pctx.errcode) {
		fix_problem(ctx, PR_1_RELOC_MEMORY_ALLOCATE, &pctx);
		ext2fs_unmark_valid(fs);
		return;
	}
	ext2fs_mark_super_dirty(fs);
	pctx.blk2 = *new_block;
	fix_problem(ctx, (old_block ? PR_1_RELOC_FROM_TO :
			  PR_1_RELOC_TO), &pctx);
	pctx.blk2 = 0;
	for (i = 0; i < num; i++) {
		pctx.blk = i;
		ext2fs_mark_block_bitmap(ctx->block_found_map, (*new_block)+i);
		if (old_block) {
			pctx.errcode = io_channel_read_blk(fs->io,
				   old_block + i, 1, buf);
			if (pctx.errcode)
				fix_problem(ctx, PR_1_RELOC_READ_ERR, &pctx);
		} else
			memset(buf, 0, fs->blocksize);

		pctx.blk = (*new_block) + i;
		pctx.errcode = io_channel_write_blk(fs->io, pctx.blk,
					      1, buf);
		if (pctx.errcode)
			fix_problem(ctx, PR_1_RELOC_WRITE_ERR, &pctx);
	}
	ext2fs_free_mem((void **) &buf);
}

/*
 * This routine gets called at the end of pass 1 if bad blocks are
 * detected in the superblock, group descriptors, inode_bitmaps, or
 * block bitmaps.  At this point, all of the blocks have been mapped
 * out, so we can try to allocate new block(s) to replace the bad
 * blocks.
 */
static void handle_fs_bad_blocks(e2fsck_t ctx)
{
	ext2_filsys fs = ctx->fs;
	int		i;
	int		first_block = fs->super->s_first_data_block;

	for (i = 0; i < fs->group_desc_count; i++) {
		if (ctx->invalid_block_bitmap_flag[i]) {
			new_table_block(ctx, first_block, i, "block bitmap", 
					1, &fs->group_desc[i].bg_block_bitmap);
		}
		if (ctx->invalid_inode_bitmap_flag[i]) {
			new_table_block(ctx, first_block, i, "inode bitmap", 
					1, &fs->group_desc[i].bg_inode_bitmap);
		}
		if (ctx->invalid_inode_table_flag[i]) {
			new_table_block(ctx, first_block, i, "inode table",
					fs->inode_blocks_per_group, 
					&fs->group_desc[i].bg_inode_table);
			ctx->flags |= E2F_FLAG_RESTART;
		}
		first_block += fs->super->s_blocks_per_group;
	}
	ctx->invalid_bitmaps = 0;
}

/*
 * This routine marks all blocks which are used by the superblock,
 * group descriptors, inode bitmaps, and block bitmaps.
 */
static void mark_table_blocks(e2fsck_t ctx)
{
	ext2_filsys fs = ctx->fs;
	blk_t	block, b;
	int	i,j;
	struct problem_context pctx;
	
	clear_problem_context(&pctx);
	
	block = fs->super->s_first_data_block;
	for (i = 0; i < fs->group_desc_count; i++) {
		pctx.group = i;

		if (ext2fs_bg_has_super(fs, i)) {
			/*
			 * Mark this group's copy of the superblock
			 */
			ext2fs_mark_block_bitmap(ctx->block_found_map, block);
			ext2fs_mark_block_bitmap(ctx->block_illegal_map,
						 block);
		
			/*
			 * Mark this group's copy of the descriptors
			 */
			for (j = 0; j < fs->desc_blocks; j++) {
				ext2fs_mark_block_bitmap(ctx->block_found_map,
							 block + j + 1);
				ext2fs_mark_block_bitmap(ctx->block_illegal_map,
							 block + j + 1);
			}
		}
		
		/*
		 * Mark the blocks used for the inode table
		 */
		if (fs->group_desc[i].bg_inode_table) {
			for (j = 0, b = fs->group_desc[i].bg_inode_table;
			     j < fs->inode_blocks_per_group;
			     j++, b++) {
				if (ext2fs_test_block_bitmap(ctx->block_found_map,
							     b)) {
					pctx.blk = b;
					if (fix_problem(ctx,
						PR_1_ITABLE_CONFLICT, &pctx)) {
						ctx->invalid_inode_table_flag[i]++;
						ctx->invalid_bitmaps++;
					}
				} else {
				    ext2fs_mark_block_bitmap(ctx->block_found_map,
							     b);
				    ext2fs_mark_block_bitmap(ctx->block_illegal_map,
							     b);
			    	}
			}
		}
			    
		/*
		 * Mark block used for the block bitmap 
		 */
		if (fs->group_desc[i].bg_block_bitmap) {
			if (ext2fs_test_block_bitmap(ctx->block_found_map,
				     fs->group_desc[i].bg_block_bitmap)) {
				pctx.blk = fs->group_desc[i].bg_block_bitmap;
				if (fix_problem(ctx, PR_1_BB_CONFLICT, &pctx)) {
					ctx->invalid_block_bitmap_flag[i]++;
					ctx->invalid_bitmaps++;
				}
			} else {
			    ext2fs_mark_block_bitmap(ctx->block_found_map,
				     fs->group_desc[i].bg_block_bitmap);
			    ext2fs_mark_block_bitmap(ctx->block_illegal_map,
				     fs->group_desc[i].bg_block_bitmap);
		    }
			
		}
		/*
		 * Mark block used for the inode bitmap 
		 */
		if (fs->group_desc[i].bg_inode_bitmap) {
			if (ext2fs_test_block_bitmap(ctx->block_found_map,
				     fs->group_desc[i].bg_inode_bitmap)) {
				pctx.blk = fs->group_desc[i].bg_inode_bitmap;
				if (fix_problem(ctx, PR_1_IB_CONFLICT, &pctx)) {
					ctx->invalid_inode_bitmap_flag[i]++;
					ctx->invalid_bitmaps++;
				} 
			} else {
			    ext2fs_mark_block_bitmap(ctx->block_found_map,
				     fs->group_desc[i].bg_inode_bitmap);
			    ext2fs_mark_block_bitmap(ctx->block_illegal_map,
				     fs->group_desc[i].bg_inode_bitmap);
			}
		}
		block += fs->super->s_blocks_per_group;
	}
}
	
/*
 * Thes subroutines short circuits ext2fs_get_blocks and
 * ext2fs_check_directory; we use them since we already have the inode
 * structure, so there's no point in letting the ext2fs library read
 * the inode again.
 */
static errcode_t pass1_get_blocks(ext2_filsys fs, ino_t ino, blk_t *blocks)
{
	e2fsck_t ctx = (e2fsck_t) fs->priv_data;
	int	i;
	
	if (ino != ctx->stashed_ino)
		return EXT2_ET_CALLBACK_NOTHANDLED;

	for (i=0; i < EXT2_N_BLOCKS; i++)
		blocks[i] = ctx->stashed_inode->i_block[i];
	return 0;
}

static errcode_t pass1_read_inode(ext2_filsys fs, ino_t ino,
				  struct ext2_inode *inode)
{
	e2fsck_t ctx = (e2fsck_t) fs->priv_data;

	if (ino != ctx->stashed_ino)
		return EXT2_ET_CALLBACK_NOTHANDLED;
	*inode = *ctx->stashed_inode;
	return 0;
}

static errcode_t pass1_write_inode(ext2_filsys fs, ino_t ino,
			    struct ext2_inode *inode)
{
	e2fsck_t ctx = (e2fsck_t) fs->priv_data;

	if (ino == ctx->stashed_ino)
		*ctx->stashed_inode = *inode;
	return EXT2_ET_CALLBACK_NOTHANDLED;
}

static errcode_t pass1_check_directory(ext2_filsys fs, ino_t ino)
{
	e2fsck_t ctx = (e2fsck_t) fs->priv_data;

	if (ino != ctx->stashed_ino)
		return EXT2_ET_CALLBACK_NOTHANDLED;

	if (!LINUX_S_ISDIR(ctx->stashed_inode->i_mode))
		return EXT2_ET_NO_DIRECTORY;
	return 0;
}

void e2fsck_use_inode_shortcuts(e2fsck_t ctx, int bool)
{
	ext2_filsys fs = ctx->fs;

	if (bool) {
		fs->get_blocks = pass1_get_blocks;
		fs->check_directory = pass1_check_directory;
		fs->read_inode = pass1_read_inode;
		fs->write_inode = pass1_write_inode;
		ctx->stashed_ino = 0;
	} else {
		fs->get_blocks = 0;
		fs->check_directory = 0;
		fs->read_inode = 0;
		fs->write_inode = 0;
	}
}

		
