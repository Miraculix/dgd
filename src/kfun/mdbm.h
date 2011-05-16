/*
 * mdbm.h
 *
 * Main include file for the DGD database manager package.
 *
 * Includes the options include file which defines the database manager
 * to be used (GDBM or NDBM) and also (possibly) the path of the include 
 * file for this package.
 * The structure for database records is declared, it includes the database
 * manager access point, the integer handle that the mudlib sees and the
 * necessary variables for creation of a doubly linked list.
 * Auxiliary package functions (i.e. those that are not kfuns) are
 * prototyped.
 *
 * 16 May 1997		Modified	Greg Lewis
 *	# Converted from the MudOS version of this package (written by
 *	  me) for DGD.  Main change was PROT -> P.
 *	# Deleted many #define's and prototypes which are now extraneous.
 *
 * 22 May 1997		Modified	Greg Lewis
 *	# In testing and debugging changed the dgd include files that were
 *	  needed.
 *
 * 23 May 1997		Version 1.0	Greg Lewis
 *	# Added in a #define for some arbitrary number of ticks to be
 *	  added for an mdbm operation in general.  How good a number it
 *	  is I'm unsure.
 *
 */

#ifndef _MDBM_H
#define _MDBM_H


/*
 * MDBM_OPTIONS
 *
 * Before the the database package is compiled into the driver you should 
 * examine this section of the file and make sure the option selection is 
 * to your liking.
 * Note that you must select at least one of the different types of 
 * database managers!  The current supported types are GDBM and NDBM.
 * Note that internally to the mud the choice is in some ways irrelevant
 * since the kfuns will act exactly the same.  However ndbm has problems 
 * with the ability to move files around, and possibly tarring them as well.
 *
 */

/*
 * Define which database manager you wish to use.  You should ONLY define ONE of
 * these!
 *
 * The choices are:
 *
 * GDBM_MDBM
 * Define this if you want to use GDBM, the Gnu Database Manager.  This 
 * package was written as a replacement for the ndbm (New Database
 * Manager) and dbm (Database Manager) Unix database libraries.  GDBM is
 * an improved version of these two!  You may obtain the source from any
 * GNU source code repository (e.g. prep.ai.mit.edu).  I highly recommend
 * this as your choice as it fixes some of the problems associated with
 * both ndbm and dbm.
 *
 * NDBM_MDBM
 * Defining this will mean that the "New" Database Manager will be used.
 * Although, it is 10 years old.  Developed to fix shortcomings in the
 * original Database Manager distributed with Unix (BSD probably) it is
 * certainly a lot better than that package, allowing multiple databases
 * to be open at the same time.  However, like the original it shares (on
 * the systems I've seen anyway) the problem of the database files being
 * "sparse".  This tends to mean you can't shrink them in size, and more
 * importantly, you can't copy them around with cp, mv, etc.  This probably
 * extends to tarring them too, which is a bad thing.  Get gdbm and the
 * conversion program that comes with it.
 *
 */

#define	GDBM_MDBM
#undef	NDBM_MDBM


/* 
 * LOCAL_MDBM_INCLUDE
 *
 * If the include file for the database package is in a place which is not
 * searched by default, then #define where it is here.  If it is in a
 * standard place, i,e, #include <gdbm.h>|<ndbm.h>|<dbm.h> will find it,
 * then just leave this commented out.
 *
 * Obviously if you do choose to #define this you should make sure you
 * #define the correct include file to match your choice of database
 * managers above...
 */

/* #define	LOCAL_MDBM_INCLUDE	"/home/mydir/include/gdbm.h" */

/*
 * MDBM_OPEN_MODE
 *
 * Defines the file mode (see chmod(2) and open(2)) to be used when the  
 * actual database file is created.
 *
 * If you don't know or don't care about this the default is probably a good 
 * choice
 */

#define MDBM_OPEN_MODE	0600

/*
 * END MDBM_OPTIONS
 */


/*
 * Include the appropriate file for the various database managers.
 * First check whether the user has specified a local include file, and if
 * not assume its in a standard place.
 */

#ifdef LOCAL_MDBM_INCLUDE
#include	LOCAL_MDBM_INCLUDE
#else /* LOCAL MDBM_INCLUDE */
#   ifdef GDBM_MDBM
#      include	<gdbm.h>
#   else /* NDBM_MDBM */
#      include	<ndbm.h>
#      include	<fcntl.h>	/* Open flags */
#   endif /* GDBM_MDBM */
#endif /* LOCAL_MDBM_INCLUDE */

/*
 * Based on the choice of database managers, set up some function defines.
 */

#ifdef GDBM_MDBM
#   define MDBM_CLOSE		gdbm_close
#   define MDBM_FETCH		gdbm_fetch
#   define MDBM_STORE		gdbm_store
#   define MDBM_DELETE		gdbm_delete
#   define MDBM_FIRSTKEY	gdbm_firstkey
#   define MDBM_NEXTKEY		gdbm_nextkey
#   define MDBM_FREE(X)		free(X);
#else /* NDBM_MDBM */
#   define MDBM_CLOSE		dbm_close
#   define MDBM_FETCH		dbm_fetch
#   define MDBM_STORE		dbm_store
#   define MDBM_DELETE		dbm_delete
#   define MDBM_FIRSTKEY	dbm_firstkey
#   define MDBM_NEXTKEY		dbm_nextkey
#   define MDBM_FREE(X)		/* Blank - don't want to do anything */
#endif


/*
 * The struct used to store the necessary database information.
 * Set up as to allow the creation of a doubly linked list so that the 
 * connectivity list can keep track of what databases are open and find 
 * the one referred to by the different handles.
 */

typedef struct mdbm_s {
   struct mdbm_s *next;		/* The next element in the list */
   struct mdbm_s *previous;	/* The previous element in the list */
   int 		 handle;	/* The handle to refer to this database */
#ifdef GDBM_MDBM
   GDBM_FILE	 dbf;		/* GDBM database file */
#else /* NDBM_MDBM */
   DBM		 *dbf;		/* NDBM database struct pointer */ 
#endif
   char		 *mdbm_name;	/* The name of the database */
} mdbm_t;


/*
 * DGD include files needed for the protoyping below.
 */

#include 	"kfun.h"


/*
 * Prototypes for the various auxiliary or mudlib invisible functions
 * add_mdbm          - add an mdbm to the list of currently open ones.
 * remove_mdbm       - remove an mdbm from the list of currently open ones.
 * valid_mdbm	     - check a given handle corresponds to a database.
 * mdbm_check_handle - check this handle and name is allowable.
 */

#ifdef GDBM_MDBM
extern int       add_mdbm 	 		(int, char *, GDBM_FILE);
#else 
extern int       add_mdbm 	 		(int, char *, DBM *);
#endif 
extern void      remove_mdbm	 		(int);
extern mdbm_t    *valid_mdbm 	 		(int);
extern int       mdbm_check_handle 		(int, char *);


/*
 * Defines for return values for mdbm_check_handle
 */

#define MDBM_VALID_HANDLE	0
#define MDBM_INVALID_HANDLE	1
#define MDBM_HANDLE_DUPLICATE	2
#define MDBM_NAME_DUPLICATE	3

/*
 * A ball park figure for the ticks an mdbm operation takes.
 */

#define MDBM_TICKS	100

#endif /* _MDBM_H */
