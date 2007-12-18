
#include "ctype.h"
#include "emfdb.h"
#include "um.h"

#include "alba_db.h"

#include <getopt.h>
#include <errno.h>

int add_user( int didUsers, char_t* username, char_t* password );
int remove_user( int didUser, char_t* username );

static int    opt_trace_level = -1;           /* Trace/Verbosity level */
static char_t opt_DB_DIR[ 256 ] = ".";        /* Alba DB directory */
static int    opt_help = 0;                   /* show usage */
static int    opt_touch = 0;                  /* touch all databases */
static int    opt_add_user = 0;               /* add user */
static int    opt_remove_user = 0;            /* remove user */
static char_t opt_username[ 64 ] = "voice";   /* username */
static char_t opt_password[ 64 ] = "rec";     /* username */

void display_usage( void )
{
    printf( 
        "Usage: dbAlba [OPTION]...\n"
        "   -D, --dir=curdir       Change current working directory.\n"
        "   -T, --touch            Touch database.\n"
        "   -a, --add=username     Add user\n"
        "   -p, --pwd=password     User's password.\n"
        "   -r, --remove=username  Remove user.\n"
        "   -a, --addr=ipaddr      Bind to specific ip address.\n"
        "   -V, --help             Display this help and exit.\n"
        "   -v, --trace[=level]    Trace level.\n"
        "                          Default: -1 (disabled)\n"
        );
    }

int main(int argc, char** argv)
{
    for( ;; )
    {
        int option_index = 0;
        static struct option long_options[] = 
        {
            { "dir",     1, 0, 0   }, // -D curdir
            { "touch",   0, 0, 0   }, // -T
            { "add",     1, 0, 0   }, // -a
            { "pwd",     1, 0, 0   }, // -p
            { "remove",  1, 0, 0   }, // -r
            { "help",    0, 0, 0   }, // -V
            { "trace",   2, 0, 0   }, // -v level
            { 0,         0, 0, 0   }
            };

        int c = getopt_long (argc, argv, "D:Ta:p:r:V:v::", long_options, &option_index );
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
                case 0:  c = 'D'; break; // dir
                case 1:  c = 'T'; break; // touch
                case 2:  c = 'a'; break; // add
                case 3:  c = 'p'; break; // pwd
                case 4:  c = 'r'; break; // remove
                case 5:  c = 'V'; break; // help
                case 6:  c = 'v'; break; // trace
                default: c = '?'; break; // ?unknown?
                }
            }

        switch( c ) 
        {
            case 'D':
                strcpy( opt_DB_DIR, optarg ? optarg : "." );
                break;

            case 'T':
                opt_touch = 1;
                break;

            case 'a':
                opt_add_user = 1;
                opt_remove_user = 0;
                strcpy( opt_username, optarg ? optarg : "voice" );
                break;

            case 'p':
                strcpy( opt_password, optarg ? optarg : "rec" );
                break;

            case 'r':
                opt_remove_user = 1;
                opt_add_user = 0;
                strcpy( opt_username, optarg ? optarg : "voice" );
                break;

            case 'V':
                opt_help = 1;
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

    if ( opt_help )
    {
        display_usage ();
        return 0;
        }

	bopen(NULL, (60 * 1024), B_USE_MALLOC);

    int rc = 0;

    // Gateway Ports Database
    //
    int didGwPorts = dbOpen( NULL, NULL, NULL, 0);
    dbRegisterDBSchema( didGwPorts, &gatewayPortTable); 
    basicSetProductDir( didGwPorts, opt_DB_DIR );

    dbZero( didGwPorts ); 
    rc = dbLoad( didGwPorts, GW_DB_NAME, 0  );
    if ( rc )
    {
        printf( "Failed to load database %s/%s!\n", opt_DB_DIR, GW_DB_NAME );
        return 1;
        }

    // Extender Ports Database
    //
    int didExtPorts = dbOpen( NULL, NULL, NULL, 0);
    dbRegisterDBSchema( didExtPorts, &extenderPortTable);
    basicSetProductDir( didExtPorts, opt_DB_DIR );

    dbZero( didExtPorts ); 
    rc = dbLoad( didExtPorts, EXT_DB_NAME, 0  );
    if ( rc )
    {
        printf( "Failed to load database %s/%s!\n", opt_DB_DIR, EXT_DB_NAME );
        return 1;
        }

    // Users & Authorizations Database
    //
    int didUsers = dbOpen( NULL, NULL, NULL, 0);
    dbRegisterDBSchema( didUsers, &userTable); // Users
    dbRegisterDBSchema( didUsers, &userAuthTable); // Authorizations
    basicSetProductDir( didUsers, opt_DB_DIR );

    dbZero( didUsers ); 
    rc = dbLoad( didUsers, USERS_DB_NAME, 0  );
    if ( rc )
    {
        printf( "Failed to load database %s/%s!\n", opt_DB_DIR, USERS_DB_NAME );
        return 1;
        }

    // Touch all databases
    //
    if ( opt_touch )
    {
        rc = dbSave( didGwPorts, GW_DB_NAME, 0 );
        if ( rc )
        {
            printf( "Failed to save database %s/%s!\n", opt_DB_DIR, GW_DB_NAME );
            return 1;
            }
        rc = dbSave( didExtPorts, EXT_DB_NAME, 0 );
        if ( rc )
        {
            printf( "Failed to save database %s/%s!\n", opt_DB_DIR, EXT_DB_NAME );
            return 1;
            }
        rc = dbSave( didUsers, USERS_DB_NAME, 0 );
        if ( rc )
        {
            printf( "Failed to save database %s/%s!\n", opt_DB_DIR, USERS_DB_NAME );
            return 1;
            }
        }
    else if ( opt_add_user )
    {
        return add_user( didUsers, opt_username, opt_password );
        }
    else if ( opt_remove_user )
    {
        return remove_user( didUsers, opt_username );
        }

	bclose();
	return 0;
    }

int remove_user( int didUsers, char_t* username )
{
    int rid = dbSearchStr( didUsers, USERS_TABLE, "NAME", username, 0 ); 
    if ( rid < 0 )
    {
        fprintf( stderr, "User %s does not exist!\n", username );
        return 1;
        }

    // delete user
    dbDeleteRow( didUsers, USERS_TABLE, rid ); 

    int nrow = dbGetTableNrow( didUsers, USERAUTHS_TABLE ); // get number of rows
    for ( rid = 0; rid < nrow; rid++ )
    {
        char* NAME=NULL;    
        int rc = dbReadStr( didUsers, USERAUTHS_TABLE, "USERNAME", rid, &NAME );
        if ( rc == DB_OK && strcmp( NAME, username ) == 0 )
            // deleting all users authorizations
            dbDeleteRow( didUsers, USERAUTHS_TABLE, rid );
        }

    // Save database assuming that other can access current database file.
    //
    int rc = dbSave( didUsers, USERS_DB_NAME, 0 );
    if ( rc )
    {
        printf( "Failed to save database %s/%s!\n", opt_DB_DIR, USERS_DB_NAME );
        return 1;
        }

    return 0;
    }

int add_user( int didUsers, char_t* username, char_t* password )
{
    int rid = dbSearchStr( didUsers, USERS_TABLE, "NAME", username, 0 ); 
    if ( rid >= 0 )
    {
        fprintf( stderr, "User %s already exists.\n", username );
        return 1;
        }

    // Adding new user to USERS_TABLE
    //
    rid = dbAddRow( didUsers, USERS_TABLE );
    if ( rid < 0 )
    {
        fprintf( stderr, "Failed to add user %s!\n", username );
        return 1;
        }

    // Get row pointer
    //
    USER* rec = dbGetRowPtr( didUsers, USERS_TABLE, rid );
    if ( ! rec )
        return 1;

    // Update username (other columns will have default values)
    //
    dbUpdateStr( &rec->NAME, username );

    // Adding default authorization to USERAUTHS_TABLE
    //
    rid = dbAddRow( didUsers, USERAUTHS_TABLE );
    if ( rid < 0 )
    {
        fprintf( stderr, "Cannot add default authorization for user %s!\n", username );
        return 1;
        }

    int rc = dbWriteStr( didUsers, USERS_TABLE, "PASS", rid, password );

    USERAUTH* rec2 = dbGetRowPtr( didUsers, USERAUTHS_TABLE, rid );
    if ( ! rec2 )
        return 1;

    // Update username
    //
    dbUpdateStr( &rec2->USERNAME, username );

    // Save database assuming that other can access current database file.
    //
    rc = dbSave( didUsers, USERS_DB_NAME, 0 );
    if ( rc )
    {
        printf( "Failed to save database %s/%s!\n", opt_DB_DIR, USERS_DB_NAME );
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
 * Compare strings, ignoring case:  normal strcmp return codes.
 *
 *	WARNING: It is not good form to increment or decrement pointers inside a
 *	"call" to tolower et al. These can be MACROS, and have undesired side
 *	effects.
 */

int strcmpci(char_t *s1, char_t *s2)
{
	int		rc;

	a_assert(s1 && s2);
	if (s1 == NULL || s2 == NULL) {
		return 0;
	}

	if (s1 == s2) {
		return 0;
	}

	do {
		rc = gtolower(*s1) - gtolower(*s2);
		if (*s1 == '\0') {
			break;
		}
		s1++;
		s2++;
	} while (rc == 0);
	return rc;
}


