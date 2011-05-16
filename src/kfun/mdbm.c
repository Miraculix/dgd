/*
 * mdbm.c
 *
 * The actual implementation of the mud database manager package.
 * DGD version.
 *
 * 4 May 1997		Begun		Greg Lewis
 *	# Begun adaptation from the MudOS version.  Rather than have many
 *	  #ifdef's this will be done separately.
 *	# This version, in keeping with DGD, will be more minimalistic, 
 *	  but the necessary functionality will be provided in order that
 *	  people can write efuns which both override these kfuns and add
 *	  additional efuns so that the full functionality of the MudOS
 *	  package can be achieved, albeit at some performance loss.
 *
 * 18 May 1997		Modified	Greg Lewis
 *	# Finished initial DGD conversion save for a call to i_add_ticks
 *	  in each function.  Values for this will have to be worked out
 *	  somewhat by trial and error I suspect.
 *
 * 22 May 1997		Modified	Greg Lewis
 *	# Tested and debugged DGD version.  Everything appears to work
 *	  now.  The i_add_ticks call still needs to be added though.
 *
 * 23 May 1997		Version 1.0 	Greg Lewis
 *	# Added in i_add_ticks call.  There is a general arbitrary cost
 *	  associated with a given mdbm operation and to this is generally
 *	  added the size of any string which needs allocating/copying.
 *	  The exception is mdbm_reorganize, which is considered to be
 *	  expensive.
 *
 * 20 June 1997		Modified	Greg Lewis
 *	# Minor clean up of code due to output of gcc -Wall.
 *
 * 22 September 1997	Version 1.1	Greg Lewis
 *	# Clean up of code to be slightly faster and more DGD-like.  
 *	  The stored length of DGD strings is generally used rather than
 *	  calculating this using strlen and memcpy is used instead of strcpy
 *	  as per the DGD main source code.  
 *	# This addition means that keys and values with '\0' in them can
 *	  now be stored.
 *
 * 18 December 1997	Version 1.1a	Greg Lewis
 *	# Made memory allocations for the mdbm struct and database name static 
 *	  since dynamic memory is cleared at swapout.  Will check that a swapout
 *	  can't happen during a kfun (I can't see how it could!).
 *
 * TBD
 *	# Documentation.
 *	# Consider our own "fatal function".
 */
	

/* Include files */
#ifdef MDBM_SUPPORT

#ifndef FUNCDEF
#include	<string.h>	/* For strcpy, strcmp, memcpy, etc. */
#include	<sys/stat.h>	/* For stat */
#include 	"mdbm.h"	/* All the MDBM declarations */	

#define 	INCLUDE_FILE_IO
#include	"path.h"


/* Variable declarations and initializations */

/*
 * This is the list of open databases.  This works by virtue of the link
 * elements provided in the mdbm_t struct.
 */
static mdbm_t *MDBM_open_list = (mdbm_t *) NULL;

/*
 * The handle of the current database we are dealing with.  Should be set
 * by the efunctions _not_ any of the auxiliary functions (although they
 * may clear it).
 */
int mdbm_current_handle = 0;


/* Some of the auxiliary functions */

/*
 * int
 * add_mdbm(int handle, char *name, DBM*|GDBM_FILE dbf, int mode)
 *
 * This function allocates memory for an mdbm_t struct.  It assigns to
 * it the handle and name given and inserts it into the doubly linked
 * list of mdbm's (MDBM_open_list) that are open.
 *
 * Called by mdbm_open.
 *
 * Returns 1 on success, 0 on failure.
 *
 * handle - the integer handle by which the mdbm is to be referred to
 *	    by the other mdbm efunctions.
 * name   - the file name of the mdbm.  This is important to check that
 *          the same database is not opened twice.
 * dbf	  - the pointer to the database file returned by whichever database
 *	    manager is being used.
 */

int 
#ifdef GDBM_MDBM
add_mdbm(int handle, char *name, GDBM_FILE dbf) {
#else /* NDBM_MDBM */
add_mdbm(int handle, char *name, DBM *dbf) {
#endif
   mdbm_t *mdbm;
    
   /*
    * Allocate static memory for the mdbm struct
    */
   m_static();
   mdbm = ALLOC(mdbm_t, 1);
   m_dynamic();

   /*
    * Return 0, indicating failure, if the memory hasn't been allocated.
    */
   if (!mdbm) {
      /* Fail */
      return 0; 
   }

   /* 
    * Allocate static memory for the name of the database
    */
   m_static();
   mdbm->mdbm_name = ALLOC(char, (strlen(name) + 1));
   m_dynamic();

   /*
    * Return 0, indicating failure, if the memory hasn't been allocated.
    */
   if (!mdbm->mdbm_name) {
      /* Fail */
      return 0; 
   }

   /*
    * Copy the name, handle, dbf and mode to their proper places in the mdbm 
    * struct.
    */
   strcpy(mdbm->mdbm_name, name);

   mdbm->handle = handle;

   mdbm->dbf = dbf;

   /*
    * Set up the previous and next pointers for our doubly linked list.
    * Note that list elements are added in at the front.
    */
   if (MDBM_open_list == (mdbm_t *) NULL ) {
      /*
       * List creation if it is currently blank
       */
      mdbm->previous = (mdbm_t *) NULL;
      mdbm->next = (mdbm_t *) NULL;
      MDBM_open_list = mdbm;
   }
   else {
      /*
       * Add the mdbm to the front of the list.
       */
      mdbm->next = MDBM_open_list;
      MDBM_open_list = mdbm;
   }

   /*
    * Indicate success
    */
   return 1;
}

/*
 * void
 * remove_mdbm(int handle)
 *
 * Removes from the linked list the database specified by handle.  
 * Allocated memory is also freed.
 *
 * handle - The "handle" of the particular database that is to be removed.
 */

void 
remove_mdbm(int handle) {
   mdbm_t *tmp_mdbm, *previous_mdbm, *next_mdbm;

   /*
    * Get the database which corresponds to handle (if there is one)
    */
   tmp_mdbm = valid_mdbm(handle);

   /*
    * Check that there was in fact such a database.
    * If the logic of this package is correct this should never happen!
    */
   if (tmp_mdbm == (mdbm_t *) NULL) {
      return;
   }

   /*
    * Now the previous and next elements in the list are known
    * The database is removed from the list by forming the appropriate
    * new links between the previous and next databases in the list.
    */
   next_mdbm = tmp_mdbm->next;
   previous_mdbm = tmp_mdbm->previous;
   if (tmp_mdbm->previous != (mdbm_t *) NULL) {
      (tmp_mdbm->previous)->next = tmp_mdbm->next;
   }
   else {
      MDBM_open_list = tmp_mdbm->next;
   }
   if (tmp_mdbm->next != (mdbm_t *) NULL) {
      (tmp_mdbm->next)->previous = tmp_mdbm->previous;
   }

   /*
    * Free the memory associated with the database that has been removed from
    * the list.
    */
   FREE(tmp_mdbm->mdbm_name);
   FREE(tmp_mdbm);
}

/*
 * mdbm_t *
 * valid_mdbm(int handle)
 *
 * Check if the given handle corresponds to an actual database.
 * Returns the a pointer to the database for success or NULL on failure.
 *
 * handle - The number that is to be checked as a valid database "handle"
 *
 * Check first for valid handles? i.e. >= 0?
 */

mdbm_t 
*valid_mdbm(int handle) {
   mdbm_t 	*tmp_mdbm;

   /*
    * Get the first element of the linked list of databases
    */
   tmp_mdbm = MDBM_open_list;

   /*
    * Check that such a list in fact currently exists.
    * Fail if it doesn't.
    */
   if (tmp_mdbm == (mdbm_t *) NULL) {
      return (mdbm_t *) NULL;
   }

   /*
    * Search through the list to find the database that is wanted.
    */
   while (tmp_mdbm->handle != handle) {
      if (tmp_mdbm->next == (mdbm_t *) NULL) {
	 /*
	  * End of list, corresponding database not found.
	  * Fail and return NULL
	  */
         return (mdbm_t *) NULL;
      }
      tmp_mdbm = tmp_mdbm->next;
   }

   /*
    * Return the corresponding database.
    */
   return tmp_mdbm;
}

/*
 * int
 * check_handle(int handle, char *name)
 *
 * Checks with a given database handle (and name, optionally) are "valid".
 * Valid being defined as unused or in the range of acceptable values.
 * Returns MDBM_VALID_HANDLE on success or one of the following error codes.
 *	MDBM_INVALID_HANDLE	- This number is invalid for use as a handle.
 *	MDBM_HANDLE_DUPLICATE	- This handle is in use.
 *	MDBM_NAME_DUPLICATE	- This name (and hence database file) is in use.
 *
 * handle - The number to be checked as a "valid" handle.
 * name   - The name (optional) to be checked as a "valid" name.
 */

int
mdbm_check_handle(int handle, char * name) {
   mdbm_t *tmp_mdbm;

   /*
    * Handles are numbered 1..MAXINT, so those which are negative or 0
    * are rejected as invalid.
    */
   if (handle <= 0) {
      return MDBM_INVALID_HANDLE;
   }

   /*
    * Get the first element of the linked list of databases
    */
   tmp_mdbm = MDBM_open_list;

   /*
    * Loop through the list, looking for handles or names that are already in
    * use.  Return an error code if so.
    */
   while (tmp_mdbm != (mdbm_t *) NULL) {
      if (handle == tmp_mdbm->handle) {
	 return MDBM_HANDLE_DUPLICATE;
      }
      if (name && !strcmp(name, tmp_mdbm->mdbm_name)) {
	 return MDBM_NAME_DUPLICATE;
      }
      tmp_mdbm = tmp_mdbm->next;
   }

   /* 
    * Success!  The handle (and name if given) are both "valid".
    * Indicate this with the return value.
    */
   return MDBM_VALID_HANDLE; /* Success */
}
#endif /* FUNCDEF */

/* Kfunctions */

#ifdef FUNCDEF
FUNCDEF("mdbm_open", kf_mdbm_open, pt_mdbm_open, 0)
#else
char pt_mdbm_open[] = { C_TYPECHECKED | C_STATIC, 1, 0, 0, 7, T_INT, T_STRING };

/*
 * kf_mdbm_open(string mdbm_name)
 *
 * The kfunction that interfaces with the file opening routine of the
 * database manager.
 *
 * Get a useable handle for the mdbm, check that this database file is not
 * already open (via check_mdbm).  Then the mudlib is queried as to whether
 * the object in question has the appropriate read/write permissions.  Then
 * the appropriate database manager is called to open the file and upon the
 * success the mdbm is added to the linked list of open databases. The database
 * is tagged with a mode upon opening, either MUD or RAW.
 *
 * mdbm_name	- The file name of the database file.
 *
 * On failure, zero is returned, with success returning the handle of the 
 * newly opened database connection.
 */

int
kf_mdbm_open(frame *f) 
{
   int 		handle = 0;
   char		*mdbm_file;
#ifdef GDBM_MDBM
   GDBM_FILE	new_dbf;
#else /* NDBM_MDBM */
   DBM		*new_dbf = (DBM *) 0;
   char		*mdbm_dir_name;
#endif
   char file[STRINGSZ];

   /* 
    * Find a new handle that can be given this mdbm 
    * (It would be cheaper and possibly smarter to increment handle?)
    */
   while (mdbm_check_handle(++handle, (char *) 0) != MDBM_VALID_HANDLE)
      ;

   /* 
    * Check validity.  The database should only be able to be opened
    * once, so this check is primarily versus the file name.
    */
   if (mdbm_check_handle(handle, f->sp->u.string->text) != MDBM_VALID_HANDLE) {
      str_del(f->sp->u.string);
      PUT_INTVAL(f->sp, 0);
      return 0;
   }

   /* 
    * Expand file name.
    */
    if ( path_string(file,f->sp->u.string->text, f->sp->u.string->len) == (char *)NULL)
    	{
      str_del(f->sp->u.string);
     PUT_INTVAL(f->sp, 0);
      return 0;
   }
   mdbm_file = file;

   /*
    * Take off some execution ticks.
    */
   i_add_ticks(f,MDBM_TICKS + f->sp->u.string->len);

#ifdef GDBM_MDBM
   /* 
    * Now try gdbm_open, with the appropriate flags 
    * block_size == 0 => Use the file system blocksize
    * (Seems to be a good idea)
    * Also should think about making a fatal function 
    * (when I know what its for and if we can use it)
    */
   new_dbf = gdbm_open(mdbm_file, 0, GDBM_WRCREAT, MDBM_OPEN_MODE, 
		       (void (*)) NULL);
#else /* NDBM_MDBM */
   /*
    * Try the ndbm routine dbm_open, with the appropriate flags.
    */
   new_dbf = dbm_open(mdbm_file, O_RDWR|O_CREAT, MDBM_OPEN_MODE);
#endif

   /* 
    * Add in the database to the linked list if its been opened succesfully, 
    * else error. 
    */
   if (new_dbf && add_mdbm(handle, mdbm_file, new_dbf)) {
      str_del(f->sp->u.string);
     PUT_INTVAL(f->sp, handle);
      return 0;
   }
   else {
      /*
       * If its add_mdbm that failed then close the database
       */
      if (new_dbf) {
	 MDBM_CLOSE(new_dbf);
      }
      /* 
       * Should be checking gdbm_errno or errno here and writing an error? 
       */
      str_del(f->sp->u.string);
     PUT_INTVAL(f->sp, 0);
      return 0;
   }
}
#endif /* FUNCDEF */


#ifdef FUNCDEF
FUNCDEF("mdbm_close", kf_mdbm_close, pt_mdbm_close,0)
#else
char pt_mdbm_close[] = { C_TYPECHECKED | C_STATIC, 1, 0, 0, 7, T_VOID , T_INT };

/*
 * kf_mdbm_close(int handle)
 *
 * The kfunction that interfaces with the database manager file closing
 * routine.
 * Checks that we have a valid and known handle and closes the database using
 * the appropriate database manager close routine.  The database is then
 * removed from the linked list.
 *
 * handle	- The handle of the database to be closed.
 *
 * Returns 1 for success, 0 for failure.
 */

int
kf_mdbm_close(frame *f) 
{
   mdbm_t 	*tmp_mdbm;

   /*
    * Set the current handle
    */
   mdbm_current_handle = f->sp->u.number;
   
   /* 
    * Check the handle is valid and known
    */
   if ((tmp_mdbm = valid_mdbm(mdbm_current_handle)) == (mdbm_t *) NULL) {
      *f->sp = nil_value;
      return 0; 
   }

   /*
    * Take off some execution ticks.
    */
   i_add_ticks(f,MDBM_TICKS);

   /* 
    * Close the database using the database closing routine.
    */
   MDBM_CLOSE(tmp_mdbm->dbf);

   /* 
    * Remove the database from the linked list 
    */
   remove_mdbm(mdbm_current_handle);

   /* 
    * Clear the current handle
    */
   mdbm_current_handle = 0;

   /*
    * Success!
    */
  *f->sp = nil_value;
   return 0;
}
#endif /* FUNCDEF */


#ifdef GDBM_MDBM

#ifdef FUNCDEF
FUNCDEF("mdbm_reorganize", kf_mdbm_reorganize, pt_mdbm_reorganize,0)
#else
char pt_mdbm_reorganize[] = { C_TYPECHECKED | C_STATIC, 1, 0, 0, 7, T_INT, T_INT };

/*
 * kf_mdbm_reorganize(int handle)
 *
 * The kfunction that interfaces with the database manager file reorganization
 * routine.  
 *
 * Note that this kfunction is only available when the GDBM database manager
 * is being used.  It is used after many deletions have been done on the
 * database to shrink the associated file.
 * Checks that we have a valid and known handle and reorganizes the database 
 * using the database manager reorganization routine.  This only works for
 * GDBM databases.
 *
 * handle	- The handle of the database to be reorganized.
 *
 * Returns 1 for success, 0 for failure.
 */

int
kf_mdbm_reorganize(frame *f) {
   mdbm_t 	*tmp_mdbm;
   int		result;

   /*
    * Set the current handle
    */
   mdbm_current_handle = f->sp->u.number;

   /* 
    * Check the handle is valid and known
    */
   if ((tmp_mdbm = valid_mdbm(mdbm_current_handle)) == (mdbm_t *) NULL) {
      PUT_INTVAL(f->sp, 0);
      return 0;
   }

   /*
    * Take off some execution ticks.
    * This is generally an expensive operation.
    */
   i_add_ticks(f,10*MDBM_TICKS);

   /* 
    * Reorganize the database using the database reorganizing routine.
    */
   result = gdbm_reorganize(tmp_mdbm->dbf);

   /* 
    * Clear the current handle
    */
   mdbm_current_handle = 0;

   if (!result) {
      /*
       * Success!
       */
      PUT_INTVAL(f->sp, 1);
      return 0;
   }
   else {
      /*
       * Call to gdbm_reorganize failed.
       */
      PUT_INTVAL(f->sp, 0);
      return 0;
   }
}
#endif /* FUNCDEF */

#endif /* GDBM_MDBM */


#ifdef FUNCDEF
FUNCDEF("mdbm_store", kf_mdbm_store, pt_mdbm_store,0)
#else
char pt_mdbm_store[] = { C_TYPECHECKED | C_STATIC| C_ELLIPSIS, 3, 1, 0, 10, T_INT, T_INT ,
                          T_STRING, T_STRING, T_INT };

/*
 * kf_mdbm_store(int handle, string key, string value, void|int insert_flag)
 *
 * The kfunction that interfaces with the database manager key/value storage
 * routine.
 * Checks that we have a valid and known handle and stores the key and
 * value using the database manager storage routine.
 *
 * handle	- The handle of the database to be reorganized.
 * key		- The "key" with which to reference the value.
 * value	- The data wanted to stored in the database.
 * insert_flag	- Whether to overwrite the value if the key already exists.
 *
 * Returns 1 for success, 0 for failure.
 */

int
kf_mdbm_store(frame *f,int nargs) {
   mdbm_t	*tmp_mdbm;
   datum	mdbm_key;
   datum	mdbm_value;
   int		result;
   int 		insert_flag;
   int		handle;

   /*
    * Check that the right number of arguments are passed.
    */
   switch (nargs) {
      case 0 : case 1 : case 2 :
	 return -1;
	 break;
      case 4 :
         /*
          * Get the optional insertion flag.
          */
#ifdef GDBM_MDBM
         insert_flag = (f->sp++->u.number) ? GDBM_INSERT : GDBM_REPLACE;
#else /* NDBM_MDBM */
         insert_flag = (f->sp++)->u.number) ? DBM_INSERT : DBM_REPLACE;
#endif
	 break;
      case 3 : 
         /*
          * Default to replacement of existing key data.
          */
#ifdef GDBM_MDBM
         insert_flag = GDBM_REPLACE;
#else /* NDBM_MDBM */
         insert_flag = DBM_REPLACE;
#endif
	 break;
      default :
	 return ++nargs;
	 break;
   }

   /* 
    * Get the handle 
    */
   handle = f->sp[2].u.number;

   /*
    * Set the current handle
    */
   mdbm_current_handle = handle;

   /* 
    * Check its a valid mdbm 
    */
   if (!(tmp_mdbm = valid_mdbm(handle))) {
   		message("Handle invalid");
      str_del(f->sp++->u.string);
      str_del(f->sp++->u.string);
      f->sp->u.number = 0;
      return 0;
   }

   /*
    * Take off some execution ticks.
    */
   i_add_ticks(f,MDBM_TICKS + f->sp[1].u.string->len + f->sp->u.string->len);

   /*
    * Allocate datum appropriately
    */
   mdbm_key.dsize = (f->sp[1].u.string->len);
   mdbm_value.dsize = (f->sp->u.string->len);
   mdbm_key.dptr = ALLOC(char, mdbm_key.dsize);
   mdbm_value.dptr = ALLOC(char, mdbm_value.dsize);

   /*
    * Copy the key and value values into strings for save and restore 
    * using the appropriate routines.
    */
   memcpy(mdbm_key.dptr, f->sp[1].u.string->text, f->sp[1].u.string->len);
   memcpy(mdbm_value.dptr, f->sp->u.string->text, f->sp->u.string->len);

   /* 
    * result will be 0 on success 
    */
   result = MDBM_STORE(tmp_mdbm->dbf, mdbm_key, mdbm_value, insert_flag);
   message("store result = %d (flag = %d)\n",result,insert_flag);
   /* 
    * Free the value and key dptr's 
    */
   FREE(mdbm_key.dptr);
   FREE(mdbm_value.dptr);

   /*
    * Clear the current handle
    */
   mdbm_current_handle = 0;

   /* 
    * Frees the value and key, indicate success or failure 
    */
   str_del(f->sp++->u.string);
   str_del(f->sp++->u.string);
   f->sp->type = T_INT; 
   f->sp->u.number = (!result) ? 1 : 0;
   return 0;
}
#endif /* FUNCDEF */


#ifdef FUNCDEF
FUNCDEF("mdbm_fetch", kf_mdbm_fetch, pt_mdbm_fetch,0)
#else
char pt_mdbm_fetch[] = { C_TYPECHECKED | C_STATIC, 2, 0, 0, 8, T_STRING,
			 T_INT, T_STRING };

/*
 * kf_mdbm_fetch(int handle, string key)
 *
 * The kfunction that interfaces with the database manager key/value fetch
 * routine.  
 * Checks that we have a valid and known handle and fetches the data and
 * associated with the given key, if it exists, using the database manager 
 * fetch routine.
 *
 * handle	- The handle of the database.
 * key		- The "key" with which to reference the stored data.
 *
 * Returns the value string on success, 0 on failure.
 */

int
kf_mdbm_fetch(frame *f) {
   int		handle;
   mdbm_t 	*tmp_mdbm;
   datum  	mdbm_key;
   datum 	mdbm_value;

   /* 
    * Get the handle 
    */
   handle = f->sp[1].u.number;

   /*
    * Set the current handle
    */
   mdbm_current_handle = handle;

   /* 
    * Check its a valid mdbm 
    */
   if (!(tmp_mdbm = valid_mdbm(handle))) {
      str_del(f->sp++->u.string);
      f->sp->u.number = 0;
      return 0;
   }

   /*
    * Take off some execution ticks.
    */
   i_add_ticks(f,MDBM_TICKS + f->sp->u.string->len);

   /*
    * Copy key to the datum structure needed for the database call.
    */
   mdbm_key.dsize = f->sp->u.string->len;
   mdbm_key.dptr = ALLOC(char, mdbm_key.dsize);
   memcpy(mdbm_key.dptr, f->sp->u.string->text, f->sp->u.string->len);
   str_del(f->sp++->u.string);

   /*
    * On success, mdbm_value.dtpr will contain the data matching the key,
    * on failure it will be NULL.
    */
   mdbm_value = MDBM_FETCH(tmp_mdbm->dbf, mdbm_key);

   /*
    * On failure, free memory and return undefined.
    */
   if (mdbm_value.dptr == NULL) {
      FREE(mdbm_key.dptr);
      /* sp->type = T_INT; */
      f->sp->u.number = 0;
      return 0;
   }

   /*
    * Take off some execution ticks.
    */
   i_add_ticks(f,mdbm_value.dsize);

   /*
    * Allocate and copy the string
    */
   f->sp->type = T_STRING;
   str_ref(f->sp->u.string = str_new(mdbm_value.dptr, mdbm_value.dsize));

   /* 
    * Careful freeing mdbm_value.dptr, it has been malloc'ed by gdbm 
    * internally, not by the driver. 
    */
   FREE(mdbm_key.dptr);
   MDBM_FREE(mdbm_value.dptr);

   /*
    * Clear the current handle
    */
   mdbm_current_handle = 0;

   /*
    * Successful completion
    */
   return 0;
}
#endif /* FUNCDEF */


#ifdef FUNCDEF
FUNCDEF("mdbm_exists", kf_mdbm_exists, pt_mdbm_exists,0)
#else
char pt_mdbm_exists[] = { C_TYPECHECKED | C_STATIC| C_ELLIPSIS, 1, 0, 0, 8, T_INT, T_INT ,
                          T_STRING };

/*
 * kf_mdbm_exists(int handle, string key)
 *
 * The kfunction that interfaces with the database manager key/value existence
 * routine.
 * Checks that we have a valid and known handle and then checks if the database
 * contains a key/value pair with the given key using the database manager 
 * existence routine.
 *
 * handle	- The handle of the database.
 * key		- The database "key" whose existence is in question.
 *
 * Returns 1 on success, 0 on failure.
 */

int
kf_mdbm_exists(frame *f,int nargs) {
   mdbm_t 	*tmp_mdbm;
   datum  	mdbm_key;
   int		result;
   int		handle;
#ifdef NDBM_MDBM
   datum	mdbm_value;
#endif 

   /*
    * Check that the right number of arguments are passed.
    */
   switch (nargs) {
      case 0 :
	 return -1;
	 break;
      case 1 :
         /*
          * Deal with the case where it is only wished to check for the 
	  * existence of a database with the given handle.
          * sp->u.number will be the handle initially 
          */
         f->sp->u.number = (valid_mdbm(f->sp->u.number)) ? 1 : 0;
         return 0;
      case 2 :
	 /*
	  * Normal case
	  */
	 break;
      default :
	 return ++nargs;
	 break;
   }

   /* 
    * Get the handle 
    */
   handle = f->sp[1].u.number;

   /*
    * Set the current handle
    */
   mdbm_current_handle = handle;

   /* 
    * Check its a valid mdbm 
    */
   if (!(tmp_mdbm = valid_mdbm(handle))) {
      str_del(f->sp++->u.string);
      f->sp->u.number = 0;
      return 0;
   }

   /*
    * Take off some execution ticks.
    */
   i_add_ticks(f,MDBM_TICKS + f->sp->u.string->len);

   /*
    * Allocate and copy string.
    */
   mdbm_key.dsize = f->sp->u.string->len;
   mdbm_key.dptr = ALLOC(char, mdbm_key.dsize);
   memcpy(mdbm_key.dptr, f->sp->u.string->text, f->sp->u.string->len);
   str_del(f->sp++->u.string);
   
#ifdef GDBM_MDBM
   /*
    * Check for existence using the database manager existence routine.
    */
   result = gdbm_exists(tmp_mdbm->dbf, mdbm_key);
#else /* NDBM_MDBM */
   /* 
    * ndbm has no existence routine, so fake it 
    */
   mdbm_value = dbm_fetch(tmp_mdbm->dbf, mdbm_key);
   result = (mdbm_value.dptr != NULL) ? 1 : 0;
#endif

   /*
    * Free the key datum dptr
    */
   FREE(mdbm_key.dptr);

   /*
    * Clear the current handle
    */
   mdbm_current_handle = 0;

   /*
    * The stack pointer will just be the handle, so the result can be put
    * straight into it.
    */
   f->sp->u.number = result;

   /*
    * Successful completion
    */
   return 0;
}
#endif /* FUNCDEF */


#ifdef FUNCDEF
FUNCDEF("mdbm_delete", kf_mdbm_delete, pt_mdbm_delete,0)
#else
char pt_mdbm_delete[] = { C_TYPECHECKED | C_STATIC, 2, 0, 0, 8, T_INT, T_INT ,
                          T_STRING };

/*
 * kf_mdbm_delete(int handle, string key)
 *
 * The kfunction that interfaces with the database manager key/value deletion
 * routine.
 * Checks that we have a valid and known handle and then deletes the key/value 
 * pair from the given database if they exist, using the database manager 
 * deletion routine.
 *
 * handle	- The handle of the database.
 * key		- The "key" of the key/data pair for deletion. 
 *
 * Returns 1 on success, 0 on failure.
 */

int
kf_mdbm_delete(frame *f) {
   int		handle;
   int		result;
   mdbm_t 	*tmp_mdbm;
   datum  	mdbm_key;

   /* 
    * Get the handle 
    */
   handle = f->sp[1].u.number;

   /*
    * Set the current handle
    */
   mdbm_current_handle = handle;

   /* 
    * Check its a valid mdbm 
    */
   if (!(tmp_mdbm = valid_mdbm(handle))) {
      str_del((f->sp++)->u.string);
      f->sp->u.number = 0;
      return 0;
   }

   /*
    * Take off some execution ticks.
    */
   i_add_ticks(f,MDBM_TICKS + f->sp->u.string->len);

   /*
    * Allocate and copy string.
    */
   mdbm_key.dsize = f->sp->u.string->len;
   mdbm_key.dptr = ALLOC(char, mdbm_key.dsize);
   memcpy(mdbm_key.dptr, f->sp->u.string->text, f->sp->u.string->len);
   str_del((f->sp++)->u.string);

   /* 
    * The result will be zero if deletion was successful.
    */
   result = MDBM_DELETE(tmp_mdbm->dbf, mdbm_key);

   /*
    * Free the key datum dptr
    */
   FREE(mdbm_key.dptr);

   /*
    * Clear the current handle
    */
   mdbm_current_handle = 0;

   /*
    * The stack pointer will just be the handle, so the result can be put
    * straight into it.
    */
   f->sp->u.number = (!result) ? 1 : 0;

   /*
    * Successful completion
    */
   return 0;
}
#endif /* FUNCDEF */


#ifdef FUNCDEF
FUNCDEF("mdbm_firstkey", kf_mdbm_firstkey, pt_mdbm_firstkey,0)
#else
char pt_mdbm_firstkey[] = { C_TYPECHECKED | C_STATIC, 1, 0, 0, 7, T_STRING, T_INT };

/*
 * kf_mdbm_firstkey(int handle)
 *
 * The kfunction that interfaces with the database manager firstkey routine
 * that, along with nextkey, can be used to step through the keys (and thus
 * the values) in the database.
 * This function returns the first key in the database.  Note that the order 
 * of the keys in the database is determined by its hashing algorithm, nothing 
 * useful.
 * Checks that we have a valid and known handle and gets the first key in
 * the database using the database manager firstkey routine.
 *
 * handle	- The handle of the database.
 *
 * Returns the first key on success, 0 on failure.
 */

int
kf_mdbm_firstkey(frame *f) {
   int		handle;
   mdbm_t 	*tmp_mdbm;
   datum  	mdbm_key;

   /* 
    * Get the handle 
    */
   handle = f->sp->u.number;

   /*
    * Set the current handle
    */
   mdbm_current_handle = handle;

   /* 
    * Check its a valid mdbm 
    */
   if (!(tmp_mdbm = valid_mdbm(handle))) {
      f->sp->u.number = 0;
      return 0;
   }

   /*
    * Take off some execution ticks.
    */
   i_add_ticks(f,MDBM_TICKS);

   /*
    * Call the database manager firstkey routine.
    */
   mdbm_key = MDBM_FIRSTKEY(tmp_mdbm->dbf);

   /*
    * On failure return.
    */
   if (mdbm_key.dptr == NULL) {
      f->sp->u.number = 0;
      return 0;
   }

   /*
    * Take off some execution ticks.
    */
   i_add_ticks(f,mdbm_key.dsize);

   /*
    * Allocate and copy the string
    */
  f->sp->type = T_STRING;
   str_ref(f->sp->u.string = str_new(mdbm_key.dptr, mdbm_key.dsize));

   /* 
    * Careful freeing mdbm_key.dptr, it has been malloc'ed by gdbm 
    * internally, not by the driver. 
    */
   MDBM_FREE(mdbm_key.dptr);

   /*
    * Clear the current handle
    */
   mdbm_current_handle = 0;

   /*
    * Successful completion.
    */
   return 0;
}
#endif /* FUNCDEF */


#ifdef FUNCDEF
FUNCDEF("mdbm_nextkey", kf_mdbm_nextkey, pt_mdbm_nextkey,0)
#else
char pt_mdbm_nextkey[] = { C_TYPECHECKED | C_STATIC, 2, 0, 0, 8, T_STRING, T_INT ,T_STRING };

/*
 * kf_mdbm_nextkey(int handle, string key)
 *
 * The kfunction that interfaces with the database manager nextkey routine
 * that, along with firstkey, can be used to step through the keys (and thus
 * the values) in the database.
 * This function returns, given a key in the database, the next key in the
 * database.  Note that the order of the keys in the database is determined
 * by its hashing algorithm, nothing useful.
 * Checks that we have a valid and known handle and gets the next key in
 * the database (after key) using the database manager nextkey routine.
 *
 * handle	- The handle of the database.
 * key		- The current "key" if iterating over the database.
 *
 * Returns the next key on success, 0 on failure.
 */

int
kf_mdbm_nextkey(frame *f) {
   int		handle;
   mdbm_t 	*tmp_mdbm;
   datum  	mdbm_key;
   datum 	mdbm_nextkey;

   /* 
    * Get the handle 
    */
   handle = f->sp[1].u.number;

   /*
    * Set the current handle
    */
   mdbm_current_handle = handle;

   /* 
    * Check its a valid mdbm 
    */
   if (!(tmp_mdbm = valid_mdbm(handle))) {
      str_del((f->sp++)->u.string);
      f->sp->u.number = 0;
      return 0;
   }

   /*
    * Take off some execution ticks.
    */
   i_add_ticks(f,MDBM_TICKS + f->sp->u.string->len);

   /*
    * Allocate and copy string.
    */
   mdbm_key.dsize = f->sp->u.string->len;
   mdbm_key.dptr = ALLOC(char, mdbm_key.dsize);
   memcpy(mdbm_key.dptr, f->sp->u.string->text, f->sp->u.string->len);
   str_del((f->sp++)->u.string);

   /*
    * On success, mdbm_value.dtpr will contain the data matching the key,
    * on failure it will be NULL.
    */
   mdbm_nextkey = MDBM_NEXTKEY(tmp_mdbm->dbf, mdbm_key);

   /*
    * On failure, free memory and return undefined.
    */
   if (mdbm_nextkey.dptr == NULL) {
      FREE(mdbm_key.dptr);
      f->sp->u.number = 0;
      return 0;
   }

   /*
    * Take off some execution ticks.
    */
   i_add_ticks(f,mdbm_nextkey.dsize);

   /*
    * Allocate and copy the string
    */
   f->sp->type = T_STRING;
   str_ref(f->sp->u.string = str_new(mdbm_nextkey.dptr, mdbm_nextkey.dsize));

   /* 
    * Careful freeing mdbm_nextkey.dptr, it has been malloc'ed by gdbm 
    * internally, not by the driver. 
    */
   FREE(mdbm_key.dptr);
   MDBM_FREE(mdbm_nextkey.dptr);

   /*
    * Clear the current handle
    */
   mdbm_current_handle = 0;

   /*
    * Successful completion
    */
   return 0;
}
#endif /* FUNCDEF */
#endif /* MDBM_SUPPORT */
