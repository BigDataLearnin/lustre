/*
 * GPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 for more details (a copy is included
 * in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; If not, see
 * http://www.gnu.org/licenses/gpl-2.0.html
 *
 * GPL HEADER END
 */
/*
 * Copyright (c) 2007, 2010, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2011, 2014, Intel Corporation.
 */
/*
 * This file is part of Lustre, http://www.lustre.org/
 * Lustre is a trademark of Sun Microsystems, Inc.
 */

#ifndef _LUSTRE_DEBUG_H
#define _LUSTRE_DEBUG_H

/** \defgroup debug debug
 *
 * @{
 */

#include <lustre_net.h>
#include <obd.h>

#define LL_CDEBUG_PAGE(mask, page, fmt, arg...)				\
	CDEBUG(mask, "page %p map %p index %lu flags %lx count %u priv %0lx: " \
	       fmt, page, page->mapping, page->index, (long)page->flags, \
	       page_count(page), page_private(page), ## arg)

#define ASSERT_MAX_SIZE_MB 60000ULL
#define ASSERT_PAGE_INDEX(index, OP)                                    \
do { if (index > ASSERT_MAX_SIZE_MB << (20 - PAGE_SHIFT)) {         \
        CERROR("bad page index %lu > %llu\n", index,                    \
	       ASSERT_MAX_SIZE_MB << (20 - PAGE_SHIFT));            \
        libcfs_debug = ~0UL;                                            \
        OP;                                                             \
}} while(0)

#define ASSERT_FILE_OFFSET(offset, OP)                                  \
do { if (offset > ASSERT_MAX_SIZE_MB << 20) {                           \
        CERROR("bad file offset %llu > %llu\n", offset,                 \
               ASSERT_MAX_SIZE_MB << 20);                               \
        libcfs_debug = ~0UL;                                            \
        OP;                                                             \
}} while(0)

/* lib/debug.c */
void dump_lniobuf(struct niobuf_local *lnb);
int dump_req(struct ptlrpc_request *req);
int block_debug_setup(void *addr, int len, __u64 off, __u64 id);
int block_debug_check(char *who, void *addr, int len, __u64 off, __u64 id);

/** @} debug */

#endif
