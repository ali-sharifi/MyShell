#include <sys/wait.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "helper.h"

struct historyStruct history;

int main(int argc, char* argv[])
{
	struct sigaction handler;
	handler.sa_handler = handle_SIGINT;
	handler.sa_flags = 0;
	sigemptyset(&handler.sa_mask);
	sigaction(SIGINT, &handler, NULL);

	history.currentSize = 0;
	history.totalCommandsExecuted = 0;
	char input_buffer[COMMAND_LENGTH];
	char *tokens[NUM_TOKENS] =
	{ 0 };
	_Bool in_background = false;

	while (true)
	{
		executePWDCommand();
		write(STDOUT_FILENO, "> ", strlen("> "));

		if (read_command(input_buffer, tokens, &in_background))
			executeCommand(in_background, tokens);

		 cleanArray(tokens);
	}

	return 0;
}
