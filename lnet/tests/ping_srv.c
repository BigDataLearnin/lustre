/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) 2002, Lawrence Livermore National Labs (LLNL)
 * Author: Brian Behlendorf <behlendorf1@llnl.gov>
 * 	   Amey Inamdar	    <amey@calsoftinc.com>
 * 	   Kedar Sovani	    <kedar@calsoftinc.com>
 *
 *
 * This file is part of Portals, http://www.sf.net/projects/lustre/
 *
 * Portals is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * Portals is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Portals; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define DEBUG_SUBSYSTEM S_PINGER

#include <libcfs/kp30.h>
#include <portals/p30.h>
#include "ping.h"

#define STDSIZE (sizeof(int) + sizeof(int) + sizeof(struct timeval))
#define MAXSIZE (16*1024)

static unsigned ping_head_magic;
static unsigned ping_bulk_magic;
static unsigned long packets_valid = 0;         // Valid packets 
static int running = 1;
atomic_t pkt;
       
static struct pingsrv_data *server=NULL;             // Our ping server

static void *pingsrv_shutdown(int err)
{
        int rc;

        /* Yes, we are intentionally allowing us to fall through each
         * case in to the next.  This allows us to pass an error
         * code to just clean up the right stuff.
         */
        switch (err) {
                case 1:
                        /* Unlink any memory descriptors we may have used */
                        if ((rc = PtlMDUnlink (server->mdin_h)))
                                PDEBUG ("PtlMDUnlink (out head buffer)", rc);
                case 2:
                        /* Free the event queue */
                        if ((rc = PtlEQFree (server->eq)))
                                PDEBUG ("PtlEQFree", rc);

                        /* Unlink the client portal from the ME list */
                        if ((rc = PtlMEUnlink (server->me)))
                                        PDEBUG ("PtlMEUnlink", rc);

                case 3:
                        PtlNIFini (server->ni);

                case 4:
                        
                case 5:
                        if (server->in_buf != NULL)
                                PORTAL_FREE (server->in_buf, MAXSIZE);
                        
                        if (server != NULL)
                                PORTAL_FREE (server, 
                                             sizeof (struct pingsrv_data));
                        
        }

        CDEBUG (D_OTHER, "ping sever resources released\n");
        return NULL;
} /* pingsrv_shutdown() */


int pingsrv_thread(void *arg)
{
        int rc;
        unsigned long magic;
        unsigned long ping_bulk_magic = __cpu_to_le32(0xcafebabe);
        
        kportal_daemonize ("pingsrv");
        server->tsk =  cfs_current();
        
        while (running) {
                set_current_state (TASK_INTERRUPTIBLE);
                if (atomic_read (&pkt) == 0) {
                        schedule_timeout (MAX_SCHEDULE_TIMEOUT);
                        continue;
                }
               
                magic =  __le32_to_cpu(*((int *)(server->evnt.md.start 
                                        + server->evnt.offset)));
                
                
                if(magic != 0xdeadbeef) {
                        CERROR("Unexpected Packet to the server, magic: %lx %d\n", magic, server->evnt.offset);
                        
                } 
                memcpy (server->in_buf, &ping_bulk_magic, sizeof(ping_bulk_magic));
                                
                server->mdout.length    = server->evnt.rlength;
                server->mdout.start     = server->in_buf;
                server->mdout.threshold = 1; 
                server->mdout.options   = PTL_MD_EVENT_START_DISABLE | PTL_MD_OP_PUT;
                server->mdout.user_ptr  = NULL;
                server->mdout.eq_handle = PTL_EQ_NONE;
       
                /* Bind the outgoing buffer */
                if ((rc = PtlMDBind (server->ni, server->mdout, 
                                     PTL_UNLINK, &server->mdout_h))) {
                         PDEBUG ("PtlMDBind", rc);
                         pingsrv_shutdown (1);
                         return 1;
	        }
         
                
                server->mdin.start     = server->in_buf;
                server->mdin.length    = MAXSIZE;
                server->mdin.threshold = 1; 
                server->mdin.options   = PTL_MD_EVENT_START_DISABLE | PTL_MD_OP_PUT;
                server->mdin.user_ptr  = NULL;
                server->mdin.eq_handle = server->eq;
        
                if ((rc = PtlMDAttach (server->me, server->mdin,
                        PTL_UNLINK, &server->mdin_h))) {
                        PDEBUG ("PtlMDAttach (bulk)", rc);
                        CDEBUG (D_OTHER, "ping server resources allocated\n");
                }
                
                if ((rc = PtlPut (server->mdout_h, PTL_NOACK_REQ,
                         server->evnt.initiator, PTL_PING_CLIENT, 0, 0, 0, 0)))
                         PDEBUG ("PtlPut", rc);
                
                atomic_dec (&pkt);
                
        }
        pingsrv_shutdown (1);
        running = 1;
        return 0;    
}

static void pingsrv_packet(ptl_event_t *ev)
{
        atomic_inc (&pkt);
        wake_up_process (server->tsk);
} /* pingsrv_head() */

static void pingsrv_callback(ptl_event_t *ev)
{
        
        if (ev == NULL) {
                CERROR ("null in callback, ev=%p\n", ev);
                return;
        }
        server->evnt = *ev;
        
        CWARN ("received ping from nid "LPX64" "
               "(off=%u rlen=%u mlen=%u head=%x seq=%d size=%d)\n",
               ev->initiator.nid, ev->offset, ev->rlength, ev->mlength,
               __le32_to_cpu(*((int *)(ev->md.start + ev->offset))),
               __le32_to_cpu(*((int *)(ev->md.start + ev->offset + sizeof(unsigned)))),
               __le32_to_cpu(*((int *)(ev->md.start + ev->offset + 2 * 
                               sizeof(unsigned)))));
        
        packets_valid++;

        pingsrv_packet(ev);
        
} /* pingsrv_callback() */


static struct pingsrv_data *pingsrv_setup(void)
{
        int rc;

        server->ni = PTL_INVALID_HANDLE;

       /* Aquire and initialize the proper nal for portals. */
        rc = PtlNIInit(PTL_IFACE_DEFAULT, 0, NULL, NULL, &server->ni);
        if (!(rc == PTL_OK || rc == PTL_IFACE_DUP)) {
                CDEBUG (D_OTHER, "PtlNIInit: error %d\n", rc);
                return pingsrv_shutdown (4);
        }


        /* Based on the initialization aquire our unique portal ID. */
        if ((rc = PtlGetId (server->ni, &server->my_id))) {
                PDEBUG ("PtlGetId", rc);
                return pingsrv_shutdown (2);
        }

        server->id_local.nid = PTL_NID_ANY;
        server->id_local.pid = PTL_PID_ANY;

        /* Attach a match entries for header packets */
        if ((rc = PtlMEAttach (server->ni, PTL_PING_SERVER,
            server->id_local,0, ~0,
            PTL_RETAIN, PTL_INS_AFTER, &server->me))) {
                PDEBUG ("PtlMEAttach", rc);
                return pingsrv_shutdown (2);
        }


        if ((rc = PtlEQAlloc (server->ni, 1024, &pingsrv_callback,
                                        &server->eq))) {
                PDEBUG ("PtlEQAlloc (callback)", rc);
                return pingsrv_shutdown (2);
        }
        
        PORTAL_ALLOC (server->in_buf, MAXSIZE);
        if(!server->in_buf){
                CDEBUG (D_OTHER,"Allocation error\n");
                return pingsrv_shutdown(2);
        }
        
        /* Setup the incoming buffer */
        server->mdin.start     = server->in_buf;
        server->mdin.length    = MAXSIZE;
        server->mdin.threshold = 1; 
        server->mdin.options   = PTL_MD_EVENT_START_DISABLE | PTL_MD_OP_PUT;
        server->mdin.user_ptr  = NULL;
        server->mdin.eq_handle = server->eq;
        memset (server->in_buf, 0, STDSIZE);
        
        if ((rc = PtlMDAttach (server->me, server->mdin,
                PTL_UNLINK, &server->mdin_h))) {
                    PDEBUG ("PtlMDAttach (bulk)", rc);
                CDEBUG (D_OTHER, "ping server resources allocated\n");
       }
 
        /* Success! */
        return server; 
} /* pingsrv_setup() */

static int pingsrv_start(void) 
{
        /* Setup our server */
        if (!pingsrv_setup()) {
                CDEBUG (D_OTHER, "pingsrv_setup() failed, server stopped\n");
                return -ENOMEM;
        }
        cfs_kernel_thread (pingsrv_thread,NULL,0);
        return 0;
} /* pingsrv_start() */

static int __init pingsrv_init(void)
{
        ping_head_magic = __cpu_to_le32(PING_HEADER_MAGIC);
        ping_bulk_magic = __cpu_to_le32(PING_BULK_MAGIC);
        PORTAL_ALLOC (server, sizeof(struct pingsrv_data));  
        atomic_set(&pkt, 0);
        return pingsrv_start ();
} /* pingsrv_init() */

static void /*__exit*/ pingsrv_cleanup(void)
{
        cfs_remove_proc_entry ("net/pingsrv", NULL);
        
        running = 0;
        wake_up_process (server->tsk);
        while (running != 1) {
                set_current_state (TASK_UNINTERRUPTIBLE);
                schedule_timeout (cfs_time_seconds(1));
        }
        
} /* pingsrv_cleanup() */


MODULE_AUTHOR("Brian Behlendorf (LLNL)");
MODULE_DESCRIPTION("A kernel space ping server for portals testing");
MODULE_LICENSE("GPL");

cfs_module(ping_srv, "1.0.0", pingsrv_init, pingsrv_cleanup);
