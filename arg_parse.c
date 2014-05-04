/*	$Id: arg_parse.c,v 1.4 2013/11/18 22:39:39 foxk5 Exp $	*/

#include <stdio.h>
#include <unistd.h> 
#include <errno.h> 
#include <stdlib.h> 
#include <sys/types.h> 

int arg_parse(char * line, char *** argvp)
{
    // 1. Count number of arguments
    // 2. malloc
    // 3. map pointers and insert eol
    
    int argc = 0; // number of arguments
    char **argv;  // argument vector (for return)
    char *lptr = line; // A pointer to the string for processing 

    /* State Machine
     * Keep track of what we are doing to step through and find arguments
     * quote and arg are the two values used to hold all the states
     */
    int quote = 0; /* boolean value for the quote mode */
    int arg = 0;
   
    while (*lptr != '\0') {
      
      /* State 1: we are in a quote and an argument */
      if (quote && arg) {
        switch(*lptr) {
          case ' ': /* If it is a space just go ahead */
            lptr++;
            break;
          case '"': /* if its a quote end the quote */
            quote = !quote;
            lptr++;
            break;
          default: /* character */
            lptr++;
        }
      /* State 2: we are in an argument but not a quote */
      } else if (!quote && arg) {
        switch(*lptr) {      
          case ' ': /* when not in a quote space mean end of argument */
            arg = !arg;
            lptr++;
            break;
          case '"': /* when not in quote a quote means the start of a quote */
            quote = !quote;
            lptr++;
            break;
          default: /* character: continue */
            lptr++;
        }
      /* State 3: we are in neither an argument or a quote */
      } else if (!quote && !arg) {
        switch(*lptr) {
          case ' ': /* eat spaces between arguments */
            lptr++;
            break;
          case '"': /* start of an argument and a quote */
            arg = !arg;
            quote = !quote;
            argc++; /* inc argc because this is a beginning of an argument */
            lptr++;
            break;
          default: /* an argument starts so inc arg */
            arg = !arg;
            argc++;
            lptr++;
        }
      }
    }
    //if (quote || arg) {
    //  printf("Error parsing input (Invalid number of quotes)");
    //  return 0;
    //}    
    if (argc == 0) return 0;
    /* printf("We have: %i Arguments\n", argc); */

    /* 
     * Allocate an array of pointers that is the size of the number of    
     * tokens we found
     *
     */
    argv = (char **) malloc((argc + 1) * sizeof(char *));
    /* Keep track of the head as we iterate through */
    char ** head = argv;

    /* RESET THE MACHINE!!! This time with two pointers...*/
    arg = 0;
    quote = 0; 
    lptr = line; 
    char * optr = line; /* out pointer */
    
    /* MACHINE 2.0 */
    while (*lptr != '\0') {
      
      /* State 1: we are in a quote and an argument */
      if (quote && arg) {
        switch(*lptr) {
          case ' ': /* If it is a space copy it */
            *optr = *lptr;
            optr++; 
            lptr++;
            break;
          case '"': /* if its a quote end the quote NO COPY */
            quote = !quote;
            lptr++;
            break;
          default: /* character COPY */
            *optr = *lptr;
            optr++;
            lptr++;
        }
      /* State 2: we are in an argument but not a quote */
      } else if (!quote && arg) {
        switch(*lptr) {
          /* when not in a quote space mean end of argument COPY EOS */
          case ' ':   
            arg = !arg;
            *optr = '\0';
            optr++;
            lptr++;
            break;
          /* when not in quote a quote means the start of a quote SKIP */
          case '"':
            quote = !quote;
            lptr++;
            break;
          default: /* character: continue COPY */
            *optr = *lptr;
            optr++;
            lptr++;
        }
      /* State 3: we are in neither an argument or a quote */
      } else if (!quote && !arg) {
        switch(*lptr) {
          case ' ': /* eat spaces between arguments SKIP */
            lptr++;
            break;
          case '"': /* start of an argument and a quote PTR SKIP*/
            arg = !arg;
            quote = !quote;
            *argv = optr;
            argv++;
            lptr++;
            break;
          default: /* an argument starts so inc arg PTR COPY */
            arg = !arg;
            *optr = *lptr;
            *argv = optr;
            argv++;
            optr++;
            lptr++;
        }
      }
    }
    /* Copy the final EOS */
    *optr = *lptr;

    /* Just to be safe this is explicitly done (not *argv = NULL) */
    head[argc] = NULL;

    *argvp = head;
    return argc;
}
