/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 *  Copyright (C) 2002 Cluster File Systems, Inc.
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
 *   Top level header file for LProc SNMP
 */
#ifndef _LPROCFS_SNMP_H
#define _LPROCFS_SNMP_H

#ifndef LPROC_SNMP
#define LPROC_SNMP
#endif

#include <linux/proc_fs.h>
#include <linux/lustre_lite.h>

typedef struct lprocfs_vars{
        char* name;
        read_proc_t* read_fptr;
        write_proc_t* write_fptr;
} lprocfs_vars_t;

#ifdef LPROC_SNMP

struct proc_dir_entry* lprocfs_mkdir(const char* dname,
                                     struct proc_dir_entry *parent);
struct proc_dir_entry* lprocfs_srch(struct proc_dir_entry* head,
                                    const char* name);
void lprocfs_remove_all(struct proc_dir_entry* root);
struct proc_dir_entry* lprocfs_new_dir(struct proc_dir_entry* root,
                                       const char* string, 
                                       const char* tok);
int lprocfs_new_vars(struct proc_dir_entry* root,
                     lprocfs_vars_t* list,
                     const char* tok, 
                     void* data);
int lprocfs_add_vars(struct proc_dir_entry* root,
                     lprocfs_vars_t* var, 
                     void* data);
int lprocfs_reg_obd(struct obd_device* device, 
                    lprocfs_vars_t* list, 
                    void* data);
int lprocfs_dereg_obd(struct obd_device* device);
int lprocfs_reg_mnt(struct ll_sb_info* sb, 
                    lprocfs_vars_t* list, 
                    void* data);
int lprocfs_dereg_mnt(struct ll_sb_info* sb);
int lprocfs_reg_class(struct obd_type* type,
                      lprocfs_vars_t* list, 
                      void* data);
int lprocfs_dereg_class(struct obd_type* class);
int lprocfs_reg_main(void);
int lprocfs_dereg_main(void);
int lprocfs_ll_rd(char *page, char **start, off_t off,
                  int count, int *eof, void *data);
#else


static inline int lprocfs_add_vars(struct proc_dir_entry* root,
				   lprocfs_vars_t* var, 
				   void* data)
{
        return 0;
}

static inline int lprocfs_reg_obd(struct obd_device* device, 
				  lprocfs_vars_t* list, 
				  void* data)
{
        return 0;
}

static inline int lprocfs_dereg_obd(struct obd_device* device)
{

        return 0;
}

static inline lprocfs_reg_mnt(struct ll_sb_info* sb, 
                              lprocfs_vars_t* list, 
                              void* data)
{
        return 0;
        
}

static inline int lprocfs_dereg_mnt(struct ll_sb_info* sb)
{
        return 0;
}

static inline int lprocfs_reg_class(struct obd_type* type,
                                                       lprocfs_vars_t* list, 
                                                       void* data)
{
        return 0;
}

static inline int lprocfs_dereg_class(struct obd_type* class)
{
        return 0;

}
static inline int lprocfs_reg_main()
{
        return 0;

}

static inline int lprocfs_dereg_main()
{
        return 0;
}

static inline int lprocfs_ll_rd(char *page, char **start, off_t off,
                                int count, int *eof, void *data)
{
        return 0;
}
#endif /* LPROC_SNMP */

#endif /* LPROCFS_SNMP_H */
