/*
 *
 *    Copyright (C) 1989 by the Massachusetts Institute of Technology
 *    Developed by the MIT Student Information Processing Board (SIPB).
 *    For copying information, see the file mit-copyright.h in this release.
 *
 */
/*
 *	$Id: anm.c,v 1.12 2006-03-10 07:11:30 ghudson Exp $
 *
 */

#ifndef lint
static char rcsid_anm_c[] =
    "$Id: anm.c,v 1.12 2006-03-10 07:11:30 ghudson Exp $";
#endif /* lint */

#include <stdio.h>
#include <sys/file.h>
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ss/ss.h>
#include <discuss/discuss.h>
#include "config.h"
#include "globals.h"

extern tfile unix_tfile();

announce_mtg (argc, argv)
	int argc;
	char **argv;
{
	char temp_file[100],*src_mtg,*dest_mtg;
	char *editor = NULL;
	int code,fd,public,i,txn_no;
	tfile tf;
	name_blk nbsrc, nbdest;

	if (argc < 3) {
		(void) fprintf(stderr, "Usage:  %s {-private | -public} mtg_to_announce mtg_to_announce_in\n", argv[0]);
		return;
	}

	public = 1;
	src_mtg = dest_mtg = NULL;

	i = 0;
	while (i++,--argc > 0) {
		if (*argv[i] == '-') {
			if (!strcmp(argv[i],"-public")) {
				public = 1;
				continue;
			}
			else if (!strcmp(argv[i],"-private")) {
				public = 0;
				continue;
			}
			else if (!strcmp(argv[i], "-editor") ||
				 !strcmp(argv[i], "-ed")) {
					 if (argc == 1) {
						 (void) fprintf(stderr, 
								"No argument to %s.\n", argv[i]);
						 return;
					 }
					 editor = argv[++i];
					 argc--;
					 continue;
				 }
			else if (!strcmp(argv[i], "-no_editor")) {
				editor = "";
				continue;
			}
			else {
				fprintf(stderr,
					"Unknown control argument %s\n",
					argv[i]);
				return;
			}
		}
		if (src_mtg)
			if (dest_mtg) {
				fprintf(stderr,
					"Extra arguments specified: %s\n",
					argv[i]);
				return;
			}
			else
				dest_mtg = argv[i];
		else
			src_mtg = argv[i];
	}

	if (!src_mtg || !dest_mtg) {
		fprintf (stderr,
			 "Two meeting names must be specified\n");
		return;
	}

	dsc_get_mtg (user_id, src_mtg, &nbsrc, &code);
	if (code != 0) {
		(void) fprintf (stderr,
				"%s: Meeting not found in search path.\n",
				src_mtg);
		return;
	}

	dsc_get_mtg (user_id, dest_mtg, &nbdest, &code);
	if (code != 0) {
		(void) fprintf (stderr,
				"%s: Meeting not found in search path.\n",
				dest_mtg);
		return;
	}

	printf("Please enter the body of the meeting announcement.\n");

	(void) sprintf(temp_file,"/tmp/mtg%d.%d",getuid(),getpid());
	(void) unlink(temp_file);

	if (edit(temp_file, editor)) {
		(void) fprintf(stderr,
		  "Error during edit; meeting not announced.\n");
		return;
	}

	fd = open(temp_file,O_RDONLY,0);
	if (fd < 0) {
		(void) fprintf(stderr,"Temporary file disappeared!\n");
		return;
	}

	tf = unix_tfile(fd);

	dsc_announce_mtg(&nbsrc, &nbdest, public, tf, &txn_no, &code);

	if (code) {
		(void) fprintf(stderr,"Error adding transation: %s\n",
			error_message(code));
		return;
	}

	(void) printf("Transaction [%04d] entered in the %s meeting.\n",
		      txn_no, nbdest.aliases[0]);

	(void) close(fd);

}
