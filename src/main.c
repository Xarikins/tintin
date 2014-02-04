/******************************************************************************
*   TinTin++                                                                  *
*   Copyright (C) 2004 (See CREDITS file)                                     *
*                                                                             *
*   This program is protected under the GNU GPL (See COPYING)                 *
*                                                                             *
*   This program is free software; you can redistribute it and/or modify      *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   This program is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with this program; if not, write to the Free Software               *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*******************************************************************************/

/*********************************************************************/
/* file: main.c - main module - signal setup/shutdown etc            */
/*                             TINTIN++                              */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/

/* note: a bunch of changes were made here to add readline support -- daw */

#include "tintin.h"

#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>

/*************** globals ******************/

struct session *gts;
struct tintin_data *gtd;

/*
	when the screen size changes, take note of it
*/

RETSIGTYPE winchhandler(int no_care)
{
	struct session *ses;

	/*
		select() will see a "syscall interrupted" error; remember not to worry
	*/

	for (ses = gts ; ses ; ses = ses->next)
	{
		init_screen_size(ses);

		if (HAS_BIT(ses->flags, SES_FLAG_NAWS))
		{
			send_sb_naws(ses);
		}
	}

	/*
		we haveta reinitialize the signals for sysv machines
	*/

	if (signal(SIGWINCH, winchhandler) == BADSIG)
	{
		syserr("signal SIGWINCH");
	}
}

/*
	CHANGED to get rid of double-echoing bug when tintin++ gets suspended
*/

RETSIGTYPE tstphandler(int no_care)
{
	printf("\e[r\e[%d;%dH", gtd->ses->rows, 1);

	fflush(stdout);

	kill(0, SIGSTOP);

	dirty_screen(gtd->ses);

	tintin_puts("#RETURNING BACK TO TINTIN++.", NULL);

	/*
		we haveta reinitialize the signals for sysv machines
	*/

	if (signal(SIGTSTP, tstphandler) == BADSIG)
	{
		syserr("signal SIGTSTP");
	}
}

void trap_handler(int signal)
{
	static char crashed = FALSE;

	if (crashed)
	{
		exit(-1);
	}
	crashed = TRUE;

	clean_screen(gtd->ses);

	dump_stack();

	fflush(NULL);

	exit(-1);
}


/****************************************************************************/
/* main() - show title - setup signals - init lists - readcoms - mainloop() */
/****************************************************************************/


int main(int argc, char **argv)
{
	char filestring[256];
	int arg_num;

	#ifdef SOCKS
		SOCKSinit(argv[0]);
	#endif

	push_call("main(%p,%p)",argc,argv);

	init_tintin();


	if (signal(SIGTERM, trap_handler) == BADSIG)
	{
		syserr("signal SIGTERM");
	}

	if (signal(SIGSEGV, trap_handler) == BADSIG)
	{
		syserr("signal SIGSEGV");
	}


	if (signal(SIGABRT, myquitsig) == BADSIG)
	{
		syserr("signal SIGTERM");
	}

	if (signal(SIGINT, myquitsig) == BADSIG)
	{
		syserr("signal SIGINT");
	}

	if (signal(SIGTSTP, tstphandler) == BADSIG)
	{
		syserr("signal SIGTSTP");
	}


	if (signal(SIGWINCH, winchhandler) == BADSIG)
	{
		syserr("signal SIGWINCH");
	}

/*
	if (signal(SIGALRM, tick_handler) == BADSIG)
	{
		syserr("signal SIGALRM");
	}
*/

	if (getenv("HOME") != NULL)
	{
		if (getenv("TINTIN_HISTORY") == NULL)
		{
			sprintf(filestring, "%s/%s", getenv("HOME"), HISTORY_FILE);
		}
		else
		{
			sprintf(filestring, "%s/%s", getenv("TINTIN_HISTORY"), HISTORY_FILE);
		}
		read_history(filestring);

		add_history("");
	}
	srand48(time(NULL));

	arg_num = 1;

	if (argc > 1 && argv[1])
	{
		if (*argv[1] == '-' && argv[1][1] == 'v')
		{
			arg_num = 2;

			SET_BIT(gts->flags, SES_FLAG_VERBOSE);
		}
	}

	if (argc > arg_num && argv[arg_num])
	{
		gtd->ses = do_read(gts, argv[arg_num]);
	}

	commandloop();

	pop_call();
	return 0;
}


void init_tintin(void)
{
	int cnt;

	gts                 = calloc(1, sizeof(struct session));

	for (cnt = 0 ; cnt < LIST_ALL ; cnt++)
	{
		gts->list[cnt] = init_list();
	}

	gts->name           = strdup("gts");
	gts->host           = strdup("");
	gts->port           = strdup("");
	gts->flags          = SES_FLAG_LOCALECHO;
	gts->map_size       = 6;
	gts->scroll_line    = -1;

	gtd                 = calloc(1, sizeof(struct tintin_data));

	gtd->ses            = gts;
	gtd->str_hash_size  = sizeof(struct str_hash_data);

	gtd->mccp_buf_max   = 64;
	gtd->mccp_buf       = calloc(1, gtd->mccp_buf_max);

	gtd->mud_output_max = 64;
	gtd->mud_output_buf = calloc(1, gtd->mud_output_max);

	initrl();

	init_screen_size(gts);

/*	dirty_screen(gts); */

	/*
		Set application keypad mode and  ESC 0 prefix
	*/

	printf("\e=\e[?1h");

	SET_BIT(gts->flags, SES_FLAG_VERBOSE);

	do_configure(gts, "{SPEEDWALK}          {ON}");
	do_configure(gts, "{VERBATIM}          {OFF}");
	do_configure(gts, "{REPEAT ENTER}      {OFF}");
	do_configure(gts, "{ECHO COMMAND}      {OFF}");
	do_configure(gts, "{VERBOSE}           {OFF}");
	do_configure(gts, "{WORDWRAP}           {ON}");
	do_configure(gts, "{LOG}               {RAW}");
	do_configure(gts, "{BUFFER SIZE}      {5000}");
	do_configure(gts, "{SCROLL LOCK}        {ON}");
	do_configure(gts, "{HISTORY SIZE}     {1000}");
	do_configure(gts, "{CONNECT RETRY}      {60}");
	do_configure(gts, "{PACKET PATCH}        {0}");
	do_configure(gts, "{TINTIN CHAR}         {#}");
	do_configure(gts, "{VERBATIM CHAR}    {\\\\}");
	do_configure(gts, "{REPEAT CHAR}         {!}");

	DEL_BIT(gts->flags, SES_FLAG_VERBOSE);

	insertnode_list(gts, "n", "s", "0", LIST_PATHDIR);
	insertnode_list(gts, "e", "w", "0", LIST_PATHDIR);
	insertnode_list(gts, "s", "n", "0", LIST_PATHDIR);
	insertnode_list(gts, "w", "e", "0", LIST_PATHDIR);
	insertnode_list(gts, "u", "d", "0", LIST_PATHDIR);
	insertnode_list(gts, "d", "u", "0", LIST_PATHDIR);

	do_help(gtd->ses, "CREDITS");
}