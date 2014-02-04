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
/* file: variables.c - functions related the the variables           */
/*                             TINTIN ++                             */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/


#include "tintin.h"


DO_COMMAND(do_variable)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], temp[BUFFER_SIZE];
	struct listroot *root;

	root = ses->list[LIST_VARIABLE];

	arg = get_arg_in_braces(arg, temp,  FALSE);
	substitute(ses, temp, left, SUB_VAR|SUB_FUN);

	arg = get_arg_in_braces(arg, temp, TRUE);
	substitute(ses, temp, right, SUB_VAR|SUB_FUN);

	if (!*left)
	{
		show_list(ses, root, LIST_VARIABLE);
	}
	else if (*left && !*right)
	{
		if (show_node_with_wild(ses, left, LIST_VARIABLE) == FALSE)
		{
			if (show_message(ses, LIST_VARIABLE))
			{
				tintin_puts2("#THAT VARIABLE IS NOT DEFINED.", ses);
			}
		}
	}
	else
	{
		updatenode_list(ses, left, right, "0", LIST_VARIABLE);

		if (show_message(ses, LIST_VARIABLE))
		{
			tintin_printf2(ses, "#Ok. $%s is now set to {%s}.", left, right);
		}
	}
	return ses;
}


DO_COMMAND(do_internal_variable)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], temp[BUFFER_SIZE];
	struct listroot *root;

	root = ses->list[LIST_VARIABLE];

	arg = get_arg_in_braces(arg, temp,  FALSE);
	substitute(ses, temp, left, SUB_VAR|SUB_FUN);

	arg = get_arg_in_braces(arg, temp, TRUE);
	substitute(ses, temp, right, SUB_VAR|SUB_FUN);

	if (*left)
	{
		updatenode_list(ses, left, right, "0", LIST_VARIABLE);
	}
	return ses;
}


DO_COMMAND(do_unvariable)
{
	char left[BUFFER_SIZE];
	int flag = FALSE;
	struct listroot *root;
	struct listnode *node;

	root = ses->list[LIST_VARIABLE];

	arg = get_arg_in_braces(arg,left,1);

	while ((node = search_node_with_wild(root, left)) != NULL)
	{
		if (show_message(ses, LIST_VARIABLE))
		{
			tintin_printf2(ses, "#Ok. $%s is no longer a variable.", node->left);
		}
		deletenode_list(ses, node, LIST_VARIABLE);
		flag = TRUE;
	}
	if (!flag && show_message(ses, LIST_VARIABLE))
	{
		tintin_puts2("#THAT VARIABLE IS NOT DEFINED.", ses);
	}
	return ses;
}

/********************************************************/
/* the #getlistlength command * By Sverre Normann       */
/********************************************************/
/* Syntax: #getlistlength {destination variable} {list} */
/*****************************************************************/
/* Note: This will return the number of items in the list.       */
/*       An item is either a word, or grouped words in brackets. */
/*       Ex:  #getl {listlength} {smile {say Hi!} flip bounce}   */
/*            -> listlength = 4                                  */
/*****************************************************************/

DO_COMMAND(do_getlistlength)
{
	char left[BUFFER_SIZE], list[BUFFER_SIZE], result[BUFFER_SIZE];
	int i = 0;

	arg = get_arg_in_braces(arg, left, FALSE);
	arg = get_arg_in_braces(arg, list, TRUE);

	if (*left == 0)
	{
		tintin_puts2("#Error - Syntax: #getlistlength {dest var} {list}", ses);
	}
	else
	{
		for (arg = list ; *arg ; arg = get_arg_in_braces(arg, result, FALSE))
		{
			i++;
		}
		sprintf(result, "{%s} {%d}", left, i);

		do_internal_variable(ses, result);
	}
	return ses;
}


/******************************************************************/
/* the #getitemnr command * By Sverre Normann                     */  
/******************************************************************/
/* Syntax: #getitemnr {destination variable} {item number} {list} */
/******************************************************************/
/* Note: This will return a specified item from a list.           */
/*       An item is either a word, or grouped words in brackets.  */
/*       Ex:  #geti {dothis} {2} {smile {say Hi!} flip bounce}    */
/*            -> dothis = say Hi!                                 */
/******************************************************************/

DO_COMMAND(do_getitemnr)
{
	char destvar[BUFFER_SIZE], itemval[BUFFER_SIZE], list[BUFFER_SIZE], temp[BUFFER_SIZE];
	int i;

	arg = get_arg_in_braces(arg, destvar, FALSE);
	arg = get_arg_in_braces(arg, itemval, FALSE);
	arg = get_arg_in_braces(arg, list,    TRUE);

	if (*destvar == 0 || *itemval == 0)
	{
		tintin_puts2("#SYNTAX: #GETITEMNR: {destination variable} {item number} {list}", ses);
	}
	else
	{
		if (!is_number(itemval))
		{
			tintin_printf2(ses, "#GETITEMNR: ITEM NUMBER {%s} IS NOT VALID.", itemval);
		}
		else
		{
			arg = list;

			for (i = temp[0] = 0 ; i < atoi(itemval) ; i++)
			{
				arg = get_arg_in_braces(arg, temp, FALSE);

				if (*temp == 0)
				{
					break;
				}
			}

			if (*temp == 0)
			{
				tintin_printf2(ses, "#GETITEMNR: ITEM {%d} DOES NOT EXIST.", atoi(itemval));
			}
			else
			{
				sprintf(list, "{%s} {%s}", destvar, temp);

				do_internal_variable(ses, list);
			}
		}
	}
	return ses;
}

DO_COMMAND(do_removestring)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE];

	arg = get_arg_in_braces(arg, left,  FALSE);
	arg = get_arg_in_braces(arg, right, TRUE);

	sprintf(result, "{%s} {%s}", left, right);

	tintin_printf2(ses, "#OLD COMMAND, REPLACING WITH: #REPLACESTRING %s", result);

	do_replacestring(ses, result);

	return ses;
}


DO_COMMAND(do_replacestring)
{
	char var[BUFFER_SIZE], old[BUFFER_SIZE], new[BUFFER_SIZE], result[BUFFER_SIZE], *pti, *ptr, *pto;
	struct listroot *root;
	struct listnode *node;

	root = ses->list[LIST_VARIABLE];

	arg = get_arg_in_braces(arg, var, FALSE);
	arg = get_arg_in_braces(arg, old, FALSE);
	arg = get_arg_in_braces(arg, new, TRUE);

	if (*var == 0 || *old == 0)
	{
		if (show_message(ses, LIST_VARIABLE))
		{
			tintin_puts2("#Syntax: #replacestring <var> <oldtext> <newtext>", ses);
		}
	}
	else	if ((node = searchnode_list(root, var)) == NULL)
	{
		if (show_message(ses, LIST_VARIABLE))
		{
			tintin_puts2("#THAT VARIABLE IS NOT DEFINED.", ses);
		}
	}
	else if ((ptr = strstr(node->right, old)) == NULL)
	{
		if (show_message(ses, LIST_VARIABLE))
		{
			tintin_printf2(ses, "#REPLACESTRING {%s} NOT FOUND IN {%s}", old, node->right);
		}
	}
	else
	{
		pti = node->right;
		pto = var;

		while ((ptr = strstr(pti, old)) != NULL)
		{
			while (pti != ptr)
			{
				*pto++ = *pti++;
			}

			strcpy(pto, new);
			pto += strlen(new);

			pti += strlen(old);
		}

		while (*pti)
		{
			*pto++ = *pti++;
		}

		*pto = 0;

		sprintf(result, "{%s} {%s}", node->left, var);

		if (show_message(ses, LIST_VARIABLE))
		{
			tintin_printf2(ses, "#REPLACESTRING: $%s is now set to {%s}", node->left, var);
		}
		do_internal_variable(ses, result);
	}
	return ses;
}


/******************************************************************************
* support routines for #format - Scandum                                      *
******************************************************************************/

void colorstring(char *str)
{
	char result[BUFFER_SIZE] = { 0 }, *pti;
	int cnt;

	pti = str;

	for (pti = str ; *pti ; pti++)
	{
		for (cnt = 0 ; *color_table[cnt].name ; cnt++)
		{
			if (is_abbrev(color_table[cnt].name, pti))
			{
				strcat(result, color_table[cnt].code);
				pti += strlen(color_table[cnt].name);
				break;
			}
		}
	}
	strcpy(str, result);
}

void headerstring(char *str)
{
	char buf[BUFFER_SIZE];

	if (strlen(str) > gtd->ses->cols - 2)
	{
		str[gtd->ses->cols - 2] = 0;
	}

	memset(buf, '#', gtd->ses->cols);

	memcpy(&buf[(gtd->ses->cols - strlen(str)) / 2], str, strlen(str));

	buf[gtd->ses->cols] = 0;

	strcpy(str, buf);
}

void lowerstring(char *str)
{
	char *pts;

	for (pts = str ; *pts ; pts++)
	{
		*pts = tolower(*pts);
	}
}

void upperstring(char *str)
{
	char *pts;

	for (pts = str ; *pts ; pts++)
	{
		*pts = toupper(*pts);
	}
}

void reversestring(char *str)
{
	char t;
	int a = 0, z = strlen(str) - 1;

	while (z > a)
	{
		t = str[z];
		str[z--] = str[a];
		str[a++] = t;
	}
}

void mathstring(struct session *ses, char *str)
{
	char result[BUFFER_SIZE];

	sprintf(result, "%lld", mathexp(ses, str));

	strcpy(str, result);
}

void thousandgroupingstring(char *str)
{
	char result[BUFFER_SIZE], strold[BUFFER_SIZE];
	int cnt1, cnt2, cnt3;

	sprintf(strold, "%lld", atoll(str));

	cnt1 = strlen(strold);
	cnt2 = strlen(strold) + (strlen(strold) - 1 - (atoll(strold) < 0)) / 3;

	for (cnt3 = 0 ; cnt2 >= 0 ; cnt1--, cnt2--, cnt3++)
	{
		result[cnt2] = strold[cnt1];

		if (cnt3 && cnt3 % 3 == 0)
		{
			result[--cnt2] = ',';
		}
	}

	if (atoll(strold) < 0)
	{
		result[0] = '-';
	}

	strcpy(str, result);
}

/******************************************************************************
* #format command - Scandum                                                   *
*******************************************************************************
*  Syntax: #format {variable} {format} {...}                                  *
* Example: #format {upperstr} {%-3s %u %3s)} {- muhahahaha -}                 *
*  Result: $test will hold: -   MUHAHAHAHA   -                                *
******************************************************************************/

DO_COMMAND(do_format)
{
	char destvar[BUFFER_SIZE], format[BUFFER_SIZE], argument[BUFFER_SIZE], arglist[20][BUFFER_SIZE], *ptf;
	struct tm timeval_tm;
	time_t    timeval_t;
	int i;

	arg = get_arg_in_braces(arg, destvar,  FALSE);
	arg = get_arg_in_braces(arg, format,   FALSE);
	arg = get_arg_in_braces(arg, argument, TRUE);

	if (*destvar == 0 || *format == 0)
	{
		tintin_printf2(ses, "#SYNTAX: #format {variable} {format} {arguments}");

		return ses;
	}

	arg = argument;

	for (i = 0 ; i < 20 ; i++)
	{
		arg = get_arg_in_braces(arg, arglist[i], FALSE);
	}

	i = 0;

	for (ptf = format ; *ptf ; ptf++)
	{
		if (i == 20)
		{
			break;
		}

		if (*ptf == '%')
		{
			ptf++;

			if (*ptf == 0)
			{
				break;
			}
			else if (*ptf == '%')
			{
				ptf++;
			}
			else
			{
				while (!isalpha(*ptf))
				{
					ptf++;
				}

				if (*ptf == 0)
				{
					break;
				}

				switch (*ptf)
				{
					case 'c':
						colorstring(arglist[i]);
						break;

					case 'd':
						timeval_t  = (time_t) atoi(arglist[i]);
						timeval_tm = *localtime(&timeval_t);
						strftime(arglist[i], BUFFER_SIZE, "%d-%m-%Y", &timeval_tm);
						break;

					case 'h':
						headerstring(arglist[i]);
						break;

					case 'l':
						lowerstring(arglist[i]);
						break;

					case 'm':
						mathstring(ses, arglist[i]);
						break;

					case 'n':
						arglist[i][0] = toupper(arglist[i][0]);
						break;

					case 'r':
						reversestring(arglist[i]);
						break;

					case 's':
						break;

					case 't':
						timeval_t  = (time_t) atoi(arglist[i]);
						timeval_tm = *localtime(&timeval_t);
						strftime(arglist[i], BUFFER_SIZE, "%H:%M", &timeval_tm);
						break;

					case 'u':
						upperstring(arglist[i]);
						break;


					case 'C':
						sprintf(arglist[i], "%d", ses->cols);
						break;

					case 'G':
						thousandgroupingstring(arglist[i]);
						break;

					case 'L':
						sprintf(arglist[i], "%d", (int) strlen(arglist[i]));
						break;

					case 'R':
						sprintf(arglist[i], "%d", ses->rows);
						break;

					case 'T':
						sprintf(arglist[i], "%d", (int) time(NULL));
						break;

					case 'U':
						sprintf(arglist[i], "%lld", utime());
						break;

					default:
						if (show_message(ses, LIST_VARIABLE))
						{
							tintin_printf2(ses, "#FORMAT: UNKNOWN ARGUMENT {%%%c}", *ptf);
						}
						break;
				}
				*ptf = 's';
				i++;
			}
		}
	}

	sprintf(argument, format, arglist[0], arglist[1], arglist[2], arglist[3], arglist[4], arglist[5], arglist[6], arglist[7], arglist[8], arglist[9], arglist[10], arglist[11], arglist[12], arglist[13], arglist[14], arglist[15], arglist[16], arglist[17], arglist[18], arglist[19]);

	sprintf(format, "{%s} {%s}", destvar, argument);

	if (show_message(ses, LIST_VARIABLE))
	{
		tintin_printf2(ses, "#FORMAT: $%s is now set to {%s}", destvar, argument);
	}

	do_internal_variable(ses, format);

	return ses;
}