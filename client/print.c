/*
 *
 *	Print-related requests for DISCUSS.
 *
 *	$Source: /afs/dev.mit.edu/source/repository/athena/bin/discuss/client/print.c,v $
 *	$Header: /afs/dev.mit.edu/source/repository/athena/bin/discuss/client/print.c,v 1.7 1986-10-29 10:28:59 srz Exp $
 *	$Locker:  $
 *
 *	Copyright (C) 1986 by the Student Information Processing Board
 *
 *      $Log: not supported by cvs2svn $
 * Revision 1.6  86/10/19  10:00:17  spook
 * Changed to use dsc_ routines; eliminate refs to rpc.
 * 
 * Revision 1.5  86/10/15  00:50:23  spook
 * switch to use ss_pager_create
 * 
 * Revision 1.4  86/10/15  00:18:26  spook
 * Added trn list parsing to print and write requests.
 * 
 * Revision 1.3  86/09/10  18:57:35  wesommer
 * Made to work with kerberos; meeting names are now longer.
 * ./
 * 
 * Revision 1.2  86/09/10  17:43:16  wesommer
 * Ken, please clean up after yourself.
 * 
 * Revision 1.1  86/08/22  00:24:01  spook
 * Initial revision
 * 
 * 
 */


#ifndef lint
static char *rcsid_discuss_c = "$Header: /afs/dev.mit.edu/source/repository/athena/bin/discuss/client/print.c,v 1.7 1986-10-29 10:28:59 srz Exp $";
#endif lint

#include <stdio.h>
#include <errno.h>
#include <sys/file.h>
#include <signal.h>
#include <strings.h>
#include <sys/wait.h>
#include "../include/ss.h"
#include "../include/tfile.h"
#include "../include/interface.h"
#include "../include/config.h"
#include "../include/dsc_et.h"
#include "globals.h"

#ifdef	lint
#define	USE(var)	var=var;
#else	lint
#define	USE(var)	;
#endif	lint

#define max(a, b) ((a) > (b) ? (a) : (b))

extern tfile	unix_tfile();
static trn_nums	performed;
static char *	request_name;
static trn_info	t_info;
static tfile	tf;

static error_code
display_trans(trn_no)
	int trn_no;
{
	error_code code;
	output_trans(trn_no, tf, &code);
	if (code == 0) {
	     dsc_public.highest_seen = max(dsc_public.highest_seen,trn_no);
	     dsc_public.current = trn_no;
	     performed = TRUE;
	} else if (code == EPIPE) {
	     performed = TRUE;
	     return(code);		/* silently quit */
	}
	else if (code != DELETED_TRN) {
	     fprintf(stderr, "Error printing transaction: %s\n",
		     error_message(code));
	     return(code);
	}

	return(0);
}

prt_trans(sci_idx, argc, argv)
	int sci_idx;
	int argc;
	char **argv;
{
	int txn_no;
	int fd;
	int (*old_sig)();
	error_code code;
	selection_list *trn_list;

	request_name = ss_name(sci_idx);
	if (!dsc_public.attending) {
		(void) fprintf(stderr, "No current meeting.\n");
		return;
	}
	dsc_get_mtg_info(dsc_public.mtg_uid, &dsc_public.m_info, &code);
	if (code != 0) {
		(void) ss_perror(sci_idx, code, "Can't get meeting info");
		return;
	}

	dsc_get_trn_info(dsc_public.mtg_uid, dsc_public.current, &t_info, &code);
	if (code)
		t_info.current = 0;
	else {
	     free(t_info.subject);			/* don't need these */
	     t_info.subject = NULL;
	     free(t_info.author);
	     t_info.author = NULL;
	}

	if (argc == 1) {
	        trn_list = trn_select(&t_info, "current",
				      (selection_list *)NULL, &code);
		if (code) {
			ss_perror(sci_idx, code, "");
			free(trn_list);
			return;
		}
	}
	else if (argc == 2) {
		trn_list = trn_select(&t_info, argv[1],
				      (selection_list *)NULL, &code);
		if (code) {
			ss_perror(sci_idx, code, "");
			sl_free(trn_list);
			return;
		}
	}
	else {
		trn_list = (selection_list *)NULL;
		while (argv++, argc-- > 1) {
			trn_list = trn_select(&t_info, *argv,
					      trn_list, &code);
			if (code) {
				ss_perror(sci_idx, code, *argv);
				sl_free(trn_list);
				return;
			}
		}
	}

	performed = FALSE;
	/*
	 * Ignore SIGPIPE from the pager
	 */
	old_sig = signal(SIGPIPE, SIG_IGN);
	fd = ss_pager_create();
	if (fd < 0) {
		fprintf(stderr, "%s: Can't start pager: %s\n",
			request_name, error_message(ERRNO));
		return;
	}
	tf = unix_tfile(fd);
	(void) sl_map(display_trans, trn_list);
	tclose(tf, &code);
	(void) close(fd);
	(void) tdestroy(tf);
	(void) wait((union wait *)0);
	(void) signal(SIGPIPE, old_sig);
	if (!performed)
		(void) fprintf(stderr, "print: No transactions selected\n");
}

write_trans(sci_idx, argc, argv)
	int sci_idx;
	int argc;
	char **argv;
{
	int txn_no;
	selection_list *trn_list;
	int fd;
	int (*old_sig)();
	int code;

	if (dsc_public.mtg_uid == (char *)NULL) {
		(void) fprintf(stderr, "No current meeting.\n");
		return;
	}
	dsc_get_mtg_info(dsc_public.mtg_uid, &dsc_public.m_info, &code);
	if (code != 0) {
		(void) ss_perror(sci_idx, code, "Can't get meeting info");
		return;
	}
	if (argc != 3) {
		(void) fprintf(stderr,
			       "Usage:  %s transaction_list filename\n",
			       argv[0]);
		return;
	}
	trn_list = trn_select(&t_info, argv[1], (selection_list *)NULL,
			      &code);
	if (code) {
		ss_perror(sci_idx, code, argv[1]);
		sl_free(trn_list);
		return;
	}
	performed = FALSE;
	dsc_public.current = txn_no;

	fd = open(argv[2], O_CREAT|O_APPEND|O_WRONLY, 0666);
	if (fd < 0) {
		ss_perror(sci_idx, errno, "Can't open output file");
		return;
	}
	tf = unix_tfile(fd);
	(void) sl_map(display_trans, trn_list);
	tclose(tf, &code);
	(void) close(fd);
	(void) tdestroy(tf);
	if (!performed)
		(void) fprintf(stderr, "print: No transactions selected\n");
	return;
}