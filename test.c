// -*- Mode: LISP; Package: :cl-user; BASE: 10; Syntax: ANSI-Common-Lisp; -*-
//
//   Time-stamp: <2020-07-09 15:34:39 IST>
//   Touched: Thu Jul 09 14:18:56 2020 +0530 <enometh@net.meer>
//   Bugs-To: enometh@net.meer
//   Status: Experimental.  Do not redistribute
//   Copyright (C) 2020 Madhu.  All Rights Reserved.
//
#define ENABLE_SYSTEMD
#ifdef ENABLE_SYSTEMD
#include <systemd/sd-journal.h>
#endif
#include <syslog.h>

int use_journal = 0;

static void
emit_log_msg(int level, char *src, const char *zone,
	     size_t zone_len, const char *msg, const char *param)
{
#ifdef ENABLE_SYSTEMD
	if (use_journal) {
		char *zone_fmt = zone ? "ZONE=%.*s." : NULL;
		sd_journal_send("PRIORITY=%d", level,
				"MESSAGE=%s", msg,
				zone_fmt, zone_len, zone,
				param, NULL);
	} else
#endif
	{
		syslog(level, "%s", msg);
	}
}

int
main()
{
	use_journal = 1;
	emit_log_msg(7, "source", "zone", 4, "message", NULL);

}
