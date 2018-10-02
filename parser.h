
/* Interface for the string tokenization library.
 *
 * Author: RR
*/


/* Return an array of strings that contains tokens extracted from the supplied 
   string.

   Parameters:
       cmdLine - the string to be tokenized
       background - a pointer to a value that is set depending on whether the
                    supplied string describes a command to be executed in
		    "background" mode, i.e. whether the last non-white space
		    character in the command string is an &. If it is the case that it is an
        &, then the command will be run in background mode, and
		    *background will be set to 1; otherwise, it will be set to 0.

   Returns:
       A NULL-terminated array of char*s, in which each char* points to a
       string containing an extracted token.
*/
char** parseCommand(const char* cmdLine, int* bg);
