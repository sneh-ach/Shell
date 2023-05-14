#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#define WHITESPACE " \t\n" // We want to split our command line up into tokens

#define MAX_LENGTH 255 // The maximum command-line size

#define MAX_ARGUMENTS 5 // Mav shell only supports 5 arguments

#define HISTORY_LENGTH 15 // Number of maximum pids/processes to show in

int execute_command(char **argument_list, pid_t process_history[], int *process_count, char **command_history, int *command_count, char *input_string);
int split_arguments(char **argument_list, char *input_string);
void execute_history(char *command, char **command_history, int *current_count, pid_t process_history[], int *process_count, char **argument_list, char *input_string);
void store_command(char **command_history, char *current_command, int *current_count);
void display_history(char **command_history, int current_count);
void store_processes(pid_t process_history[], pid_t current_pid, int *current_count, char **argument_list);

void store_processes(pid_t process_history[], pid_t current_pid, int *current_count, char **argument_list)
{
  int i;
  // Check if the current process ID is already stored in the process history
  for (i = 0; i < *current_count; i++)
  {
    if (process_history[i] == current_pid)
    {
      // If the current process ID is already stored, return without storing it again
      return;
    }
  }

  // Check if the process history is full
  if (*current_count == HISTORY_LENGTH)
  {
    // Shift all the elements of the process history one position to the left
    for (i = 0; i < HISTORY_LENGTH - 1; i++)
    {
      process_history[i] = process_history[i + 1];
    }
    // Replace the first element with the latest process id
    process_history[HISTORY_LENGTH - 1] = current_pid;
  }
  else
  {
    // Store the current process id in the next position of the process history
    process_history[*current_count] = current_pid;
    // Increment the current count of processes stored in the history
    (*current_count)++;
  }
}

void store_command(char **command_history, char *current_command, int *current_count)
{
  // Check if the command history is full
  if (*current_count == HISTORY_LENGTH)
  {
    // Shift all the elements of the command history one position to the left
    int i;
    for (i = 0; i < HISTORY_LENGTH - 1; i++)
    {
      command_history[i] = command_history[i + 1];
    }
    // Replace the first element with the latest command
    strcpy(command_history[HISTORY_LENGTH - 1], current_command);
  }
  else if (*current_count < HISTORY_LENGTH)
  {
    // Store the current command in the next position of the command history
    strcpy(command_history[*current_count], current_command);
    // Increment the current count of commands stored in the history
    (*current_count)++;
  }
  else
  {
    // In case the current count exceeds the maximum history length, set it to the maximum length
    *current_count = HISTORY_LENGTH;
  }
}

void display_history(char **command_history, int current_count)
{
  int i;
  int index = 0;
  for (i = 0; i < current_count; i++)
  {
    // Ignore leading and trailing white space
    char *trimmed_command = command_history[i];
    while (*trimmed_command && isspace(*trimmed_command))
    {
      trimmed_command++;
    }
    if (*trimmed_command == '\0')
    {
      // Blank line, skip it
      continue;
    }
    char *end = trimmed_command + strlen(trimmed_command) - 1;
    while (end > trimmed_command && isspace(*end))
    {
      end--;
    }
    end[1] = '\0';
    // Display the index (starting from 1) and the command stored at that index
    printf("%d: %s\n", index, trimmed_command);
    index++;
  }
}

int split_arguments(char **argument_list, char *input_string)
{
  char *argument;
  int argument_count = 0;

  // Split the input string into individual arguments based on whitespace characters
  while (((argument = strsep(&input_string, WHITESPACE)) != NULL) &&
         (argument_count < MAX_ARGUMENTS))
  {

    // Store a copy of each argument in the argument list
    argument_list[argument_count] = strndup(argument, MAX_LENGTH);
    // If the argument is an empty string, set the corresponding position in the argument list to NULL
    if (strlen(argument_list[argument_count]) == 0)
    {
      argument_list[argument_count] = NULL;
    }
    argument_count++;
  }
  // Set the last position in the argument list to NULL
  argument_list[MAX_ARGUMENTS] = NULL;
  // Return the number of arguments found
  return argument_count;
}

void execute_history(char *command, char **command_history, int *current_count, pid_t process_history[], int *process_count,
                     char **argument_list, char *input_string)
{
  int index = atoi(command + 1);

  // Check if the specified index is within the range of stored commands
  if (index >= 0 && index < *current_count)
  {
    // Set the input string to the command stored at the specified index
    input_string = command_history[index];
    // Split the input string into arguments
    split_arguments(argument_list, input_string);
    // Execute the command
    execute_command(argument_list, process_history, process_count, command_history, current_count, input_string);
  }
  else
  {
    // If the specified index is out of range, display an error message
    printf("Command not in history.");
  }
}

int execute_command(char **argument_list, pid_t process_history[], int *process_count, char **command_history, int *command_count, char *input_string)
{
  // Create a variable to store the process ID of the child process
  pid_t pid;

  // Store the input command in the command history
  store_command(command_history, input_string, command_count);

  // Check if the first argument is not NULL
  if (argument_list[0] != NULL)
  {

    // Check if the command is "quit" or "exit"
    if (strcmp(argument_list[0], "quit") == 0 || strcmp(argument_list[0], "exit") == 0)
    {
      // Return 0 to indicate that the shell should exit
      return 0;
    }
    // Check if the command is "cd"
    else if (strcmp(argument_list[0], "cd") == 0)
    {
      // Change the current working directory
      if (chdir(argument_list[1]) == -1)
      {
        printf("%s: No such file or directory.\n", argument_list[1]);
      }
      // Assign the process ID of -1 for all cd commands
      process_history[*process_count] = -1;
      (*process_count)++;
    }
    // Check if the command is "history"
    else if (strcmp(argument_list[0], "history") == 0)
    {
      process_history[*process_count] = -1;
      (*process_count)++;
      if (argument_list[1] != NULL && strcmp(argument_list[1], "-p") == 0)
      {
        process_history[*process_count] = -1;
        (*process_count)++;
        // Display the list of previously executed commands along with their corresponding process IDs
        int i;
        for (i = 0; i < *command_count; i++)
        {
          // Ignore leading and trailing white space
          char *trimmed_command = command_history[i];
          while (*trimmed_command && isspace(*trimmed_command))
          {
            trimmed_command++;
          }
          if (*trimmed_command == '\0')
          {
            // Blank line, skip it
            continue;
          }
          char *end = trimmed_command + strlen(trimmed_command) - 1;
          while (end > trimmed_command && isspace(*end))
          {
            end--;
          }
          end[1] = '\0';
          // Display the index (starting from 0), the process ID, and the command stored at that index
          printf("%d: (%d) %s\n", i, process_history[i], trimmed_command);
        }
      }
      else
      {
        // Display the list of previously executed commands
        display_history(command_history, *command_count);
      }
    }
    // Check if the command starts with "!"
    else if (argument_list[0][0] == '!')
    {
      // Execute a command from the history
      execute_history(argument_list[0], command_history, command_count, process_history, process_count, argument_list, input_string);
    }
    // For all other commands
    else
    {
      // Create a child process
      pid = fork();

      // Check if the current process is the child process
      if (pid == 0)
      {
        // Execute the command
        int ret = execvp(argument_list[0], &argument_list[0]);
        // If execvp returns -1, the command was not found
        if (ret == -1)
        {
          printf("%s: Command not found.\n", argument_list[0]);
        }
        // Exit the child process
        return 0;
      }
      else
      {
        // Store the process ID of the child process in the process history
        store_processes(process_history, pid, process_count, argument_list);
        int status;

        // Wait for the child process to finish
        wait(&status);
      }
    }
  }
  // Return 1 to indicate that the shell should continue running
  return 1;
}

int main()
{
  // Allocate memory for the input command string
  char *input_string = (char *)malloc(MAX_LENGTH);

  // Create arrays to store the process IDs and previously executed commands
  pid_t process_history[HISTORY_LENGTH + 1];
  int process_count = 0;

  char **command_history = malloc((HISTORY_LENGTH + 1) * sizeof(char *));
  int i;
  // Allocate memory for each string in the command history array
  for (i = 0; i <= HISTORY_LENGTH; i++)
  {
    command_history[i] = malloc(MAX_LENGTH);
  }
  int command_count = 0;

  // Continuously prompt the user for commands until the shell is exited
  while (1)
  {
    printf("msh> ");

    // Read a line of input from the user
    while (!fgets(input_string, MAX_LENGTH, stdin))
      ;

    // Allocate memory for a working copy of the input string
    char *working_copy = strdup(input_string);

    // Create an array to store the arguments extracted from the input string
    char *argument_list[MAX_ARGUMENTS + 1];

    // Split the input string into arguments
    split_arguments(argument_list, working_copy);

    // Execute the command specified in the input string
    if (execute_command(argument_list, process_history, &process_count, command_history, &command_count, input_string) == 0)
    {
      // If the execute_command function returns 0, the shell should exit
      return 0;
    }
    // Free the working copy of the input string
    free(working_copy);
  }
  // Free the memory used by the command history array
  for (i = 0; i < HISTORY_LENGTH; i++)
  {
    free(command_history[i]);
  }
  free(command_history);
  // Free the memory used by the input string
  free(input_string);
  return 0;
}