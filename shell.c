/* A program that creates a shell and executes user-inputted commands
 *
 * Author: Huseyin Altinisik
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "parser.h"

#define MAX_CMD_LENGTH 1000 // Maximum command length
#define HISTORY_LENGTH 10 // How many commands our program remembers

void handleInput();
int checkDigits();
void handler();
void builtInCommands();
void executeCommand();
struct history_t createNewCmd();
void recordCommand();
void printHistory();
void printStruct();
void executeOldCommand();

// Struct that represents a command. Holds the string that is the command,
// and the command's ID number
typedef struct history_t{
	char command[MAX_CMD_LENGTH];
	unsigned int commandID;
} history_t;

history_t historyBuffer[HISTORY_LENGTH]; // Circular buffer to store commands
unsigned int commandCount; // Number of commands entered

int main(){
	signal(SIGCHLD, handler); // Register the signal handler
	char string[MAX_CMD_LENGTH]; // User input
	commandCount = 0;

	while (1){
		printf("seaShell> ");
		fflush(stdout);
		char* get = fgets(string, MAX_CMD_LENGTH, stdin);
		if (get == NULL){ // fget failed
			printf("Failed to read input. Exiting...\n");
			fflush(stdout);
			exit(-1);
		}

		handleInput(string); // Deal with the user's inputted command
	}

	return 0;
}

/* Tokenizes the users input and executes the command accordingly (foreground
   or background).

   Parameters:
    	input - The command the user entered
*/
void handleInput(char* input){
	int background; // Background command flag
	char** tokens; // Tokenized input
	tokens = parseCommand(input, &background);
	// Empty input
	if (tokens[0] == '\0'){
		return;
	}
	// Valid input
	else{
		// Record all commands in our history buffer except !num commands
		if (**(tokens) != '!'){
			commandCount++;
			history_t newCmd = createNewCmd(input);
			recordCommand(newCmd);
		}
		// Built-in commands
		if(!strcmp(tokens[0], "exit") || !strcmp(tokens[0], "history")
		|| checkDigits(tokens[0])){
			builtInCommands(tokens);
		}
		// Foreground and background commands
		else{
			executeCommand(tokens, background);
		}
	}
	int len = 0;
	while(tokens[len] != '\0'){ //getting the length of tokens
		len++;
	}
	for(int i=0; i<len+1; i++){  // free tokens
		free(*(tokens+i));
	}
	free(tokens);
}

/* Runs specified commands in the parent process, without forking a child
   process. Specified commands are: exit, history, and !num.

   Parameters:
    	tokens - Tokenized user input
*/
void builtInCommands(char** tokens){
	// Exit command
	if(!strcmp(tokens[0], "exit")){
		exit(0);
	}
	// History command (show history)
	else if(!(strcmp(tokens[0], "history"))){
		printHistory(commandCount);
	}
	// !num command (execute previous command)
	else{
		executeOldCommand(tokens[0]);
	}
}

/* Executes the supplied command by forking a child process. The command is
   executed in the child process, in either the foreground or the background.

   Parameters:
    	tokens - Tokenized user input
		background - Indicates if the command should run in the background
*/
void executeCommand(char** tokens, int background){
	// Background command - does not wait for the child process to end
	pid_t pid = fork(); // Fork to execute command in child process
	if (background){
		// If we are not the child process, execute the command. No waiting
		// since it's a background process
		if (!pid){
			execvp(tokens[0], tokens);
			// If the command failed to execute
			if(stdout){
				printf("%s: command not found\n", tokens[0]);
				fflush(stdout);
				exit(0);
			}
		}
	}
	// Foreground command - requires waiting for the child process to end
	else{
		// If we are the parent process, wait for the child process to end
		if(pid){
			waitpid(pid, NULL, 0);
		}
		// If we are the child process, execute the command
		else{
			execvp(tokens[0], tokens);
			// If the command failed to execute
			if(stdout){
				printf("%s: command not found\n", tokens[0]);
				fflush(stdout);
				exit(0); // Child won't end on its own, so we exit manually
			}
		}
	}
}

/* Makes a new command structure from the user's input.

   Parameters:
    	input - The command the user entered, including all flags

	Returns:
		The command struct for the user's input
*/
history_t createNewCmd(char* input){
	history_t tempCmd;
	strcpy(tempCmd.command, input);
	tempCmd.commandID = commandCount;
	return tempCmd;
}

/* Places the command in the circular history buffer so that it can be
   displayed when the user asks for the command history.

   Parameters:
   		cmd - The struct representing a given command
*/
void recordCommand(history_t cmd){
	// Move all other stored commands down one place in the buffer
	for (int i = HISTORY_LENGTH - 2; i > -1; i--){
		*(historyBuffer + i + 1) = *(historyBuffer + i);
	}
	*(historyBuffer) = cmd; // Add most recent command to front of buffer
}

/* Prints the 10 most recent commands and their command IDs to the user.
*/
void printHistory(){
	// If the buffer is not full, don't print the empty spaces in the buffer
	if (commandCount < HISTORY_LENGTH){
		for (int i = commandCount - 1; i > -1; i--){
			printStruct(i);
		}
	}
	// Print everything when the buffer is full
	else{
		for (int i = HISTORY_LENGTH - 1; i > -1 ; i--){
			printStruct(i);
		}
	}
}

/* Print the given command struct, displaying the command ID then the command.

   Parameters:
    	index - Position in the buffer of the command we want to print
*/
void  printStruct(int index){
	history_t cmdStruct = *(historyBuffer + index);
	unsigned int cmdID = cmdStruct.commandID;
	char* cmd = cmdStruct.command;
	printf("\t\t%u %s", cmdID, cmd);
	fflush(stdout);
}

/* Check if the chars following the '!' in a !num command are valid integers.

   Parameters:
    	token - The !num command

	Returns:
		1 if the input was valid, 0 otherwise
*/
int checkDigits(char* token){
	int hasDigits = 0; // Default false

	// Check that they entered the previous command command
	if (token[0] != '!'){
		return 0;
	}
	for(int i = 1; i < strlen(token); i++){
		if(!isdigit(token[i])){
			return 0;
		}
		else{
			hasDigits = 1;
		}
	}

	return hasDigits;
}

/* Handler that reaps child processes.

   Parameters:
    	sigNum - Signal number for the ended child process
*/
void handler(int sigNum){
	// Don't wait for child processes to hang before going back to the parent
	// process (this way, we immediately show the prompt again after calling
	// a background process)
	while (waitpid(-1, NULL, WNOHANG) > 0) {}
}

/* Finds the appropriate command in the buffer and re-executes it. Used in the
   !num command.

   Parameters:
    	numCmd - ID number for the desired command. It's the !num command
*/
void executeOldCommand(char* numCmd){
	// Get the desired ID from the user input
	char charID[strlen(numCmd)];
	strcpy(charID, numCmd + 1);
	unsigned int desiredID = (unsigned int)atoi(charID);

	// Search through the history buffer for a command with the same ID number
	for (int i = 0; i < HISTORY_LENGTH; i++){
		history_t currCmd = *(historyBuffer + i);
		unsigned int currID = currCmd.commandID;

		// When the command struct is found, execute it
		if (currID == desiredID){
			handleInput(currCmd.command);
			return;
		}
	}
	// Command was not found in the history buffer
	printf("%s: event not found\n", numCmd);
	fflush(stdout);
}
