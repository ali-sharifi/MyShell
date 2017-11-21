/*
 * historyCommand.c
 *
 *  Created on: 
 *      Author: Ali Sharifi
 */

#include <sys/wait.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "helper.h"

void executePrintHistoryCmd();
void addCmdToHistory(char *command);
void executeSeqHistoryCommand(int commandNo);
void execHistCommandAtIndex(int commandNoIndex);


void executePrintHistoryCmd()
{
	char str[50];

	for (int i = 0; i < history.currentSize; i++)
	{
		sprintf(str, "%d",
				history.totalCommandsExecuted - history.currentSize + i + 1);
		strcat(str, "\t");
		write(STDOUT_FILENO, str, strlen(str));
		write(STDOUT_FILENO, history.historyArray[i],
				strlen(history.historyArray[i]));
		write(STDOUT_FILENO, "\n", strlen("\n"));
	}
}


void executeSeqHistoryCommand(int cmdNo)
{
	if ( cmdNo > history.totalCommandsExecuted || cmdNo < 1 )
	{
		write(STDOUT_FILENO, "Invalid command number\n",
				strlen("Invalid command number\n"));
		return;
	}

	if (history.currentSize < HISTORY_DEPTH)
	{
		execHistCommandAtIndex(cmdNo - 1);
	}
	else
	{
		if (history.totalCommandsExecuted - cmdNo >= HISTORY_DEPTH)
		{
			write(STDOUT_FILENO, "Invalid command number\n",
					strlen("Invalid command number\n"));
			return;
		}
		else
		{
			execHistCommandAtIndex((HISTORY_DEPTH - (history.totalCommandsExecuted - cmdNo + 1)));
		}
	}
}

void addCmdToHistory(char *command)
{
	if (history.currentSize == HISTORY_DEPTH)
	{
		for (int index = 1; index < HISTORY_DEPTH; index++)
		{
			strcpy(history.historyArray[index - 1], history.historyArray[index]);
		}
		strcpy(history.historyArray[HISTORY_DEPTH - 1], command);
	}
	else
	{
		strcpy(history.historyArray[history.currentSize], command);
		history.currentSize++;
	}
	history.totalCommandsExecuted++;
}

void execHistCommandAtIndex(int commandNoIndex)
{
	char commandBuff[COMMAND_LENGTH];
	strcpy(commandBuff, history.historyArray[commandNoIndex]);
	char *tokens[NUM_TOKENS];
	_Bool background = false;

	write(STDOUT_FILENO, commandBuff, strlen(commandBuff));
	write(STDOUT_FILENO, "\n", strlen("\n"));

	tokenizeAndProcessCommand(commandBuff, tokens, &background);
	executeCommand(background, tokens);
}

void handle_SIGINT()
{
	write(STDOUT_FILENO, "\n", strlen("\n"));
	executePrintHistoryCmd();
}

int read_command(char *buff, char *tokens[], _Bool *background)
{
	*background = false;

	int len = read(STDIN_FILENO, buff, COMMAND_LENGTH - 1);

	if ((len < 0) && (errno != EINTR))
	{
		perror("Unable to read command. Terminating.\n ");
		exit(-1);
	}
	if ((len < 0) && (errno == EINTR))
	{
		return 0;
	}

	buff[len] = '\0';
	if (buff[strlen(buff) - 1] == '\n')
	{
		buff[strlen(buff) - 1] = '\0';
	}
	return (tokenizeAndProcessCommand(buff, tokens,background));
}
void executeCommand(_Bool background, char *tokens[])
{
	if (isBuiltInCommand(tokens))
	{
		executeBuiltInCommand(tokens);
	}
	else
	{
		pid_t pID = fork();
		if (pID == 0)
		{
			if (execvp(tokens[0], tokens) == -1)
			{
				write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
				write(STDOUT_FILENO, "\n", strlen("\n"));
				exit(0);
			}
		}
		else if (pID < 0)
		{
			perror("Failed to fork");
			return;
		}
		if (!background)
		{
			if (waitpid(pID, NULL, 0) == -1)
				perror("Notice: Error this proc is waiting for child to exit");
		}
		else
		{
			usleep(10000);
		}

		cleanupZombiProc();
	}
}

_Bool isBuiltInCommand(char *tokens[])
{
	if (strcmp(tokens[0], "pwd") == 0 || strcmp(tokens[0], "cd") == 0
			|| strcmp(tokens[0], "exit") == 0
			|| strcmp(tokens[0], "history") == 0 || strcmp(tokens[0], "\0") == 0
			|| tokens[0][0] == '!')
		return 1;
	return 0;
}

void executeBuiltInCommand(char *tokens[])
{
	if (strcmp(tokens[0], "pwd") == 0)
	{
		executePWDCommand();
		write(STDOUT_FILENO, "\n", strlen("\n"));
	}
	else if (strcmp(tokens[0], "cd") == 0)
	{
		if (chdir(tokens[1]) == -1)
			write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
	}
	else if (strcmp(tokens[0], "history") == 0)
	{
		executePrintHistoryCmd();
	}
	else if (strcmp(tokens[0], "!!") == 0)
	{
		executeSeqHistoryCommand(history.totalCommandsExecuted);
	}
	else if (tokens[0][0] == '!')
	{
		float commandNo = strtof(&tokens[0][1], NULL);
		if (commandNo == 0 || commandNo != (int) commandNo)
		{
			write(STDOUT_FILENO, "Invalid history command number\n",
					strlen("Invalid history command number\n"));
		}
		else
		{
			executeSeqHistoryCommand(commandNo);
		}
	}
	else if (strcmp(tokens[0], "\0") == 0)
	{
		return;
	}
	else
	{
		exit(0);
	}
}

void executePWDCommand()
{
	char myBuffer[150];
	if (!getcwd(myBuffer, 150))
		write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
	else
	{
		write(STDOUT_FILENO, myBuffer, strlen(myBuffer));
	}
}

int tokenizeAndProcessCommand(char* buff, char* tokens[], _Bool* in_background)
{
	if (buff[0] != '!' && buff[0] != '\0')
	{
		addCmdToHistory(buff);
	}

	int token_count = tokenize_command(buff, tokens);
	if (token_count == 0)
	{
		return 0;
	}

	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0)
	{
		*in_background = true;
		tokens[token_count - 1] = 0;
	}
	return 1;
}

int tokenize_command(char *buff, char *tokens[])
{
	int my_Token_Count = 0;
	char *tokenStart = buff;

	while (*buff != '\0')
	{
		if (*buff == ' ')
		{
			if (my_Token_Count == NUM_TOKENS - 1)
				return 0;
			tokens[my_Token_Count++] = tokenStart;
			tokenStart = buff + 1;
			*buff = '\0';
		}
		buff++;
	}

	tokens[my_Token_Count++] = tokenStart;
	tokens[my_Token_Count + 1] = '\0';

	return my_Token_Count;
}

void cleanupZombiProc()
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
}

void cleanArray(char* tokens[])
{
	for (int i = 0; i < NUM_TOKENS; i++)
	{
		if (tokens[i] == '\0')
			break;
		tokens[i] = '\0';
	}
}


