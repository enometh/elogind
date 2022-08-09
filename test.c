//
//   Time-stamp: <>
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

void test_wtfreleaselog();

int
main()
{
	use_journal = 1;
	emit_log_msg(7, "source", "zone", 4, "message", NULL);

	test_wtfreleaselog();
	return 0;
}


#define _XSTRINGIFY(line) #line
#define _STRINGIFY(line) _XSTRINGIFY(line)
#define SD_JOURNAL_SEND(channel, priority, file, line, function, ...) do { \
        sd_journal_send_with_location("CODE_FILE=" file, "CODE_LINE=" line, function, "WEBKIT_SUBSYSTEM=%s", "LOG_CHANNEL(channel).subsystem", "WEBKIT_CHANNEL=%s", "LOG_CHANNEL(channel).name", "PRIORITY=%i", priority, "MESSAGE=" __VA_ARGS__, NULL); \
} while (0)
#define RELEASE_LOG(channel, ...) SD_JOURNAL_SEND(channel, LOG_NOTICE, __FILE__, _STRINGIFY(__LINE__), __func__, __VA_ARGS__)
#define WEBPROCESSPOOL_RELEASE_LOG_STATIC(channel, fmt, ...) RELEASE_LOG(channel, "WebProcessPool::" fmt, ##__VA_ARGS__)
void test_wtfreleaselog() {
            WEBPROCESSPOOL_RELEASE_LOG_STATIC("ServiceWorker", "establishRemoteWorkerContextConnectionToNetworkProcess reusing an existing web process (process=%p, workerType=%{private}s, PID=%d)", NULL, 1 ? "service" : "shared", 666);
}