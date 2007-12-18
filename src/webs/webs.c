/*
 * main.c -- Main program for the GoAhead WebServer (LINUX version)
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: main.c,v 1.5 2003/09/11 14:03:46 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	Main program for for the GoAhead WebServer. This is a demonstration
 *	main program to initialize and configure the web server.
 */

/********************************* Includes ***********************************/

#include	"uemf.h"
#include	"wsIntrn.h"
#include	<signal.h>
#include	<unistd.h> 
#include	<sys/types.h>
#include	<sys/wait.h>

#ifdef WEBS_SSL_SUPPORT
#include	"websSSL.h"
#endif

#ifdef USER_MANAGEMENT_SUPPORT
#include	"um.h"
void	formDefineUserMgmt(void);
#endif

#include <getopt.h>
#include <errno.h>

extern void albaDefineFunctions(void);
extern void albaDbDefineFunctions(void);

void display_usage( void )
{
    printf( 
        "Usage: webs [OPTION]...\n"
        "   -d, --daemon[=pidfile] Start as daemon. Implies --run option.\n"
        "                          Default: /var/run/c54load.pid\n"
        "   -D, --dir=curdir       Change current working directory.\n"
        "   -p, --port=tcpport     Bind to specific tcp port.\n"
        "                          Default: 80\n"
        "   -a, --addr=ipaddr      Bind to specific ip address.\n"
        "                          Default: hostname lookup\n"
        "   -V, --help             Display this help and exit.\n"
        "   -v, --trace[=level]    Trace level.\n"
        "                          Default: -1 (disabled)\n"
        );
    }

/*********************************** Locals ***********************************/
/*
 *	Change configuration here
 */

static char_t		*rootWeb = T("web");			/* Root web directory */
static char_t		*password = T("");				/* Security password */
static int			port = 80;						/* Server port */
static int			retries = 5;					/* Server port retries */
static int			finished;						/* Finished flag */
static int          opt_daemon = 0;                 /* Run as daemon */
static char*        opt_pidfile = NULL;             /* Write pid to this file */
static char*        opt_chdir = NULL;               /* Change dir to this */
static int          opt_help = 0;                   /* Show usage */
static char*        opt_ipaddr = NULL;              /* Server ip address */
static int          opt_trace_level = -1;           /* Trace/Verbosity level */


/****************************** Forward Declarations **************************/

static int 	initWebs();
static int	aspTest(int eid, webs_t wp, int argc, char_t **argv);
static void formTest(webs_t wp, char_t *path, char_t *query);
static int  websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
				int arg, char_t *url, char_t *path, char_t *query);
extern void defaultErrorHandler(int etype, char_t *msg);
extern void defaultTraceHandler(int level, char_t *buf);
#ifdef B_STATS
static void printMemStats(int handle, char_t *fmt, ...);
static void memLeaks();
#endif

/*********************************** Code *************************************/
/*
 *	Main -- entry point from LINUX
 */

int main(int argc, char** argv)
{
    for( ;; )
    {
        int option_index = 0;
        static struct option long_options[] = 
        {
            { "daemon",  2, 0, 0   }, // -d [pidfile]
            { "dir",     1, 0, 0   }, // -D curdir
            { "help",    0, 0, 0   }, // -V
            { "port",    1, 0, 0   }, // -p port
            { "addr",    1, 0, 0   }, // -a addr
            { "trace",   2, 0, 0   }, // -v level
            { 0,         0, 0, 0   }
            };

        int c = getopt_long (argc, argv, "d::D:Vp:a:v::", long_options, &option_index );
        if ( c == -1 )
            break;

        if ( c == 0 )
        {
            /*
            printf ("option %s", long_options[option_index].name);
            if (optarg)
                printf (" with arg %s", optarg);
            printf ("\n");
            */

            switch( option_index )
            {
                case 0:  c = 'd'; break; // daemon
                case 1:  c = 'D'; break; // dir
                case 2:  c = 'V'; break; // help
                case 3:  c = 'p'; break; // port
                case 4:  c = 'a'; break; // addr
                case 5:  c = 'v'; break; // trace
                default: c = '?'; break; // ?unknown?
                }
            }

        switch( c ) 
        {
            case 'd':
                opt_daemon = 1;
                opt_pidfile = optarg;
                break;

            case 'D':
                opt_chdir = optarg;
                break;

            case 'V':
                opt_help = 1;
                break;

            case 'p':
                if ( optarg )
                    port = strtol( optarg, NULL, 10 );
                break;

            case 'a':
                opt_ipaddr = optarg;
                break;

            case 'v':
                if ( optarg )
                    opt_trace_level = strtol( optarg, NULL, 10 );
                break;

            case '?':
                display_usage ();
                return 1;
            }
        }

    if ( ! opt_pidfile ) 
        opt_pidfile = "/var/run/webs.pid";

    if ( opt_help )
    {
        display_usage ();
        return 0;
        }

    if ( opt_daemon )
    {
        // Become daemon and write our PID.
        //
        daemon( /*nochdir*/ 0, /*noclose*/ 0 );

        FILE* f = fopen( opt_pidfile, "w" );
        if ( f )
        {
            fprintf( f, "%d\n", getpid () );
            fclose( f );
        }
    }

	if ( opt_chdir )
	{
		if( chdir( opt_chdir ) < 0 ) // failed
		{
			fprintf( stderr, "Cannot set current directory to [%s]\n", opt_chdir );
			return 1;
		}
	}

/*
 *	Initialize the memory allocator. Allow use of malloc and start 
 *	with a 60K heap.  For each page request approx 8KB is allocated.
 *	60KB allows for several concurrent page requests.  If more space
 *	is required, malloc will be used for the overflow.
 */
	bopen(NULL, (60 * 1024), B_USE_MALLOC);
	signal(SIGPIPE, SIG_IGN);

/*
 *	Initialize the web server
 */
	if (initWebs() < 0) {
		return -1;
	}

#ifdef WEBS_SSL_SUPPORT
	websSSLOpen();
#endif

/*
 *	Basic event loop. SocketReady returns true when a socket is ready for
 *	service. SocketSelect will block until an event occurs. SocketProcess
 *	will actually do the servicing.
 */
	while (!finished) {
		if (socketReady(-1) || socketSelect(-1, 1000)) {
			socketProcess(-1);
		}
		websCgiCleanup();
		emfSchedProcess();
	}

#ifdef WEBS_SSL_SUPPORT
	websSSLClose();
#endif

#ifdef USER_MANAGEMENT_SUPPORT
	umClose();
#endif

/*
 *	Close the socket module, report memory leaks and close the memory allocator
 */
	websCloseServer();
	socketClose();
#ifdef B_STATS
	memLeaks();
#endif
	bclose();
	return 0;
}

/******************************************************************************/
/*
 *	Initialize the web server.
 */

static int initWebs()
{
	struct hostent	*hp;
	struct in_addr	intaddr;
	char			host[128], dir[128], webdir[128];
	char			*cp;
	char_t			wbuf[128];

/*
 *	Initialize the socket subsystem
 */
	socketOpen();

#ifdef USER_MANAGEMENT_SUPPORT
/*
 *	Initialize the User Management database
 */
	umOpen();
	umRestore(T("umconfig.txt"));
#endif

/*
 *	Define the local Ip address, host name, default home page and the 
 *	root web directory.
 */
	if (gethostname(host, sizeof(host)) < 0) {
        strcpy( host, "localhost" );
	}

    if ( opt_ipaddr ) {
        intaddr.s_addr = inet_addr( opt_ipaddr );
    } else {
	    if ((hp = gethostbyname(host)) == NULL) {
            intaddr.s_addr = inet_addr("127.0.0.1");
        }
        else {
	        memcpy((char *) &intaddr, (char *) hp->h_addr_list[0], (size_t) hp->h_length);
	    }
    }

/*
 *	Set ../web as the root web. Modify this to suit your needs
 */
	getcwd(dir, sizeof(dir)); 
	if ((cp = strrchr(dir, '/'))) {
		*cp = '\0';
	}
	sprintf(webdir, "%s/%s", dir, rootWeb);

/*
 *	Configure the web server options before opening the web server
 */
	websSetDefaultDir(webdir);
	cp = inet_ntoa(intaddr);
	ascToUni(wbuf, cp, min(strlen(cp) + 1, sizeof(wbuf)));
	websSetIpaddr(wbuf);
	ascToUni(wbuf, host, min(strlen(host) + 1, sizeof(wbuf)));
	websSetHost(wbuf);

/*
 *	Configure the web server options before opening the web server
 */
	websSetDefaultPage(T("default.asp"));
	websSetPassword(password);

/* 
 *	Open the web server on the given port. If that port is taken, try
 *	the next sequential port for up to "retries" attempts.
 */
	websOpenServer(port, retries);

/*
 * 	First create the URL handlers. Note: handlers are called in sorted order
 *	with the longest path handler examined first. Here we define the security 
 *	handler, forms handler and the default web page handler.
 */
	websUrlHandlerDefine(T(""), NULL, 0, websSecurityHandler, 
		WEBS_HANDLER_FIRST);
	websUrlHandlerDefine(T("/goform"), NULL, 0, websFormHandler, 0);
	websUrlHandlerDefine(T("/cgi-bin"), NULL, 0, websCgiHandler, 0);
	websUrlHandlerDefine(T(""), NULL, 0, websDefaultHandler, 
		WEBS_HANDLER_LAST); 

/*
 *	Now define two test procedures. Replace these with your application
 *	relevant ASP script procedures and form functions.
 */
	websAspDefine(T("aspTest"), aspTest);
	websFormDefine(T("formTest"), formTest);

/*
 *	Initialize Albatross special functions
 */
	albaDefineFunctions();
	albaDbDefineFunctions();

/*
 *	Create the Form handlers for the User Management pages
 */
#ifdef USER_MANAGEMENT_SUPPORT
	formDefineUserMgmt();
#endif

/*
 *	Create a handler for the default home page
 */
	websUrlHandlerDefine(T("/"), NULL, 0, websHomePageHandler, 0); 
	return 0;
}

/******************************************************************************/
/*
 *	Test Javascript binding for ASP. This will be invoked when "aspTest" is
 *	embedded in an ASP page. See web/asp.asp for usage. Set browser to 
 *	"localhost/asp.asp" to test.
 */

static int aspTest(int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*name, *address;

	if (ejArgs(argc, argv, T("%s %s"), &name, &address) < 2) {
		websError(wp, 400, T("Insufficient args\n"));
		return -1;
	}
	return websWrite(wp, T("Name: %s, Address %s"), name, address);
}

/******************************************************************************/
/*
 *	Test form for posted data (in-memory CGI). This will be called when the
 *	form in web/forms.asp is invoked. Set browser to "localhost/forms.asp" to test.
 */

static void formTest(webs_t wp, char_t *path, char_t *query)
{
	char_t	*name, *address;

	name = websGetVar(wp, T("name"), T("Joe Smith")); 
	address = websGetVar(wp, T("address"), T("1212 Milky Way Ave.")); 

	websHeader(wp);
	websWrite(wp, T("<body><h2>Name: %s, Address: %s</h2>\n"), name, address);
	websFooter(wp);
	websDone(wp, 200);
}

/******************************************************************************/
/*
 *	Home page handler
 */

static int websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
	int arg, char_t *url, char_t *path, char_t *query)
{
/*
 *	If the empty or "/" URL is invoked, redirect default URLs to the home page
 */
	if (*url == '\0' || gstrcmp(url, T("/")) == 0) {
		websRedirect(wp, T("home.asp"));
		return 1;
	}
	return 0;
}

/******************************************************************************/
/*
 *	Default error handler.  The developer should insert code to handle
 *	error messages in the desired manner.
 */

void defaultErrorHandler(int etype, char_t *msg)
{
	if ( msg ) {
		write( 1, msg, gstrlen(msg) );
	}
}

/******************************************************************************/
/*
 *	Trace log. Customize this function to log trace output
 */

void defaultTraceHandler(int level, char_t *buf)
{
	if ( buf && opt_trace_level >= level ) {
		write( 1, buf, gstrlen(buf) );
	}
}

/******************************************************************************/
/*
 *	Returns a pointer to an allocated qualified unique temporary file name.
 *	This filename must eventually be deleted with bfree();
 */

char_t *websGetCgiCommName()
{
	char_t	*pname1, *pname2;
	// pname1 = tempnam(NULL, T("cgi"));

	pname1 = malloc( 64 );
	strcpy( pname1, "/tmp/websCgiXXXXXX" );

	int fd = mkstemp( pname1 );
    if ( fd >= 0 )
        close( fd );

	pname2 = bstrdup(B_L, pname1);

	free(pname1);

    trace( 2, "websGetCgiCommName: %s\n", pname2 );

	return pname2;
}

/******************************************************************************/
/*
 *	Launch the CGI process and return a handle to it.
 */

int websLaunchCgiProc(char_t *cgiPath, char_t **argp, char_t **envp,
					  char_t *stdIn, char_t *stdOut)
{
	int	fdin, fdout, hstdin, hstdout, rc;

	fdin = fdout = hstdin = hstdout = rc = -1; 

	if ((fdin = open(stdIn, O_RDWR | O_CREAT, 0666)) < 0 ||
		(fdout = open(stdOut, O_RDWR | O_CREAT, 0666)) < 0 ||
		(hstdin = dup(0)) == -1 ||
		(hstdout = dup(1)) == -1 ||
		dup2(fdin, 0) == -1 ||
		dup2(fdout, 1) == -1) {
		goto DONE;
	}

 	rc = fork(); /* returns pid, 0 in the child or < 0 for error */
 	if (rc == 0) {
/*
 *		We are in the child process
 */
		if (execve(cgiPath, argp, envp) == -1) {
			printf(
                "content-type: text/html\n\n"
				"Execution of cgi process failed.\n"
            );
		}
		exit (0);
	} 

DONE:
	if (hstdout >= 0) {
		dup2(hstdout, 1);
        close(hstdout);
	}
	if (hstdin >= 0) {
		dup2(hstdin, 0);
        close(hstdin);
	}
	if (fdout >= 0) {
		close(fdout);
	}
	if (fdin >= 0) {
		close(fdin);
	}

    trace( 2, "CGI (pid %d): %s (%s)->(%s)\n", rc, cgiPath, stdIn, stdOut );

	return rc;
}

/******************************************************************************/
/*
 *	Check the CGI process.  Return 0 if it does not exist; non 0 if it does.
 */

int websCheckCgiProc(int handle)
{
/*
 *	Check to see if the CGI child process has terminated or not yet.  
 */
    pid_t p = waitpid( handle, NULL, WNOHANG );
    if ( p == 0 )
        return 1; // still runnning
/*
    if ( p < 0 )
        perror( "cgi waitpid:" );
    else
        fprintf( stderr, "CGI completed: pid %d\n", handle );
*/
    return 0;
}

/******************************************************************************/

#ifdef B_STATS
static void memLeaks() 
{
	int		fd;

	if ((fd = gopen(T("leak.txt"), O_CREAT | O_TRUNC | O_WRONLY, 0666)) >= 0) {
		bstats(fd, printMemStats);
		close(fd);
	}
}

/******************************************************************************/
/*
 *	Print memory usage / leaks
 */

static void printMemStats(int handle, char_t *fmt, ...)
{
	va_list		args;
	char_t		buf[256];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	write(handle, buf, strlen(buf));
}
#endif

/******************************************************************************/
