/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) 2003 Cluster File Systems, Inc.
 *   Author: Eric Barton <eeb@clusterfs.com>
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
 */

#ifndef EXPORT_SYMTAB
# define EXPORT_SYMTAB
#endif

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/kernel.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/smp_lock.h>
#include <linux/unistd.h>
#include <linux/interrupt.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#define DEBUG_SUBSYSTEM S_PORTALS

#include <linux/kp30.h>

#if LWT_SUPPORT

#define LWT_MEMORY              (1<<20)         /* 1Mb of trace memory */
#define LWT_MAX_CPUS             4

int         lwt_enabled;
int         lwt_pages_per_cpu;
lwt_cpu_t   lwt_cpus[LWT_MAX_CPUS];

/* NB only root is allowed to retrieve LWT info; it's an open door into the
 * kernel... */

int
lwt_lookup_string (int *size, char *knl_ptr,
                   char *user_ptr, int user_size)
{
        /* knl_ptr was retrieved from an LWT snapshot and the caller wants to
         * turn it into a string.  NB we can crash with an access violation
         * trying to determine the string length, so we're trusting our
         * caller... */

        if (!capable(CAP_SYS_ADMIN))
                return (-EPERM);

        *size = strlen (knl_ptr) + 1;
        
        if (user_ptr != NULL &&
            copy_to_user (user_ptr, knl_ptr, *size))
                return (-EFAULT);
        
        return (0);
}

int
lwt_control (int enable, int clear)
{
        lwt_page_t  *p;
        int          i;
        int          j;

        if (!capable(CAP_SYS_ADMIN))
                return (-EPERM);

        if (clear)
                for (i = 0; i < num_online_cpus(); i++) {
                        p = lwt_cpus[i].lwtc_current_page;

                        for (j = 0; j < lwt_pages_per_cpu; j++) {
                                memset (p->lwtp_events, 0, PAGE_SIZE);

                                p = list_entry (p->lwtp_list.next,
                                                lwt_page_t, lwtp_list);
                        }
        }

        lwt_enabled = enable;
        mb();
        if (!enable) {
                /* give people some time to stop adding traces */
                schedule_timeout(10);
        }

        return (0);
}

int
lwt_snapshot (int *ncpu, int *total_size, void *user_ptr, int user_size)
{
        const int    events_per_page = PAGE_SIZE / sizeof(lwt_event_t);
        const int    bytes_per_page = events_per_page * sizeof(lwt_event_t);
        lwt_page_t  *p;
        int          i;
        int          j;

        if (!capable(CAP_SYS_ADMIN))
                return (-EPERM);

        *ncpu = num_online_cpus();
        *total_size = num_online_cpus() * lwt_pages_per_cpu * bytes_per_page;

        if (user_ptr == NULL)
                return (0);

        for (i = 0; i < num_online_cpus(); i++) {
                p = lwt_cpus[i].lwtc_current_page;
                
                for (j = 0; j < lwt_pages_per_cpu; j++) {
                        if (copy_to_user(user_ptr, p->lwtp_events,
                                         bytes_per_page))
                                return (-EFAULT);

                        user_ptr = ((char *)user_ptr) + bytes_per_page;
                        p = list_entry(p->lwtp_list.next,
                                       lwt_page_t, lwtp_list);
                        
                }
        }

        return (0);
}

int
lwt_init () 
{
	int     i;
        int     j;
        
        if (num_online_cpus() > LWT_MAX_CPUS) {
                CERROR ("Too many CPUs\n");
                return (-EINVAL);
        }

	/* NULL pointers, zero scalars */
	memset (lwt_cpus, 0, sizeof (lwt_cpus));
        lwt_pages_per_cpu = LWT_MEMORY / (num_online_cpus() * PAGE_SIZE);

	for (i = 0; i < num_online_cpus(); i++)
		for (j = 0; j < lwt_pages_per_cpu; j++) {
			struct page *page = alloc_page (GFP_KERNEL);
			lwt_page_t  *lwtp;

			if (page == NULL) {
				CERROR ("Can't allocate page\n");
                                lwt_fini ();
				return (-ENOMEM);
			}

                        PORTAL_ALLOC(lwtp, sizeof (*lwtp));
			if (lwtp == NULL) {
				CERROR ("Can't allocate lwtp\n");
                                __free_page(page);
				lwt_fini ();
				return (-ENOMEM);
			}

                        lwtp->lwtp_page = page;
                        lwtp->lwtp_events = page_address(page);
			memset (lwtp->lwtp_events, 0, PAGE_SIZE);

			if (j == 0) {
				INIT_LIST_HEAD (&lwtp->lwtp_list);
				lwt_cpus[i].lwtc_current_page = lwtp;
			} else {
				list_add (&lwtp->lwtp_list,
				    &lwt_cpus[i].lwtc_current_page->lwtp_list);
			}
                }

        lwt_enabled = 1;
        mb();

        return (0);
}

void
lwt_fini () 
{
        int    i;
        
        if (num_online_cpus() > LWT_MAX_CPUS)
                return;

        for (i = 0; i < num_online_cpus(); i++)
                while (lwt_cpus[i].lwtc_current_page != NULL) {
                        lwt_page_t *lwtp = lwt_cpus[i].lwtc_current_page;
                        
                        if (list_empty (&lwtp->lwtp_list)) {
                                lwt_cpus[i].lwtc_current_page = NULL;
                        } else {
                                lwt_cpus[i].lwtc_current_page =
                                        list_entry (lwtp->lwtp_list.next,
                                                    lwt_page_t, lwtp_list);

                                list_del (&lwtp->lwtp_list);
                        }
                        
                        __free_page (lwtp->lwtp_page);
                        PORTAL_FREE (lwtp, sizeof (*lwtp));
                }
}

EXPORT_SYMBOL(lwt_enabled);
EXPORT_SYMBOL(lwt_cpus);

EXPORT_SYMBOL(lwt_init);
EXPORT_SYMBOL(lwt_fini);
EXPORT_SYMBOL(lwt_lookup_string);
EXPORT_SYMBOL(lwt_control);
EXPORT_SYMBOL(lwt_snapshot);
#endif
