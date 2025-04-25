/*
 * string_parser.c
 *
 *  Created on: Nov 25, 2020
 *      Author: gguan, Monil
 *
 */

#include <stdlib.h>
#include <string.h>
#include "string_parser.h"



int count_token (char* buf, const char* delim)
{
	//TODO：
	/*
	*	#1.	Check for NULL string
	*	#2.	iterate through string counting tokens
	*		Cases to watchout for
	*			a.	string start with delimeter
	*			b. 	string end with delimeter
	*			c.	account NULL for the last token
	*	#3. return the number of token (note not number of delimeter)
	*/

	if (buf == NULL) {
		return 0;
	}

	char *token; char *ptr;

	int count = 0;

	char *copy_buf = (char *)malloc(sizeof(char) * strlen(buf) + 1);

	strcpy(copy_buf, buf);

	copy_buf[strlen(buf)] = '\0';

	token = strtok_r(copy_buf, delim, &ptr);

	while (token != NULL) {
		count++;
		token = strtok_r(NULL, delim, &ptr);

	}

	free(copy_buf);

	return count;
}

command_line str_filler (char* buf, const char* delim)
{
	//TODO：
	/*
	*	#1.	create command_line variable to be filled and returned
	*	#2.	count the number of tokens with count_token function, set num_token. 
    *           one can use strtok_r to remove the \n at the end of the line.
	*	#3. malloc memory for token array inside command_line variable
	*			based on the number of tokens.
	*	#4.	use function strtok_r to find out the tokens 
    *   #5. malloc each index of the array with the length of tokens,
	*			fill command_list array with tokens, and fill last spot with NULL.
	*	#6. return the variable.
	*/
	command_line cl;

	// remove newline if applicable
	// strtok_r(buf, "\n", NULL);

	int j = 0;
	while (buf[j] != '\0')
	{
		if (buf[j] == '\n')
			buf[j] = '\0';
		j++;
	}
	
	// count tokens and assign to cl var
	int num_tok = count_token(buf, delim);
	cl.num_token = num_tok;
	
	// malloc for cmd list
	cl.command_list = (char**)malloc(sizeof(char*)*(num_tok+1));

	// fill in cmd list array
	char* saveptr;
	char* token = strtok_r(buf, delim, &saveptr);
	int i = 0;

	while (token != NULL && i < num_tok)
	{
		cl.command_list[i] = (char*)malloc(strlen(token) + 1);
		strcpy(cl.command_list[i], token);
		i++;
		token = strtok_r(NULL, delim, &saveptr);
	}

	cl.command_list[num_tok] = NULL;

	return cl;
}




void free_command_line(command_line* command)
{
	//TODO：
	/*
	*	#1.	free the array base num_token
	*/
	for (int i = 0; i < command->num_token; i++)
	{
		free(command->command_list[i]);
	}
	free(command->command_list);
}
