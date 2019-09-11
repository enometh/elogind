// seat1.c
//
//   Time-stamp: <2019-09-11 22:11:53 IST>
//   Touched: Tue Aug 13 21:26:17 2019 +0530 <enometh@net.meer>
//   Bugs-To: enometh@net.meer
//   Status: Experimental.  Do not redistribute
//   Copyright (C) 2019 Madhu.  All Rights Reserved.
//
//
// from multiseat_kde/usr/local/bin/seat1
//
// PENDING if run as root then seat=seat0 vtnr gets passed correctly.
// dbus rejected messages (when running as setuid root) - both createsession and releasesession

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <security/pam_modules.h>
#include <security/pam_modutil.h>
#include <security/pam_appl.h>
#include <pwd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include <regex.h>
#include <grp.h>

static char *argv0;
#include <arg.h>

struct passwd *
find_pwent(char * name)
{
	struct passwd * passwd = (struct passwd *) 0;
	for (setpwent(); ((passwd = getpwent()) != 0);) {
		if (strcmp(passwd->pw_name, name) == 0) break;
	}
	endpwent();
	return passwd;
}


void
make_pam(FILE *output)
{
	fprintf(output, "auth	include	system-local-login\n");
	fprintf(output, "account	include	system-local-login\n");
	fprintf(output, "password	include	system-local-login\n");
	fprintf(output, "#session required pam_loginuid.so\n");
	fprintf(output, "#session required pam_keyinit.so force revoke\n");
	fprintf(output, "#session required pam_elogind.so\n");

}


void
usage(void)
{
	fprintf(stdout, "Usage: %s [-u user] [-s service] [-v vtnr] [-S seat0] [-t x11] [-c user]\n",
		argv0);
	fprintf(stdout, "\n\n	-p [-|FILE] print /etc/pam/<service>\n");
}

char *user = 0;
char *service_name = 0;
char *vt_name = 0;
char *seat = "seat0";
char *session_type = "x11";
char *session_class = "user";

int
parse_args(int argc, char **argv, int *_argc, char ***_argv)
{
	fprintf(stderr, "start argc=%d argv=%p\n", argc, argv);
	ARGBEGIN {
	case 'u':
		user=EARGF(usage());
		break;
	case 's':
		service_name=EARGF(usage());
		break;
	case 'v':
		vt_name=EARGF(usage());
		break;
	case 'S':
		seat=EARGF(usage());
		break;
	case 't':
		session_type=EARGF(usage());
		break;
	case 'c':
		session_class=EARGF(usage());
		break;

	case 'p':
	{
		char *pam_d_config = EARGF(usage());
		FILE *out = stdout;
		if (service_name == NULL) {
			fprintf(stderr, "service name not provided\n");
			return -1;
		}
		if (*pam_d_config == '-' && *(pam_d_config + 1) == 0) {
		} else {
			out = fopen(pam_d_config, "w");
			if (out == NULL) {
				fprintf(stderr, "unable to open %s: %s\n",
					pam_d_config, strerror(errno));
				return -1;
			}
		}
		make_pam(out);
		if (out == stdout) {
		} else {
			fclose(out);
		}
		return 0;
	}
	case 'h':
		usage();
		return 0;
	default:
		usage();
		return -1;
	} ARGEND;

	if (_argc) *_argc = argc;
	if (_argv) *_argv = argv;
	fprintf(stderr, "setting argc=%d argv=%p\n", argc, argv);
	fprintf(stderr, "remaining args\n");
	while (argc--)
		fprintf(stderr, "argv=%s\n", *argv++);
	return 0;
}


static int converse(int, const struct pam_message **, struct pam_response **, void *);

/*
       struct pam_message {
           int msg_style;
           const char *msg;
       };

       struct pam_response {
           char *resp;
           int resp_retcode;
       };

*/


static int
converse(int num_msg, const struct pam_message **msg,
	 struct pam_response **resp, void *appdata_ptr)
{
  if (num_msg <= 0 || num_msg > PAM_MAX_NUM_MSG) {
    fprintf(stderr, "invalid number of PAM messages: %d", num_msg);
    return -1;
  }
  fprintf(stderr, "number of PAM messages: %d", num_msg);

  struct pam_response *reply;
  if ((reply = calloc(num_msg, sizeof(struct pam_response))) == NULL) {
    fprintf(stderr, "unable to allocate memory: %s", strerror(errno));
    return -1;
  }
  *resp = reply;
  struct conv_callback *callback = NULL;
  if (appdata_ptr != NULL) {
	  callback = *((struct conv_callback **) appdata_ptr);
  }
  for (int n = 0; n < num_msg; n++) {
	  const struct pam_message *pm = msg[n];
	  fprintf(stderr, "converse[%d]=%s\n", pm->msg_style, pm->msg);
  }
  return 0;
}

/*
 * Callback struct to be passed to the conversation function.
 */
struct conv_callback {
	int bogus;
};

static struct conv_callback *conv_callback;
static struct pam_conv pam_conv = { converse, &conv_callback };
pam_handle_t* pamh;

int
pameg_pam_init() {
	int pam_status =  pam_start((const char *)service_name, (const char *)user,
				    (const struct pam_conv *)&pam_conv,
				    (pam_handle_t **) &pamh);
	if (pam_status != PAM_SUCCESS) {
		fprintf(stderr, "Unable to initialize PAM for %s, %s: %s\n",
			user, service_name,
			pam_strerror(pamh, pam_status));
	}
	return pam_status;
}


int
query_session_leader()
{
	int pid = -1;
	char *line = 0;
	FILE *proc = popen("loginctl session-status", "r");
	if (proc == 0) {
		fprintf(stderr,  "popen(loginctl session-status failed: %s\n",
			strerror(errno));
		goto done;
	}
	static regex_t *leader_reg = NULL;
	if (!leader_reg) {
		leader_reg = malloc(sizeof(regex_t));
		if (leader_reg == NULL) {
			fprintf(stderr, "malloc failed: %s\n",
				strerror(errno));
			goto done;
		}
		int err = regcomp(leader_reg, "^[ \t]+Leader:[ \t]+([0-9]+)", REG_EXTENDED);
		if (err) {
			fprintf(stderr, "regcomp failed: %s\n", strerror(errno));
			goto done;
		}
	}
	ssize_t nread;
	size_t len;
	while ((nread = getline(&line, &len, proc)) != -1) {
		regmatch_t match[2];
		//fprintf(stderr, "processing line: %s\n", line);
		if (regexec(leader_reg, line, 2, match, 0) == 0) {
			int _pid = 0;
			for (int i = match[1].rm_so ; i <  match[1].rm_eo; i++) {
				_pid = _pid * 10 + (line[i] - '0');
			}
			pid = _pid;
			goto done;

		}
	}
	fprintf(stderr, "Unable to parse loginctl session-status output\n");
done:
	if (line) free(line);
	if (proc) pclose (proc);
	return pid;
}


void
print_pam_vars(pam_handle_t* handle)
{

	const char *username = NULL, *pw = NULL, *service=NULL,
		*display = NULL, *tty = NULL, *remote_user = NULL,
		*remote_host = NULL;
	int x;
	if ((x = pam_get_user(handle, &username, NULL)) != PAM_SUCCESS)
		fprintf(stderr, "pam_get_user failed: %s\n", pam_strerror(handle, x));

	//if ((pw =  (const char *) pam_modutil_getpwnam(handle, username)) == NULL) fprintf(stderr, "pam_modutil_getpwnam failed: %s\n", pam_strerror(handle, x));

#define x_pam_get_item(...) if ((x = pam_get_item(__VA_ARGS__)) != PAM_SUCCESS) fprintf(stderr, "pam_get_item failed: %s\n", pam_strerror(handle, x));
	x_pam_get_item(handle, PAM_SERVICE, (const void**) &service);
        x_pam_get_item(handle, PAM_XDISPLAY, (const void**) &display);
        x_pam_get_item(handle, PAM_TTY, (const void**) &tty);
        x_pam_get_item(handle, PAM_RUSER, (const void**) &remote_user);
        x_pam_get_item(handle, PAM_RHOST, (const void**) &remote_host);
#undef x_pam_get_item
	fprintf(stderr,
		"PAM user=%s service=%s display=%s tty=%s ruser=%s rhost=%s\n",
		user,service,display,tty,remote_user,remote_host);
}

void
upgrade()
{
	// pam_elogind + dbus expects the process to be uid0
	if (setuid(0) < 0) {
		fprintf(stderr, "error setting uid 0: %s\n",
			strerror(errno));
	}
	if (setgid(0) < 0) {
		fprintf(stderr, "error setting gid 0: %s\n",
			strerror(errno));
	}
	fprintf(stderr, "uid=%d gid=%d\n", getuid(), getgid());
}

int
main(int argc, char **argv)
{
	char *x;
	if ((x = getenv("FOO"))==NULL) {
		// try to simulate simulate fexec by re-exec
		if (setenv("FOO", "1", 0) < 0) {
			fprintf(stderr, "setenv(): failed: %s\n",
				strerror(errno));
			exit(1);
		}
		upgrade();
		if (execvp(argv[0], argv)) {
			fprintf(stderr, "execvp(%s): failed: %s\n",argv[0],
				strerror(errno));
			exit(1);
		}
	} else if (strcmp("FOO", "0")) /* TODO debug  */
		upgrade();

	int ret, pam_status;

	int _argc = -1; char **_argv = NULL;


	if (parse_args(argc, argv, &_argc, &_argv) < 0) exit(-1);
	fprintf(stdout, "user=%s, service=%s\n", user, service_name);
	fprintf(stdout, "XDG_SESSION_ID=%s\n", getenv("XDG_SESSION_ID"));

	int session_leader_pid;
	if ((session_leader_pid = query_session_leader()) < 0) {
		// exit(-1);
	}
	fprintf(stderr, "initial session_leader_pid=%d\n", session_leader_pid);

	int pid = fork();
	if (pid) {
		fprintf(stderr,"parent: %d (sid %d): child spawned at %d\n", getpid(), getsid(0), pid);
		if (wait(NULL) < 0) { perror("wait failed"); }
		exit(0);
	}

	fprintf(stderr,"child running at %d sid=%d\n", getpid(), getsid(0));

	/* double fork
	pid = fork();
	if (pid) {
		fprintf(stderr,"double parent%d (sid %d): child spawned at %d\n", getpid(), getsid(0), pid);
		exit(0);
	}
	fprintf(stderr,"double child running at %d sid=%d\n", getpid(), getsid(0));
	*/

	if (setsid() < 0) {
		fprintf(stderr, "setsid failed: %s\n", strerror(errno));
		exit(1);
	}
	fprintf(stderr, "sid=%d\n", getsid(0));

	if (setenv("XDG_SEAT", seat, 1) < 0) { perror("error setting XDG_SEAT\n"); }
	if (setenv("XDG_SESSION_TYPE", session_type, 1) < 0) { perror("error setting XDG_SESION_TYPE\n"); }
	if (setenv("XDG_SESSION_CLASS", session_class, 1) < 0) { perror("error setting XDG_SESION_TYPE\n"); }

	/* chvt(10); */
	if (!vt_name) {
		fprintf(stderr, "supply -v vtnr\n");
		exit(1);
	}

	if (setenv("XDG_VTNR", vt_name, 1) < 0) {
		fprintf(stderr, "error setting XDG_VTNR to %s: %s\n",
			vt_name, strerror(errno));
	}

	/* init pam */
	ret = pameg_pam_init();
	if (ret < 0) exit(-1);

	pam_status = pam_open_session(pamh, 0);
	if (pam_status != PAM_SUCCESS) {
		fprintf(stderr, "Unable to open PAM session: %s\n", pam_strerror(pamh, pam_status));
		exit(-1);
	}
	fprintf(stdout, "XDG_SESSION_ID=%s\n", getenv("XDG_SESSION_ID"));

	print_pam_vars(pamh);

	struct passwd *ent = getpwnam(user);
	if (!ent) {
		fprintf(stderr, "could not get pwent for %s: %s\n",
			user, strerror(errno));
		exit(1);
	}


	/* change_user */
	gid_t gid = ent->pw_gid;
	uid_t uid = ent->pw_uid;


	int cpid = fork();
	if (cpid == 0) {
		fprintf(stderr, "changing user to uid=%d gid=%d\n", uid, gid);
		if (initgroups(user, gid) != 0) {
			fprintf(stderr, "initgroups(%s, %d) failed: %s\n", user, gid,
				strerror(errno));
		}
		if (setresgid(gid, gid, gid) != 0) {
			fprintf(stderr, "setresgid(%d, %d, %d) failed: %s\n",
				gid, gid, gid, strerror(errno));
		}
		if (setresuid(uid, uid, uid) != 0) {
			fprintf(stderr, "setresuid(%d, %d, %d) failed: %s\n",
				gid, gid, gid, strerror(errno));
		}

		if ((session_leader_pid = query_session_leader()) < 0) exit(-1);
		fprintf(stderr, "final session_leader_pid=%d\n", session_leader_pid);

		fprintf(stderr, "executing argc=%d argv=%s\n", _argc, _argv[0]);
		execvp( _argv[0], _argv );
	}
	if (cpid  < 0) {
		perror("fork failed");
		exit(1);
	}
	if (wait(NULL) < 0) {
		perror("wait failed");
	}

	/* cleanup pam */
	pam_status = pam_close_session(pamh, 0);
	if (pam_status != PAM_SUCCESS) {
		fprintf(stderr, "Unable to close PAM session: %s\n", pam_strerror(pamh, pam_status));
		exit(-1);
	}



	exit(0);

}


// Local Variables:
// c-basic-offset:8;
// c-file-style:"bsd";
// c-toggle-electric-state:-1;
// compile-command:(format "gcc -ggdb -D_GNU_SOURCE -Wall -std=c99 %S -o %S -lpam"  (file-relative-name buffer-file-name) (file-name-sans-extension (file-relative-name buffer-file-name)))
// end:
