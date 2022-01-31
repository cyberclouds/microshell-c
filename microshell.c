#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>


#define ERROR( x ) fprintf( stderr, x )
#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_COUNT 64

char* home_path;

/*
 *  int cmd_cp( char** args )
 *  copies the contents of one file to another
 */

int cmd_cp( char** args ) {
    if ( args[ 1 ] == NULL || args[ 2 ] == NULL ) {
        ERROR( "cp: expected two arguments\n" );
        return 1;
    }
    char buf[ 1024 ];
    const int mask = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    int from, to;
    if ( ( from = open( args[ 1 ], O_RDONLY ) ) == -1 ) {
        perror( "microshell" );
        exit( EXIT_FAILURE );
    }
    if ( ( to = open( args[ 2 ], O_WRONLY | O_CREAT | O_TRUNC, mask ) ) == -1 ) {
        perror( "microshell" );
        exit( EXIT_FAILURE );
    }
    int num;
    while ( ( num = read( from, &buf, sizeof( buf ) ) ) > 0 ) {
        write( to, &buf, num );
    }
    close( from );
    close( to );
    return 1;
}

/*
 *  int cmd_touch( char** args )
 *  replicates touch command functionality
 */

int cmd_touch( char** args ) {
    if ( args[ 1 ] == NULL ) {
        ERROR( "touch: expected an argument\n" );
        return 1;
    }
    const int mask = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int file = open( args[ 1 ], O_WRONLY | O_CREAT | O_APPEND, mask );
    if ( file == -1 ) {
        perror( "microshell" );
        exit( EXIT_FAILURE );
    }
    /* get current time */
    time_t current_time = time( NULL );
    struct utimbuf ubuf;
    ubuf.actime = current_time;
    ubuf.modtime = current_time;

    /* change access and modification time */
    if ( utime( args[ 1 ], &ubuf ) != 0 ) {
        ERROR( "touch: cannot modify timestamps of the file\n" );
        close( file );
        exit( EXIT_FAILURE );
    }
    close( file );
    return 1;
}

/*
 *  int cmd_exit( char** args )
 *  terminates the program
 */

int cmd_exit( char** args ) {
    return 0;
}

/*
 *  int cmd_cd( char** args )
 *  changes the current directory
 */

int cmd_cd( char** args ) {
    if ( args[ 1 ] == NULL || !strcmp( args[ 1 ], "~" ) ) {
        if ( chdir( home_path ) != 0 ) {
            perror( "microshell" );
        }
    }
    else if ( chdir( args[ 1 ] ) != 0 ) {
        perror( "microshell" );
    }
    return 1;
}

/*
 *  int cmd_help( char** args )
 *  prints out information about the program and built in commands
 */

int cmd_help( char** args ) {
    if ( args[ 1 ] == NULL ) {
        printf( "* \033[1;34mansi c microshell\033[m\n" );
        printf( "* \033[1;34mauthor: \033[0;0mPatryk Sobala\n" );
        printf( "* \033[1;34mto get help regarding built in commands use: \033[0;0m$ help <command>\n");
        printf( "* \033[1;34mbuilt in commands:\033[m\n" );
        printf( "* - cd\n* - touch\n* - cp\n* - help\n* - exit\n");
    }
    else if ( !strcmp( args[ 1 ], "cd" ) ) {
        printf( "* \033[1;34mcd \033[0;0m- changes current directory\n" );
        printf( "* \033[1;34musage\033[0;0m: cd <directory>\n" );
    }
    else if ( !strcmp( args[ 1 ], "cp" ) ) {
        printf( "* \033[1;34mcp \033[0;0m- copies the contents of a file to another\n" );
        printf( "* \033[1;34musage\033[0;0m: cp <source> <destination>\n" );
    }
    else if ( !strcmp( args[ 1 ], "exit" ) ) {
        printf( "* \033[1;34mexit \033[0;0m- exits the shell\n" );
        printf( "* \033[1;34musage\033[0;0m: exit\n" );
    }
    else if ( !strcmp( args[ 1 ], "touch" ) ) {
        printf( "* \033[1;34mtouch \033[0;0m- update the access and modification times of a file\n" );
        printf( "* \033[1;34musage\033[0;0m: touch <filename>\n" );
    }
    else if ( !strcmp( args[ 1 ], "help" ) ) {
        printf( "* \033[1;34mhelp \033[0;0m- prints out information about the program and built in commands\n" );
        printf( "* \033[1;34musage\033[0;0m: help or help <command>\n" );
    }
    else {
        ERROR( "help: unknown command, use help to get information about available arguments\n" );
    }
    return 1;
}

/*
 *  char* read_line( )
 *  returns the current line
 */

char* read_line( ) {
    char* buf = malloc( sizeof( char ) * MAX_INPUT_SIZE );
    if ( ! buf ) {
        ERROR( "allocation error\n" );
        exit( EXIT_FAILURE );
    }
    int c;
    int pos = 0;

    while ( c != '\n' ) {
        c = getchar( );

        if ( c == EOF ) {
            /* handle CTRL+D combo */
            exit( EXIT_SUCCESS );
        }
        else {
            buf[ pos ] = c;
            ++pos;
        }

        if ( pos >= MAX_INPUT_SIZE ) {
            ERROR( "error: line too long, try inputting less than 1024 characters, program may not behave as expected\n" );
            buf[ pos ] = '\0';
            return buf;
        }
    }
    /* terminate the string */
    buf[ pos ] = '\0';
    return buf;
}

/*
 *  char** parse_line( char* line )
 *  returns an array of tokens from the string
 *  used for recognizing arguments from the line
 */

char** parse_line( char* line ) {
    char** tokens = malloc( sizeof( char* ) * MAX_TOKEN_COUNT );
    if ( !tokens ) {
        ERROR( "allocation error\n" );
        exit( EXIT_FAILURE );
    }
    char* t;
    int pos = 0;

    t = strtok( line, " \n" );

    while ( t != NULL ) {
        tokens[ pos ] = t;
        ++pos;

        if ( pos >= MAX_TOKEN_COUNT ) {
            ERROR( "error: too many arguments, try inputting less than 64 arguments, program may not behave as expected\n" );
            break;
        }

        t = strtok( NULL, " \n" );
    }
    tokens[ pos ] = NULL;
    return tokens;
}

/*
 *  char* get_current_path( )
 *  returns the current path
 */

char* get_current_path( ) {
    char* path = malloc( sizeof( char ) * FILENAME_MAX );

    if ( !path ) {
        ERROR( "allocation error\n" );
        exit( EXIT_FAILURE );
    }

    if ( getcwd( path, FILENAME_MAX ) == NULL ) {
        ERROR( "error: could not retrieve current path\n" );
        exit( EXIT_FAILURE );
    }

    return path;
}

/*
 *  int execute( char** args )
 *  attempts to execute built in commands
 *  if command is not built in, attempts to run the command from PATH
 */

int execute( char** args ) {
    if ( !args[ 0 ] ) {
        return 1;
    }
    else if ( !strcmp( args[ 0 ], "exit" ) ) return cmd_exit( args );
    else if ( !strcmp( args[ 0 ], "help" ) ) return cmd_help( args );
    else if ( !strcmp( args[ 0 ], "cd" ) ) return cmd_cd( args );
    else if ( !strcmp( args[ 0 ], "touch" ) ) return cmd_touch( args );
    else if ( !strcmp( args[ 0 ], "cp" ) ) return cmd_cp( args );

    pid_t pid = fork( );
    int status;

    if ( pid < 0 ) {
        perror( "microshell" ); 
        exit( EXIT_FAILURE );
    }
    else if ( pid == 0 ) {
        if ( execvp( args[ 0 ], args ) == -1 )
            perror( "microshell" );
        exit( EXIT_FAILURE );
    } 
    else {
        pid = waitpid( pid, &status, WUNTRACED );
        status = WEXITSTATUS( status );
    }
    return 1;
}

int main( ) {
    char* path;
    char* input;
    char** args;
    int should_continue = 1;
    char* user = getenv( "USER" );
    home_path = getenv( "HOME" );

    if ( home_path == NULL ) {
        ERROR( "error: could not retrieve home path\n" );
        exit( EXIT_FAILURE );
    }

    if ( user == NULL ) {
        ERROR( "error: could not retrieve user\n" );
        exit( EXIT_FAILURE );
    }

    printf( "** \033[1;34mansi c microshell\033[m **\n" );

    do {
        path = get_current_path( );

        printf( "\033[1;36m%s:[%s]\033[m$ ", user, path );

        input = read_line( );
        args = parse_line( input );
        should_continue = execute( args );

        free( path );
        free( input );
        free( args );
    } while ( should_continue );

    exit( EXIT_SUCCESS );
}