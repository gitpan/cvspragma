/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS 1.4 kit.
 * 
 * Pragma (Martin.Cleaver@BCS.org.uk) 10/Dec/1997
 * ================================== ===========
 *
 * PURPOSE 
 * -------
 *
 * CVS Pragma is an extension command to CVS that allows end users and 
 * repository administrators to add new sub commands to the 'cvs' program
 * without requiring the binary to be recompiled.
 *
 * Typically this could be used to prototype new cvs commands, 
 * but also allows customisation of CVS and the addition of unofficial
 * patches without recompilation.
 *
 * It was designed and tested against CVS-1.9
 *
 * OVERVIEW
 * --------
 *
 * Commands are added under the 'cvs pragma' command. For instance,
 * I can create a new command 'cvs pragma gettags' to show all the
 * tags in use in this module. Having the prefix 'pragma' means 
 * we don't polute the CVS commands namespace.
 *
 * The extra commands are stored in directories pointed to by the 
 * CVSPRAGMADIR environment variable, these would typically be Perl scripts
 * but any executable format would work (see examples).
 *
 * Both server-side and client-side pragma commands are possible and  
 * unless the user specifies, cvs first attempts to run the command 
 * on the client. If it is not found on the client, cvs will try to
 * run it on the server.
 * 
 * Environment variable CVSPRAGMADIR is used on both sides.
 *
 * If all the work can be done without looking at the repository, 
 * implementation should be installed on the client side; this saves
 * on unnecessary RPC. An example of a client side script would be
 * a terse cvs 'status' to show only the filename and whether the
 * file had been modified.
 * 
 * Sometimes the work can only be done on the server side. For instance,
 * somebody might want to create a 'cvs rstatus' command to show the
 * tags etc of files in a module that has not been checked out.
 * 
 * In other cases, it is desirable to have both client-side and
 * server-side parts to a new command; pragma provides a mechanism 
 * to allow both client and server parts to be combined into one script.
 * An example of when this might be useful is a command that needed to
 * operate on the local module/CVS directory and on the remote CVSROOT
 * directory. This would allow for some pretty flexible arrangements.
 *
 * SCRIPT INVOCATION
 * -----------------
 *
 * Because the same scripts are installed on both the clients and servers,
 * the first argument to the command when executed is one of 'client',
 * 'server' or 'callback'.
 *
 * An 'rstatus' scenario might run as follows:
 *
 * $ cvs pragma rstatus mymodule param1 param2
 *
 * Client side CVS finds and runs $CVSPRAGMADIR/rstatus as:
 *		$CVSPRAGMADIR/rstatus client param1 param2
 *
 * If the rstatus script needs access to the repository it can
 * invoke 'cvs pragma server rstatus params...' to run an
 * RPC on the server. Again CVSPRAGMADIR is used to locate
 * the server version of gettags (ideally this is the same script)
 * running it with 'server' as the first parameter, thus:
 *
 *		$CVSPRAGMADIR/rstatus server param1 param2
 *
 * Standard output from the server side script will be shown to the
 * user.
 *
 * Examples included.
 * ------------------
 *
 * Two examples are included with this initial release. One is a
 * pair of MSDOS batch files, the other a perl script. 
 *
 * To use these examples you need to copy the scripts into a directory
 * pointed to by $CVSPRAGMADIR on the client and server. 
 * 
 * client: 
 *
 * I've included two examples, the first example, separate_client,
 * uses a pair of files, separate_client & separate_server
 * one for each of the client and server sides.
 * 
 * The second example, 'combined', uses a single script. This is the 
 * preferred method.
 *
 * Notes 
 * -----
 *
 * cvs pragma command looks first on the client and, if it can't find
 * it there, on the server for the command.
 *
 * cvs pragma client command looks only on the client
 *
 * cvs pragma server command if run from the client side connects and
 * sends the command to the server.
 * 
 * cvs pragma server command if run from the server side looks only on 
 * the server.
 *
 * cvs pragma callback command is not yet implemented. This would allow
 * the server to make a callback RPC to the client.
 * 
 * I've done only limited testing on a Windows 95 OSR2 machine at home.
 * At some point I might test it on Solaris.
 * 
 * Note that the check for CVSROOT should be disabled. 
 * This allows maximum flexibility.
 *
 * Note that I have not tested this in client-server mode.
 *
 * Note that I don't consider myself a C programmer, I wanted
 * to write extensions in Perl and Java and this seemed the best way!
 */

#include <crtdbg.h>

#include "cvs.h"
#include "savecwd.h"
#include <stdarg.h>
#include <Assert.h>

static int pragma_callback PROTO((char* options, int verbose, int argc, char** argv));
static int pragma_client PROTO((char* options, int verbose, int argc, char** argv));
static int pragma_send_to_server PROTO((char* options, int verbose, int argc, char** argv));
static int pragma_receive_as_server PROTO((char* options, int verbose, int argc, char** argv));

static const char *const pragma_usage[] =
{
    "Usage: %s %s [client|server] pragma-program [options]\n",
    NULL
};


#define traceprint(message, arglist) \
	_CrtDbgReport(_CRT_WARN, NULL, 0, NULL, message, arglist);

int
pragma (argc, argv)
    int argc;
    char **argv;
{
    int c;
    char *options = NULL;
	int verbose = FALSE;
	char *mode = argv[1]; 

    if (argc == 1 || argc == -1)
	usage (pragma_usage);

    wrap_setup ();

    /* parse args */
    optind = 1;
    while ((c = getopt (argc, argv, "v")) != -1)
    {
	switch (c)
	{
	    case 'v':
		verbose = TRUE;
		break;

	    case '?':
	    default:
		usage (pragma_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    if (argc <= 0)
	usage (pragma_usage);

/*
	Must support 3 modes:
		pragma client prog == pragma prog
		pragma server prog
		pragma callback prog

	call the relevant routine without the switching word. It is expected that
	the routine will put the word back when calling the external program.
	This is mainly to implement  cvs pragma prog == cvs pragma client prog

	
*/


	traceprint("mode=%s\n", mode);
#ifdef CLIENT_SUPPORT
    if (client_active) { 
		/* We're the client side.  Fire up the remote server if called by 'pragma server'.*/	 

		if (strcmp(mode, "server") == 0) { 
			pragma_send_to_server(options, verbose, argc-1, argv+1);
		} else if (strcmp(mode, "client") == 0) {
			pragma_client(options, verbose, argc-1, argv+1);
		} else {
			pragma_on_client_or_server(options, verbose, argc, argv);
		}

	} else { 
#endif CLIENT_SUPPORT

		/* We're the server side or the user is running in local mode */
		if (strcmp(mode, "server") == 0) { 
			pragma_receive_as_server(options, verbose, argc-1, argv+1);
		} else if (strcmp(mode, "callback") == 0) {
			pragma_callback(options, verbose, argc-1, argv+1);
		} else if (strcmp(mode, "client") == 0) {
			pragma_client(options, verbose, argc-1, argv+1);
		} else {
			/* should this be pragma_on_client_or_server? */
			pragma_on_client_or_server(options, verbose, argc, argv);
			/*pragma_client(options, verbose, argc, argv);*/
		}
#ifdef CLIENT_SUPPORT
	}
#endif CLIENT_SUPPORT

	return 0;
}

static int
pragma_send_to_server(options, verbose, argc, argv)
    char *options;
	int verbose;
	int argc;
    char **argv;
{
	printf("pragma_send_to_server()\n");

	start_server ();
	ign_setup ();
	if (options) send_arg(options);
	option_with_arg ("-v", (verbose==1?"y":"n"));

    send_file_names (argc, argv, SEND_EXPAND_WILD);
	send_files (argc, argv, 0, 0);
	send_to_server ("pragma server\012", 0);
	return get_responses_and_close ();
} 


static int 
pragma_setup(mode, progname, verbose, argc, argv) 
    char *mode;
	char *progname;
	int  verbose;
	int  argc;
    char **argv;

{
	int i;
	char commandline[1024];

	printf("pragma_setup()\n");

	strcpy(commandline, progname);
	strcat(commandline, " ");
	strcat(commandline, mode);
	strcat(commandline, " ");

	for (i = 0; i < argc; i++) {
		printf ("argv[%i]=%s\n", i, argv[i]);
		strcat(commandline, argv[i]);
		strcat(commandline, " ");
	}
	printf("\n");

	printf("\n"); printf(commandline); printf("\n");
	if (isfile(progname)) {		/* FIXME: not necessarily isfile, 
									for instance user might say 'foo'
									and the associated command may run
									as 'foo.bat', 'foo.exe' etc.
								*/
		run_setup (commandline);
		run_print(stdout);
		printf("\n");
		return 1;
	} else {
		printf("%s not found", progname);
		return 0;
	}
}


	
static int
pragma_client(options, verbose, argc, argv)
    char *options;
	int verbose;
	int argc;
    char **argv;
{
	char *pragmabin = getenv("CVSPRAGMADIR");
	char progname[256]; /* FIXME: MAX_PATH_LEN */
	int retcode;

	printf("pragma_client()\n");

	if (pragmabin == NULL) {
		error(1, 0, "Please set CVSPRAGMADIR to the directory on the client holding client CVS programs\n");
	}

/* TODO: if progname is NULL, show a directory listing and then send command to server for it to do same */

	strncpy(progname, pragmabin, sizeof(progname));
	strcat(progname, argv[0]);

	if (pragma_setup("client", progname, verbose, argc-1, argv+1)) {
		retcode = run_exec (RUN_TTY, RUN_TTY, RUN_TTY, RUN_NORMAL);
		printf("run_exec returned %i\n", retcode);
	} else {
		printf("\nCouldn't run %s on client\n", progname);
	}

}

static int
pragma_on_client_or_server(options, verbose, argc, argv)
    char *options;
	int verbose;
	int argc;
    char **argv;
{
	char *pragmabin = getenv("CVSPRAGMADIR");
	char progname[256]; /* FIXME: MAX_PATH_LEN */
	int retcode;

	printf("pragma_on_client_or_server()\n");

	if (pragmabin == NULL) {
		error(1, 0, "Please set CVSPRAGMADIR to the directory on the client holding client CVS programs\n");
	}

	strncpy(progname, pragmabin, sizeof(progname));
	strcat(progname, argv[0]);

	if (pragma_setup("client", progname, verbose, argc-1, argv+1)) {
		retcode = run_exec (RUN_TTY, RUN_TTY, RUN_TTY, RUN_NORMAL);
		printf("run_exec returned %i\n", retcode);
	} else {
		printf("\nCouldn't run %s on client, looking on server\n", progname);
		if (pragma_setup("server", progname, verbose, argc-1, argv+1)) {
			retcode = run_exec (RUN_TTY, RUN_TTY, RUN_TTY, RUN_NORMAL);
			printf("server run_exec returned %i\n", retcode);
		}		
	}
}

static int
pragma_receive_as_server(options, verbose, argc, argv)
    char *options;
	int verbose;
	int argc;
    char **argv;
{
	char *pragmabin = getenv("CVSPRAGMADIR");
	char progname[256]; /* FIXME: MAX_PATH_LEN */
	int retcode;

	printf("pragma_receive_as_server()\n");

	if (pragmabin == NULL) {
		error(1, 0, "Please set CVSPRAGMADIR to the directory on the server holding client CVS programs\n");
	}

	strncpy(progname, pragmabin, sizeof(progname));
	strcat(progname, argv[0]);

	if (pragma_setup("server", progname, verbose, argc-1, argv+1)) {
		retcode = run_exec (RUN_TTY, RUN_TTY, RUN_TTY, RUN_NORMAL);
		printf("run_exec returned %i\n", retcode);
	} else {
		printf("couldn't run %s\n", progname);
	}
}

static int
pragma_callback(options, verbose, argc, argv)
    char *options;
	int verbose;
	int argc;
    char **argv;
{

	printf("pragma_callback()\n");
	printf("CVS pragma callback not yet implemented.\n");
	printf("When implemented this will mean a serverside script can \n");
	printf("run the client side script with more arguments\n");

	/* does CVS have the infrastructure needed to make this possible? */ 
}
