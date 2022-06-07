/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright 2017 OPLUS Mobile Comm Corp., Ltd.
 * All rights reserved.
 *
 * header file supporting disable selinux denied log in MP version
 ************************************************************/
#ifndef _SELINUX_PROC_H_
#define _SELINUX_PROC_H_

int is_avc_audit_enable(void);
int init_denied_proc(void);

#endif /* _SELINUX_PROC_H_ */
