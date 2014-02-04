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
******************************************************************************/

/******************************************************************************
*   file: buffer.c - funtions related to the scroll back buffer               *
*           (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t ++ 2.00              *
*                     coded by Igor van den Hoven 2004                        *
******************************************************************************/


#include "tintin.h"


void init_buffer(struct session *ses, int size)
{
	int cnt;

	if (ses->scroll_max == size)
	{
		return;
	}

	if (ses->buffer)
	{
		cnt = ses->scroll_row;

		do
		{
			if (cnt == ses->scroll_max)
			{
				cnt = 0;
			}
			else
			{
				cnt++;
			}

			if (ses->buffer[cnt] == NULL)
			{
				break;
			}

			str_unhash(ses->buffer[cnt]);
		}
		while (cnt != ses->scroll_row);
	}

	ses->buffer = calloc(size, sizeof(char *));

	ses->scroll_max = size;
	ses->scroll_row = size - 1;

	return;
}


void add_line_buffer(struct session *ses, const char *line, int more_output)
{
	char linebuf[BUFFER_SIZE], linelog[BUFFER_SIZE];
	char *pti, *pto;
	int lines;

	if (HAS_BIT(ses->flags, SES_FLAG_SCROLLSTOP))
	{
		return;
	}

	strcat(ses->more_output, line);

	if (more_output == TRUE && strlen(ses->more_output) < BUFFER_SIZE / 2)
	{
		return;
	}

	if (HAS_BIT(gts->flags, SES_FLAG_RESETBUFFER))
	{
		DEL_BIT(gts->flags, SES_FLAG_RESETBUFFER);

		reset_hash_table();
	}

	pti = ses->more_output;
	pto = linebuf;

	while (*pti != 0)
	{
		while (skip_vt102_codes_non_graph(pti))
		{
			interpret_vt102_codes(ses, pti);

			pti += skip_vt102_codes_non_graph(pti);
		}

		if (*pti == 0)
		{
			break;
		}

		if (SCROLL(ses))
		{
			*pto++ = *pti++;
		}
		else
		{
			pti++;
		}
	}
	*pto = ses->more_output[0] = 0;

	lines = word_wrap(ses, linebuf, linelog, FALSE);

	ses->buffer[ses->scroll_row] = str_hash(linebuf, lines);

	if (more_output == -1)
	{
		str_hash_grep(ses->buffer[ses->scroll_row], TRUE);
	}

	if (ses->logfile)
	{
		logit(ses, linelog, ses->logfile);
	}

	if (ses->logline)
	{
		logit(ses, linelog, ses->logline);

		fclose(ses->logline);
		ses->logline = NULL;
	}

	if (--ses->scroll_row < 0)
	{
		ses->scroll_row = ses->scroll_max -1;
	}

	if (ses->buffer[ses->scroll_row])
	{
		ses->buffer[ses->scroll_row] = str_unhash(ses->buffer[ses->scroll_row]);
	}
	return;
}

DO_COMMAND(do_grep)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE];
	int scroll_cnt, grep_cnt, grep_min, grep_max, grep_add;

	grep_cnt = grep_add = scroll_cnt = grep_min = 0;
	grep_max = ses->bot_row - ses->top_row - 2;

	get_arg_in_braces(arg, left,  FALSE);

	if (ses->buffer == NULL)
	{
		tintin_puts2("#GREP, NO SCROLL BUFFER AVAILABLE.", ses);
	}
	else if (*left == 0)
	{
		tintin_puts2("#GREP WHAT?", ses);
	}
	else
	{
		if (is_number(left))
		{
			arg = get_arg_in_braces(arg, left,  FALSE);
			arg = get_arg_in_braces(arg, right, TRUE);

			if (*right == 0)
			{
				tintin_printf2(ses, "#GREP WHAT OF PAGE {%s} ?", left);

				return ses;
			}
			grep_min += grep_max * atoi(left);
			grep_max += grep_max * atoi(left);
		}
		else
		{
			arg = get_arg_in_braces(arg, right, TRUE);
		}

		SET_BIT(ses->flags, SES_FLAG_SCROLLSTOP);

		sprintf(left, "*%s*", right);

		tintin_header(ses, " GREP %s ", right);

		scroll_cnt = ses->scroll_row;

		do
		{
			if (scroll_cnt == ses->scroll_max -1)
			{
				scroll_cnt = 0;
			}
			else
			{
				scroll_cnt++;
			}

			if (ses->buffer[scroll_cnt] == NULL)
			{
				break;
			}

			if (str_hash_grep(ses->buffer[scroll_cnt], FALSE))
			{
				continue;
			}

			if (regexp(left, ses->buffer[scroll_cnt]))
			{
				grep_add = str_hash_lines(ses->buffer[scroll_cnt]);

				if (grep_cnt + grep_add > grep_max)
				{
					break;
				}

				grep_cnt += grep_add;
			}
		}
		while (scroll_cnt != ses->scroll_row);

		if (grep_cnt <= grep_min)
		{
			tintin_puts2("#NO MATCHES FOUND.", ses);
		}
		else do
		{
			if (scroll_cnt == 0)
			{
				scroll_cnt = ses->scroll_max -1;
			}
			else
			{
				scroll_cnt--;
			}

			if (ses->buffer[scroll_cnt] == NULL)
			{
				break;
			}

			if (str_hash_grep(ses->buffer[scroll_cnt], FALSE))
			{
				continue;
			}

			if (regexp(left, ses->buffer[scroll_cnt]))
			{
				grep_add = str_hash_lines(ses->buffer[scroll_cnt]);

				if (grep_cnt - grep_add < grep_min)
				{
					break;
				}

				grep_cnt -= grep_add;

				tintin_puts2(ses->buffer[scroll_cnt], ses);
			}
		}
		while (scroll_cnt != ses->scroll_row);

		tintin_header(ses, "");

		DEL_BIT(ses->flags, SES_FLAG_SCROLLSTOP);
	}
	return ses;
}



int show_buffer(struct session *ses)
{
	char temp[BUFFER_SIZE], wrap[BUFFER_SIZE], *temp_ptr, *wrap_ptr;
	int scroll_size, scroll_cnt, scroll_tmp, scroll_add, skip;

	if (ses != gtd->ses)
	{
		return TRUE;
	}

	scroll_size = get_scroll_size(ses);
	scroll_add  = 0;
	scroll_tmp  = 0;
	scroll_cnt  = ses->scroll_line;

	while (TRUE)
	{
		if (ses->buffer[scroll_cnt] == NULL)
		{
			break;
		}

		scroll_tmp = str_hash_lines(ses->buffer[scroll_cnt]);

		if (scroll_add + scroll_tmp > ses->scroll_base + scroll_size)
		{
			if (scroll_add == ses->scroll_base + scroll_size)
			{
				scroll_tmp = 0;
			}
			else
			{
				scroll_tmp -= ses->scroll_base + scroll_size - scroll_add;
			}
			break;
		}

		scroll_add += scroll_tmp;

		if (scroll_cnt == ses->scroll_max - 1)
		{
			scroll_cnt = 0;
		}
		else
		{
			scroll_cnt++;
		}
	}

	if (scroll_cnt == ses->scroll_line)
	{
		return FALSE;
	}

	if (ses->buffer[scroll_cnt] == NULL)
	{
		erase_screen(ses);
	}

	if (IS_SPLIT(ses))
	{
		save_pos(ses);
		goto_rowcol(ses, ses->bot_row, 1);
		SET_BIT(ses->flags, SES_FLAG_READMUD);
	}

/*	tintin_header(ses, " LINE %d OF %d ", ses->scroll_line, ses->scroll_max); */

	if (ses->buffer[scroll_cnt] && scroll_tmp)
	{
		word_wrap(ses, ses->buffer[scroll_cnt], temp, FALSE);

		temp_ptr = temp;
		wrap_ptr = wrap;

		while (scroll_tmp)
		{
			switch (*temp_ptr)
			{
				case ESCAPE:
					for (skip = skip_vt102_codes(temp_ptr) ; skip > 0 ; skip--)
					{
						*wrap_ptr = *temp_ptr;
						temp_ptr++;
						wrap_ptr++;
					}
					continue;

				case '\r':
					temp_ptr++;
					scroll_tmp--;
					continue;

				case '\0':
					printf("\033[1;31m(buffer error)\n\r");
					scroll_tmp--;
					break;

				default:
					temp_ptr++;
					break;
			}
		}
		*wrap_ptr = 0;

		printf("%s%s\n\r", wrap, temp_ptr);
	}

	while (TRUE)
	{
		if (scroll_cnt == 0)
		{
			scroll_cnt = ses->scroll_max - 1;
		}
		else
		{
			scroll_cnt--;
		}

		if (ses->buffer[scroll_cnt] == NULL)
		{
			break;
		}

		scroll_tmp = word_wrap(ses, ses->buffer[scroll_cnt], temp, FALSE);

		if (scroll_add - scroll_tmp < ses->scroll_base)
		{
			scroll_tmp = scroll_add - ses->scroll_base;
			break;
		}

		scroll_add -= scroll_tmp;

		printf("%s\n\r", temp);
	}

	if (scroll_tmp && ses->buffer[scroll_cnt])
	{
		temp_ptr = temp;

		while (scroll_tmp)
		{
			temp_ptr = strchr(temp_ptr, '\r');
			temp_ptr++;
			scroll_tmp--;
		}
		*temp_ptr = 0;

		printf("%s", temp);
	}

/*	tintin_header(ses, ""); */

	if (IS_SPLIT(ses))
	{
		restore_pos(ses);
		DEL_BIT(ses->flags, SES_FLAG_READMUD);
	}
	fflush(stdout);

	return TRUE;
}


DO_COMMAND(do_buffer)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE];

	arg = get_arg_in_braces(arg, left, FALSE);
	arg = get_arg_in_braces(arg, right, TRUE);

	switch (left[0])
	{
		case 'h':
			buffer_h();
			break;

		case 'u':
			buffer_u();
			break;

		case 'd':
			buffer_d();
			break;

		case 'e':
			buffer_e();
			break;

		case 'w':
			do_writebuffer(ses, right);
			break;

		case 'i':
			do_hash(ses, right);
			break;

		default:
			buffer_e();
			break;
	}

	return ses;
}

void buffer_u(void)
{
	int scroll_size, scroll_cnt, buffer_add, buffer_tmp;

	if (gtd->ses->buffer == NULL)
	{
		return;
	}

	if (gtd->ses->scroll_line == -1)
	{
		gtd->ses->scroll_line = gtd->ses->scroll_row + 1;
	}

	scroll_size = get_scroll_size(gtd->ses);
	scroll_cnt  = gtd->ses->scroll_line;
	buffer_add  = 0 - gtd->ses->scroll_base;

	while (TRUE)
	{
		if (gtd->ses->buffer[scroll_cnt] == NULL)
		{
			break;
		}

		buffer_tmp = str_hash_lines(gtd->ses->buffer[scroll_cnt]);

		if (scroll_size < buffer_add + buffer_tmp)
		{
			gtd->ses->scroll_line = scroll_cnt;
			gtd->ses->scroll_base = scroll_size - buffer_add;

			break;
		}

		buffer_add += buffer_tmp;

		if (scroll_cnt == gtd->ses->scroll_max - 1)
		{
			scroll_cnt = 0;
		}
		else
		{
			scroll_cnt++;
		}
	}

	show_buffer(gtd->ses);
}

void buffer_d(void)
{
	int scroll_size, scroll_cnt, buffer_add, buffer_tmp;

	if (gtd->ses->buffer == NULL)
	{
		return;
	}

	if (gtd->ses->scroll_line == -1)
	{
		buffer_e();
		return;
	}

	scroll_size = get_scroll_size(gtd->ses);
	scroll_cnt  = gtd->ses->scroll_line;
	buffer_add  = gtd->ses->scroll_base;

	if (scroll_cnt == 0)
	{
		scroll_cnt = gtd->ses->scroll_max - 1;
	}
	else
	{
		scroll_cnt--;
	}

	while (TRUE)
	{
		if (gtd->ses->buffer[scroll_cnt] == NULL)
		{
			buffer_e();
			return;
		}

		buffer_tmp = str_hash_lines(gtd->ses->buffer[scroll_cnt]);

		if (scroll_size <= buffer_add + buffer_tmp)
		{
			gtd->ses->scroll_line = scroll_cnt;
			gtd->ses->scroll_base = buffer_tmp - (scroll_size - buffer_add);

			break;
		}

		buffer_add += buffer_tmp;

		if (scroll_cnt == 0)
		{
			scroll_cnt = gtd->ses->scroll_max - 1;
		}
		else
		{
			scroll_cnt--;
		}
	}

	show_buffer(gtd->ses);
}

void buffer_h(void)
{
	if (gtd->ses->buffer == NULL)
	{
		return;
	}

	if (gtd->ses->buffer[0])
	{
		gtd->ses->scroll_line = gtd->ses->scroll_row - 1;
	}
	else
	{
		gtd->ses->scroll_line = gtd->ses->scroll_max - 1;
	}
	gtd->ses->scroll_base = 1;

	buffer_d();
}

void buffer_e(void)
{
	if (gtd->ses->buffer == NULL)
	{
		return;
	}

	if (gtd->ses->scroll_row == gtd->ses->scroll_max - 1)
	{
		gtd->ses->scroll_line = 0;
	}
	else
	{
		gtd->ses->scroll_line = gtd->ses->scroll_row + 1;
	}

	gtd->ses->scroll_base = 0;

	show_buffer(gtd->ses);

	gtd->ses->scroll_line = -1;
	gtd->ses->scroll_base =  0;
}