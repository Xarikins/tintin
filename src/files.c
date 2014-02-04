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
/* file: files.c - funtions for logfile and reading/writing files    */
/*                             TINTIN + +                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*                    New code by Bill Reiss 1993                    */
/*                    New code by Joann Ellsworth                    */
/*********************************************************************/

#include "tintin.h"

/*
	read and execute a command file
*/

DO_COMMAND(do_read)
{
	return readfile(ses, arg, NULL);
}

/*
	Rewritten from scratch since old version wasn't working - Scandum
*/

struct session *readfile(struct session *ses, const char *arg, struct listnode *class)
{
	FILE *fp;
	char bufi[FILE_SIZE], bufo[FILE_SIZE], filename[BUFFER_SIZE], temp[BUFFER_SIZE], *pti, *pto;
	int lvl, cnt, com, lnc, fix;
	int counter[LIST_MAX];

	get_arg_in_braces(arg, filename, TRUE);

	if ((fp = fopen(filename, "r")) == NULL)
	{
		tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
		return ses;
	}

	temp[0] = getc(fp);

	if (!isgraph(temp[0]))
	{
		tintin_puts("#ERROR - INVALID START OF FILE.", ses);
		fclose(fp);

		return ses;
	}

	ungetc(temp[0], fp);

	for (cnt = 0 ; cnt < LIST_MAX ; cnt++)
	{
		counter[cnt] = ses->list[cnt]->count;
	}

	for (bufi[0] = 0 ; fgets(bufo, BUFFER_SIZE, fp) ; strcat(bufi, bufo))
	{
		if (strlen(bufo) + strlen(bufi) >= FILE_SIZE)
		{
			tintin_printf2(ses, "#READ, FILESIZE MUST BE SMALLER THAN %d.", FILE_SIZE);

			fclose(fp);

			return ses;
		}
	}

	pti = bufi;
	pto = bufo;
	lvl = com = lnc = fix = 0;

	while (*pti)
	{
		if (com == 0)
		{
			switch (*pti)
			{
				case DEFAULT_OPEN:
					*pto++ = *pti++;
					lvl++;
					break;

				case DEFAULT_CLOSE:
					*pto++ = *pti++;
					lvl--;
					break;

				case '/':
					if (lvl == 0 && pti[1] == '*')
					{
						pti += 2;
						com += 1;
					}
					else
					{
						*pto++ = *pti++;
					}
					break;

				case '\r':
					pti++;
					break;

				case '\n':
					lnc++;

					if (fix == 0 && pti[1] == gtd->tintin_char && lvl)
					{
						fix = lnc;
					}

					if (lvl)
					{
						pti++;

						while (isspace(*pti))
						{
							pti++;
						}
					}
					else for (cnt = 1 ; ; cnt++)
					{
						if (pti[cnt] == 0)
						{
							*pto++ = *pti++;
							break;
						}

						if (pti[cnt] == DEFAULT_OPEN || pti[cnt] == DEFAULT_CLOSE)
						{
							pti++;
							while (isspace(*pti))
							{
								pti++;
							}
							*pto++ = ' ';
							break;
						}

						if (!isspace(pti[cnt]))
						{
							*pto++ = *pti++;
							break;
						}
					}
					break;

				default:
					*pto++ = *pti++;
					break;
			}
		}
		else
		{
			switch (*pti)
			{
				case '/':
					if (pti[1] == '*')
					{
						pti += 2;
						com += 1;
					}
					else
					{
						pti += 1;
					}
					break;

				case '*':
					if (pti[1] == '/')
					{
						pti += 2;
						com -= 1;
					}
					else
					{
						pti += 1;
					}
					break;

				case '\n':
					lnc++;
					pti++;
					break;

				default:
					pti++;
					break;
			}
		}
	}
	*pto++ = '\n';
	*pto   = '\0';

	if (lvl)
	{
		tintin_printf2(ses, "#READ '%s' MISSING %d '%c' ON OR BEFORE LINE %d.", filename, abs(lvl), lvl < 0 ? DEFAULT_OPEN : DEFAULT_CLOSE, fix == 0 ? lnc + 1 : fix);

		fclose(fp);

		return ses;
	}

	if (com)
	{
		tintin_printf2(ses, "#READ '%s' MISSING %d '%s'", filename, abs(com), com < 0 ? "/*" : "*/");

		fclose(fp);

		return ses;
	}

	sprintf(temp, "{TINTIN CHAR} {%c}", bufo[0]);

	SET_BIT(gts->flags, SES_FLAG_QUIET);

	do_configure(ses, temp);

	lvl = 0;
	pti = bufo;
	pto = bufi;

	while (*pti)
	{
		if (*pti != '\n')
		{
			*pto++ = *pti++;
			continue;
		}
		*pto = 0;

		if (strlen(bufi) >= BUFFER_SIZE)
		{
			DEL_BIT(gts->flags, SES_FLAG_QUIET);

			bufi[20] = 0;

			tintin_printf2(ses, "#READ '%s' BUFFER OVERFLOW AT COMMAND: %s.", filename, bufi);

			fclose(fp);

			return ses;
		}

		if (bufi[0])
		{
			if (class == NULL || HAS_BIT(class->data, NODE_FLAG_CLASS))
			{
				ses = parse_input(bufi, ses);
			}
			else
			{
				parse_class(ses, bufi, class);
			}
		}
		pto = bufi;
		pti++;
	}

	DEL_BIT(gts->flags, SES_FLAG_QUIET);

	if (!HAS_BIT(gts->flags, SES_FLAG_VERBOSE))
	{
		for (cnt = 0 ; cnt < LIST_MAX ; cnt++)
		{
			switch (ses->list[cnt]->count - counter[cnt])
			{
				case 0:
					break;

				case 1:
					tintin_printf2(ses, "#OK %2d %s LOADED.", ses->list[cnt]->count - counter[cnt], list_table[cnt].name);
					break;

				default:
					tintin_printf2(ses, "#OK %2d %s LOADED.", ses->list[cnt]->count - counter[cnt], list_table[cnt].name_multi);
					break;
			}
		}
	}
	fclose(fp);

	return ses;
}

DO_COMMAND(do_write)
{
	FILE *file;
	char temp[BUFFER_SIZE], filename[BUFFER_SIZE];
	struct listnode *node;
	int cnt;

	get_arg_in_braces(arg, filename, TRUE);

	if (*filename == 0 || (file = fopen(filename, "w")) == NULL)
	{
		tintin_printf2(ses, "#ERROR - COULDN'T OPEN '%s' TO WRITE.", filename);
		return ses;
	}

	for (cnt = 0 ; cnt < LIST_MAX ; cnt++)
	{
		for (node = ses->list[cnt]->f_node ; node ; node = node->next)
		{
			prepare_for_write(cnt, node, temp);

			fputs(temp, file);
		}
	}

	fclose(file);

	if (show_message(ses, -1))
	{
		tintin_printf2(ses, "#COMMANDO-FILE WRITTEN.");
	}
	return ses;
}


DO_COMMAND(do_writesession)
{
	FILE *file;
	char buffer[BUFFER_SIZE];
	struct listnode *node;
	int cnt;

	get_arg_in_braces(arg, buffer, TRUE);

	if (*buffer == 0 || (file = fopen(buffer, "w")) == NULL)
	{
		tintin_printf2(ses, "#ERROR - COULDN'T OPEN '%s' TO WRITE.", buffer);
		return ses;
	}

	for (cnt = 0 ; cnt < LIST_MAX ; cnt++)
	{
		for (node = ses->list[cnt]->f_node ; node ; node = node->next)
		{
			if (gts != ses && searchnode_list(gts->list[cnt], node->left))
			{
				continue;
			}

			prepare_for_write(cnt, node, buffer);

			fputs(buffer, file);
		}
	}

	fclose(file);

	tintin_printf2(ses, "#COMMANDO-FILE WRITTEN.");

	return ses;
}


void prepare_for_write(int list, struct listnode *node, char *result)
{
	int llen = strlen(node->left)  > 20 ? 20 : strlen(node->left);
	int rlen = strlen(node->right) > 25 ? 25 : strlen(node->right);

	switch (list_table[list].args)
	{
		case 0:
			result[0] = 0;
			break;
		case 1:
			sprintf(result, "%c%-16s {%s}\n", gtd->tintin_char, list_table[list].name, node->left);
			break;
		case 2:
			sprintf(result, "%c%-16s {%s} %*s {%s}\n", gtd->tintin_char, list_table[list].name, node->left, 20 - llen, "", node->right);
			break;
		case 3:
			sprintf(result, "%c%-16s {%s} %*s {%s} %*s {%s}\n", gtd->tintin_char, list_table[list].name, node->left, 20 - llen, "", node->right, 25 - rlen, "", node->pr);
			break;
	}
}



DO_COMMAND(do_textin)
{
	FILE *fp;
	char buffer[BUFFER_SIZE], *cptr;

	get_arg_in_braces(arg, buffer, 1);

	if ((fp = fopen(buffer, "r")) == NULL)
	{
		tintin_printf2(ses, "#TEXTIN, FILE '%s' NOT FOUND.", buffer);
		return ses;
	}

	while (fgets(buffer, sizeof(buffer), fp))
	{
		for (cptr = buffer ; *cptr && *cptr != '\n' ; cptr++)
		{
			;
		}
		*cptr = '\0';

		if (*buffer)
		{
			write_line_mud(buffer, ses);
		}
		else
		{
			write_line_mud(" ", ses);
		}
	}
	fclose(fp);
	tintin_puts2("#OK, TEXTIN COMPLETED.", ses);

	return ses;
}


DO_COMMAND(do_scan)
{
	FILE *fp;
	char buffer[BUFFER_SIZE];

	get_arg_in_braces(arg, buffer, 1);

	if ((fp = fopen(buffer, "r")) == NULL)
	{
		tintin_printf2(ses, "#SCAN, FILE '%s' NOT FOUND.", buffer);
		return ses;
	}

	SET_BIT(ses->flags, SES_FLAG_SCAN);

	while (BUFFER_SIZE >= gtd->mud_output_max)
	{
		gtd->mud_output_max *= 2;
		gtd->mud_output_buf  = realloc(gtd->mud_output_buf, gtd->mud_output_max);
	}

	while (fgets(gtd->mud_output_buf, BUFFER_SIZE, fp))
	{
		readmud(ses);
	}

	DEL_BIT(ses->flags, SES_FLAG_SCAN);

	tintin_printf2(ses, "OK, SCANNED FILE '%s'.", buffer);

	fclose(fp);

	return ses;
}

DO_COMMAND(do_readmap)
{
	FILE *myfile;
	char buffer[BUFFER_SIZE], *cptr;
	const char *filename = arg;

	get_arg_in_braces(filename, buffer, 1);

	if ((myfile = fopen(buffer, "r")) == NULL)
	{
		tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
		return(ses);
	}

	SET_BIT(gts->flags, SES_FLAG_QUIET);

	while (fgets(buffer, BUFFER_SIZE - 1, myfile))
	{
		for (cptr = buffer ; *cptr && *cptr != '\n' ; cptr++);

		*cptr = 0;

		switch (buffer[0])
		{
			case 'R':
				ses->in_room = create_room(ses, &buffer[2]);
				break;
			case 'E':
				create_exit(ses, &buffer[2]);
				break;
		}
	}

	DEL_BIT(gts->flags, SES_FLAG_QUIET);

	fclose(myfile);

	return(ses);
}


DO_COMMAND(do_writemap)
{
	FILE *file;
	char buffer[BUFFER_SIZE];
	struct exit_data *exit;
	int vnum;

	get_arg_in_braces(arg, buffer, 1);

	if (*buffer == 0 || (file = fopen(buffer, "w")) == NULL)
	{
		tintin_printf2(ses, "#ERROR - COULDN'T OPEN '%s' TO WRITE.", buffer);
		return ses;
	}

	for (vnum = 0 ; vnum < MAX_ROOM ; vnum++)
	{
		if (ses->room_list[vnum])
		{
			sprintf(buffer, "R {%5d} {%d} {%s} {%s}\n",
				ses->room_list[vnum]->vnum,
				ses->room_list[vnum]->flags,
				ses->room_list[vnum]->color,
				ses->room_list[vnum]->name);

			fputs(buffer, file);

			for (exit = ses->room_list[vnum]->f_exit ; exit ; exit = exit->next)
			{
				sprintf(buffer, "E {%5d} {%s} {%s}\n",
					exit->vnum,
					exit->name,
					exit->cmd);

				fputs(buffer, file);
			}
		}
	}

	fclose(file);

	tintin_printf2(ses, "#MAP-FILE WRITTEN.");

	return ses;
}