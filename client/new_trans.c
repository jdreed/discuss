/*
 *	$Source: /afs/dev.mit.edu/source/repository/athena/bin/discuss/client/new_trans.c,v $
 *	$Header: /afs/dev.mit.edu/source/repository/athena/bin/discuss/client/new_trans.c,v 1.10 1987-03-22 04:39:17 spook Exp $
 *	$Locker:  $
 *
 *	Copyright (C) 1986 by the Student Information Processing Board
 *
 *	New-transaction routine for DISCUSS.  (Request 'talk'.)
 *
 *      $Log: not supported by cvs2svn $
 * Revision 1.9  86/12/08  00:44:32  wesommer
 * Implemented -editor, -no_editor, -mtg control args for program.
 * 
 * Revision 1.8  86/12/07  17:49:44  wesommer
 * Lint fixes.
 * 
 * Revision 1.7  86/12/07  16:04:59  rfrench
 * Globalized sci_idx
 * 
 * Revision 1.6  86/12/07  00:39:27  rfrench
 * Killed ../include
 * 
 * Revision 1.5  86/11/11  01:53:04  wesommer
 * Added access control sanity check on entry of new transactions.
 * 
 * Revision 1.4  86/10/29  10:28:40  srz
 * Miscellaneous cleanup.
 * 
 * Revision 1.3  86/10/19  10:00:13  spook
 * Changed to use dsc_ routines; eliminate refs to rpc.
 * 
 * Revision 1.2  86/09/10  18:57:32  wesommer
 * Made to work with kerberos; meeting names are now longer.
 * ./
 * 
 * Revision 1.1  86/08/22  00:23:58  spook
 * Initial revision
 * 
 */


#ifndef lint
static char *rcsid_discuss_c = "$Header: /afs/dev.mit.edu/source/repository/athena/bin/discuss/client/new_trans.c,v 1.10 1987-03-22 04:39:17 spook Exp $";
#endif lint

#include <stdio.h>
#include <sys/file.h>
#include <signal.h>
#include <strings.h>
#include <sys/wait.h>
#include "ss.h"
#include "tfile.h"
#include "interface.h"
#include "config.h"
#include "globals.h"
#include "acl.h"

#ifdef	lint
#define	USE(var)	var=var;
#else	lint
#define	USE(var)	;
#endif	lint

extern tfile	unix_tfile();
extern char *gets(), *error_message();

new_trans(argc, argv)
	int argc;
	char **argv;
{
	int fd, txn_no;
	tfile tf;
	char *subject = &buffer[0];
	char *whoami = argv[0];
	char *mtg = NULL;
	int code;
	char *editor = NULL;

	USE(sci_idx);
	
	while (++argv, --argc) {
		if (!strcmp (*argv, "-meeting") || !strcmp (*argv, "-mtg")) {
			if (argc==1) {
				(void) fprintf(stderr, 
					       "No argument to %s.\n", *argv);
				return;
			} else {
				--argc;
				mtg = *(++argv);
			}
		} else if (!strcmp (*argv, "-editor") || !strcmp(*argv, "-ed")) {
			if (argc==1) {
				(void) fprintf(stderr, 
					       "No argument to %s.\n", *argv);
				return;
			} else {
				--argc;
				editor = *(++argv);
			}
		} else if (!strcmp(*argv, "-no_editor")) {
			editor = "";
		} else {
			(void) fprintf(stderr, "Usage:  %s [ -editor cmd ] [ -no_editor ] [ -mtg meeting_name ]\n", whoami);
			return;
		}
	}
	
	if(mtg) {
		(void) sprintf(buffer, "goto %s", mtg);
		ss_execute_line(sci_idx, buffer, &code);
		if (code != 0) {
			ss_perror(sci_idx, code, buffer);
			return;
		}
	}

	if (!dsc_public.attending) {
		(void) fprintf(stderr, "Not currently attending a meeting.\n");
		return;
	}
	
	/*
	 * Sanity check on access control; this could be changed on the fly
	 * (which is why it is only a warning)
	 */
	if(!acl_is_subset("w", dsc_public.m_info.access_modes))
		(void) fprintf(stderr, "Warning: meeting is read-only (enter will fail).\n");

	(void) printf("Subject: ");
	if (gets(subject) == (char *)NULL) {
		(void) fprintf(stderr, "Error reading subject.\n");
		clearerr(stdin);
		return;
	}
	(void) unlink(temp_file);
	if (edit(temp_file, editor) != 0) {
		(void) fprintf(stderr,
			       "Error during edit; transaction not entered\n");
		unlink(temp_file);
		return;
	}
	fd = open(temp_file, O_RDONLY, 0);
	if (fd < 0) {
		(void) fprintf(stderr, "No file; not entered.\n");
		return;
	}
	tf = unix_tfile(fd);
	dsc_add_trn(&dsc_public.nb, tf, subject, 0,
		    &txn_no, &code);
	if (code != 0) {
		(void) fprintf(stderr, "Error adding transaction: %s\n", 
			       error_message(code));
		return;
	}
	(void) printf("Transaction [%04d] entered in the %s meeting.\n",
		      txn_no, dsc_public.mtg_name);
	if (dsc_public.current == 0)
	     dsc_public.current = txn_no;

	/* and now a pragmatic definition of 'seen':  If you are up-to-date
	   in a meeting, then you see transactions you enter. */
	if (dsc_public.highest_seen == txn_no -1) {
	     dsc_public.highest_seen = txn_no;
	}
}
