/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 *  Copyright (C) 2001 Cluster File Systems, Inc. <braam@clusterfs.com>
 *
 *   This file is part of Lustre, http://www.lustre.org.
 *
 *   Lustre is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License as published by the Free Software Foundation.
 *
 *   Lustre is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Lustre; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * MDS data structures.  
 * See also lustre_idl.h for wire formats of requests.
 *
 */

#ifndef _LUSTRE_MDS_H
#define _LUSTRE_MDS_H

#include <linux/obd_support.h>


struct mds_run_ctxt { 
	struct vfsmount *pwdmnt;
	struct dentry   *pwd;
	mm_segment_t     fs;
};

#define MDS_UNMOUNT 1
#define LUSTRE_MDS_NAME "mds"

struct mds_obd {
	char *mds_fstype;
	struct task_struct *mds_thread;
	wait_queue_head_t mds_waitq;
	wait_queue_head_t mds_done_waitq;
	struct timer_list *mds_timer;
	int mds_interval; 
	int mds_flags;
	struct list_head mds_reqs;
        struct super_block * mds_sb;
	struct vfsmount *mds_vfsmnt;
	struct mds_run_ctxt  mds_ctxt;
	spinlock_t mds_lock;
	__u64 mds_lastino;
	struct file_operations *mds_fop; 
	struct inode_operations *mds_iop;
	struct address_space_operations *mds_aops;
};


struct mds_request { 
	struct list_head rq_list;
	struct mds_obd *rq_obd;
	int rq_status;

	char *rq_reqbuf;
	int rq_reqlen;
	struct mds_req_hdr *rq_reqhdr;
	struct mds_req *rq_req;

	char *rq_repbuf;
	int rq_replen;
	struct mds_rep_hdr *rq_rephdr;
	struct mds_rep *rq_rep;

        void * rq_reply_handle;
	wait_queue_head_t rq_wait_for_rep;
};


/* more or less identical to the packed structure, except for the pointers */
struct mds_req {
	struct ll_fid        fid1;
	struct ll_fid        fid2;
        int                        namelen;
        int                        tgtlen;
        __u32                       opcode;
        __u32                       valid;
        __u32 			    mode;
        __u32                       uid;
        __u32                       gid;
        __u64                       size;
        __u32                       mtime;
        __u32                       ctime;
        __u32                       atime;
        __u32                       flags;
        __u32                       major;
        __u32                       minor;
        __u32                       ino;
        __u32                       nlink;
        __u32                       generation;
};

/* more or less identical to the packed structure, except for the pointers */
struct mds_rep {
	struct ll_fid        fid1;
	struct ll_fid        fid2;
        int                        namelen;
        int                        tgtlen;
        __u32                       valid;
        __u32 			    mode;
        __u32                       uid;
        __u32                       gid;
        __u64                       size;
        __u32                       mtime;
        __u32                       ctime;
        __u32                       atime;
        __u32                       flags;
        __u32                       major;
        __u32                       minor;
        __u32                       ino;
        __u32                       nlink;
        __u32                       generation;
};

struct mds_update_record { 
        __u32 ur_reclen;
        __u32 ur_opcode;
        struct ll_fid *ur_fid1;
        struct ll_fid *ur_fid2;
        int ur_namelen;
        char *ur_name;
        int ur_tgtlen;
        char *ur_tgt;
        struct iattr ur_iattr;
        __u64 ur_id;
        __u32 ur_mode;
        __u32 ur_uid;
        __u32 ur_gid;
        __u64 ur_time;
}; 

/* mds/mds_pack.c */
void *mds_req_tgt(struct mds_req *req);
int mds_pack_req(char *name, int namelen, char *tgt, int tgtlen, struct mds_req_hdr **hdr, struct mds_req **req, int *len, char **buf);
int mds_unpack_req(char *buf, int len, struct mds_req_hdr **hdr, struct mds_req **req);
int mds_pack_rep(char *name, int namelen, char *tgt, int tgtlen, struct mds_rep_hdr **hdr, struct mds_rep **rep, int *len, char **buf);
int mds_unpack_rep(char *buf, int len, struct mds_rep_hdr **hdr, struct mds_rep **rep);

/* mds/mds_reint.c  */
int mds_reint_rec(struct mds_update_record *r, struct mds_request *req); 

/* lib/mds_updates.c */
int mds_update_unpack(char *buf, int len, struct mds_update_record *r); 

void mds_setattr_pack(struct mds_rec_setattr *rec, struct inode *inode, struct iattr *iattr);
void mds_create_pack(struct mds_rec_create *rec, struct inode *inode, char *name, __u32 mode, __u64 id, __u32 uid, __u32 gid, __u64 time);

/* mds/handler.c */
struct dentry *mds_fid2dentry(struct mds_obd *mds, struct ll_fid *fid, struct vfsmount **mnt);


/* llight/request.c */
int mdc_getattr(ino_t ino, int type, int valid, 
		struct mds_rep  **mds_reply, struct mds_rep_hdr **hdr);
int mdc_setattr(struct inode *inode, struct iattr *iattr, 
		struct mds_rep  **mds_reply, struct mds_rep_hdr **hdr);
int mdc_readpage(ino_t ino, int type, __u64 offset, char *addr, 
                 struct mds_rep  **rep, struct mds_rep_hdr **hdr);
int mdc_create(struct inode *dir, char *name, int mode, __u64 id, 
               __u32 uid, __u32 gid, __u64 time, 
               struct mds_rep **rep, struct mds_rep_hdr **hdr);



/* ioctls for trying requests */
#define IOC_REQUEST_TYPE                   'f'
#define IOC_REQUEST_MIN_NR                 30

#define IOC_REQUEST_GETATTR		_IOWR('f', 30, long)
#define IOC_REQUEST_READPAGE		_IOWR('f', 31, long)
#define IOC_REQUEST_SETATTR		_IOWR('f', 32, long)
#define IOC_REQUEST_CREATE		_IOWR('f', 33, long)
#define IOC_REQUEST_MAX_NR               33

#endif


