/*
 *	$Source: /afs/dev.mit.edu/source/repository/athena/bin/discuss/client/lsm.c,v $
 *	$Header: /afs/dev.mit.edu/source/repository/athena/bin/discuss/client/lsm.c,v 1.14 1987-07-08 01:57:43 wesommer Exp $
 *
 *	$Log: not supported by cvs2svn $
 * Revision 1.13  87/07/07  23:06:17  wesommer
 * [spook] changed format.
 * 
 * Revision 1.12  87/06/20  13:35:55  srz
 * Cleaned up Control-C code.
 * 
 * Revision 1.11  87/06/14  21:07:47  srz
 * Added control-C handling.  Removed sorting stuff so that meetings
 * appear in the order of the .meetings file.
 * 
 * Revision 1.10  87/04/19  22:16:56  srz
 * Reverted definition of 'changed' to include new meetings.
 * 
 * Revision 1.9  87/04/08  03:55:00  wesommer
 * Don't ignore code from dsc_expand_mtg_set.
 * 
 * Revision 1.8  87/03/22  04:38:12  spook
 * Changes for new interfaces.
 * 
 * Revision 1.7  87/01/06  23:47:21  rfrench
 * Added "new" field to lsm.
 * 
 * Revision 1.6  87/01/05  11:08:42  srz
 * Massaged header.
 * 
 * Revision 1.5  87/01/04  23:13:41  rfrench
 * First major change to lsm command:
 * Alphabetizes by long name and lists all local names together.
 * 
 */

#ifndef lint
static char *rcsid_lsm_c = "$Header: /afs/dev.mit.edu/source/repository/athena/bin/discuss/client/lsm.c,v 1.14 1987-07-08 01:57:43 wesommer Exp $";
#endif lint


#include <strings.h>
#include <stdio.h>
#include "types.h"
#include "interface.h"
#include "globals.h"
#include "dsc_et.h"
#include "rpc_et.h"
#include <errno.h>

extern char *malloc(), *calloc(), *ctime(), *error_message();

int print_header, long_output;
static char last_host[140], last_path[140];
static char *auser_id;
static int fast;

int do_line(nbp, code, updated)
	register name_blk *nbp;
	int code, updated;
{
	if (print_header) {
	        char *fmt = "%-7s %-22s   %-22s\n";
		last_host[0] = '\0';
		last_path[0] = '\0';
		printf(fmt, " Flags", "Meeting ID", "Short name");
		printf(fmt, " -----", "----------", "----------");
		print_header = 0;
	}
	if (code) {
		printf("        %-22s   %s", nbp->aliases[0],
		       nbp->aliases[1] ? nbp->aliases[1] : "");
		printf(" (%s", 
		       error_message(code));
		switch (code) {
		case ECONNREFUSED:
			printf(" by");
			goto hostn;
		case ETIMEDOUT:
			printf(" with");
			goto hostn;
		case RPC_HOST_UNKNOWN:
		hostn:
			printf(" %s", nbp->hostname);
			break;
				
		default:
			;
		}
		printf(")\n");
		return 0;
	}
	if (!strcmp(last_host,nbp->hostname) && 
		!strcmp(last_path, nbp->pathname))
		printf("        %-22s   %s\n","",nbp->aliases[0]);
	else {
		printf(" %c      %-22s   %*s",updated?'c':' ',
		       nbp->aliases[0],
		       updated?-22:0,
		       (nbp->aliases[1] ? nbp->aliases[1] : ""));
		printf("\n");
		strcpy(last_host,nbp->hostname);
		strcpy(last_path,nbp->pathname);
	}
}

static
do_mtg(mtg_name)
	char *mtg_name;
{
	name_blk *set;
	register name_blk *nbp;
	int n_matches, i, code;
	bool updated;

	dsc_expand_mtg_set(auser_id, mtg_name, &set, &n_matches, &code);

	if (code) {
		ss_perror(sci_idx, code, "");
		return(0);
	}

	if (!n_matches)
		return (0);

	for (i = 0; i < n_matches; i++) {
	        if (interrupt)
		        break;

		nbp = &set[i];

		if (fast) {
		     updated = 0;
		} else {
		     /* Test to see if we are attending this meeting */
		     if (dsc_public.attending 
		     && !strcmp(dsc_public.host, nbp->hostname) 
		     && !strcmp(dsc_public.path, nbp->pathname)) {
			  updated = (dsc_public.highest_seen 
				     < dsc_public.m_info.last);
		     } else {
			  dsc_updated_mtg(nbp, &updated, &code);	
			  if (interrupt)
			       break;
		     }
		}
		do_line(nbp, code, updated);
	}
	return(0);
}

list_meetings (argc, argv)
	int argc;
	char **argv;
{
	int have_names = 0;
	int i, *used;
	char *auser;

	used = (int *)calloc(argc, sizeof(int));
	long_output = 0;	/* make dependent on arguments later */
	auser = "";
	print_header = 1;
	fast = 0;
	for (i = 1; i < argc; i++) {
		if (!strcmp("-user", argv[i])) {
			if (i == argc - 1) {
				fprintf(stderr,
					"Missing argument for -user\n");
				free((char *)used);
				return;
			}
			if (auser[0] != '\0') {
			     fprintf(stderr,
				     "Only one of -user, -public allowed\n");
			     goto punt;
			}
			used[i] = 1;
			auser = argv[++i];
			used[i] = 1;
		}
		else if (!strcmp(argv[i],"-public")) {
		     if (auser[0] != '\0') {
			  fprintf(stderr,
				  "Only one of -user, -public allowed\n");
			  goto punt;
		     }
		     auser = "discuss";
		     used[i] = 1;
		}
		else if (!strcmp(argv[i],"-f") || !strcmp(argv[i], "-fast"))
		     fast = 1;
		else if (*argv[i] == '-') {
			fprintf(stderr,
				"Unknown control argument %s\n",
				argv[i]);
			free((char *)used);
			return;
		}
		else {
			have_names = 1;
		}
	}

	auser_id = malloc(128);
	if (*auser)
		(void) strcpy(auser_id, auser);
	else {
		(void) strcpy(auser_id, user_id);
	}

	flag_interrupts();
	if (!have_names) {
		do_mtg("*");
	}
	else for (i = 1; i < argc; i++) {
		if (!used[i])
			do_mtg(argv[i]);
	}
	dont_flag_interrupts();

punt:
	free((char *)used);
}
