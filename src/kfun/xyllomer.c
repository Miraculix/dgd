/*
  Xyllomer Special KFuns
 */

# ifndef FUNCDEF
# include "kfun.h"
# include "path.h"
# include "comm.h"
# include "call_out.h"
# include "editor.h"
# include "node.h"
# include "control.h"
# include "compile.h"

# endif

/* Extras from Xyllomer */
# ifdef FUNCDEF
FUNCDEF("inherit_list", kf_inherit_list, p_inherit_list,0)
# else
char p_inherit_list[] = { C_TYPECHECKED | C_STATIC, 1,0,0,7,T_STRING, T_OBJECT};

/*
 * NAME:        kfun->inherit_list()
 * DESCRIPTION: returns an array of objects inherited. [Array of Strings].
 */
int kf_inherit_list(register frame *f)
{
    object *obj;                /* Here we store the Object which we got as
                                   command line Option */
    array *arr;                 /* This is the resulting Array, we will
                                   return */

    char buffer[STRINGSZ+12], *p;           /* Temp Var */
    register int size;          /* Temp Var */
    register unsigned int i;    /* Temp Var */
    register value *v;          /* Temp Var */

    obj=&otable[f->sp->oindex];
    size = o_control(obj)->ninherits;                /* How much other
                                                   objects are inherited? */


        /*
         * Now we Allocate the Array -2 for, Hm, 0 was the Auto-Object,
         * and, umm. The Last was the Object himself, i think. ;-)
         */

    arr = arr_new(f->data, (long) size-2);               /* Init the Array */
    v = arr->elts;

        /*
         * Get the Names of the Objects, which are inherited, and add
         * them to the Array. Pointer Dingens, etc..
         */

    for(i = 1; i < (size-1); i++, v++) {
        v->type = T_STRING;
        p = o_name(buffer, OBJR(obj->ctrl->inherits[i].oindex));
        str_ref(v->u.string = str_new((char *) NULL, strlen(p) + 1L));
        v->u.string->text[0] = '/';
        strcpy(v->u.string->text + 1, p);
      }

    f->sp->type = T_ARRAY;                 /* Set the Type we return */
    arr_ref(f->sp->u.array = arr);         /* Assign the Array for return */

    return 0;
}
# endif

# ifdef FUNCDEF
FUNCDEF("function_list", kf_function_list, p_function_list,0)
# else
char p_function_list[] = { C_TYPECHECKED | C_STATIC, 1,0,0,7,T_STRING, T_OBJECT};

/*
 * NAME:        kfun->function_list()
 * DESCRIPTION: returns the list of functions of an object
 */
int kf_function_list(register frame *f)
{
    object *obj;
    control *ctrl;
    array *arr;
    register char *p;
    register int size;
    register unsigned int i;
    register value *v;

    obj=&otable[f->sp->oindex];

    ctrl = o_control(obj);                       /* Assign ctrl */
    d_get_funcdefs(ctrl);                       /* Fill ctrl */
    size = ctrl->nfuncdefs;                     /* How much funcdefs? */

    arr = arr_new(f->data, (long) size);                 /* Init the Array */
    v = arr->elts;

        /* Get the Functions Names and sum up in the Array */

    for(i = 0; i < size; i++, v++)
      {
        v->type = T_STRING;
        p = d_get_strconst(ctrl, ctrl->funcdefs[i].inherit,
                           ctrl->funcdefs[i].index)->text;
        str_ref(v->u.string = str_new(p, (long) strlen(p)));
      }

    f->sp->type = T_ARRAY;                 /* Set Type for Return */
    arr_ref(f->sp->u.array = arr);         /* Assign the Array, we return */

    return 0;
}
# endif

# ifdef FUNCDEF
FUNCDEF("object_list", kf_object_list, pt_object_list,0)
# else
char pt_object_list[] = { C_TYPECHECKED | C_STATIC, 0,0,0,6,T_INT};

/*
 * NAME:        kfun->object_list()
 * DESCRIPTION: dumps a master object list to stderr.
 *              returns number of masterobjects
 */
int kf_object_list(register frame *f)
{
  register uindex i,masters=0,clones=0;
  register object *o,*top=(object *)NULL;
  char buf[26];
  FILE *fp;

  fp=fopen("ObjectList","w");
  if (!fp)
        error("Cannot open dumpfile /ObjectList!");

  fprintf(fp,"OBJECT-MASTER TABLE DUMP generated at %s\n\n",P_ctime(buf,P_time()));
  for (i = o_nobjects(), o = otable; i > 0; --i, o++) {
    if ( (o) && (o->chain.name != (char *) NULL)) {
      if (!masters++)
        top=o;
      fprintf(fp,"/%-60s %4d ",o->chain.name,o->cref);
      clones+=o->cref;
      if (o->cref > top->cref)
        top=o;
      if (o_control(o) != (control *) NULL)
      {
      	control *ctrl = (control *)o_control(o);
        fprintf(fp,"%s %4d %4d\n",P_ctime(buf, ctrl->compiled),ctrl->progsize,ctrl->nvariables);
      }
      else
        fprintf(fp,"\n");
    }
  }
  fprintf(fp,"\nTOTALS   : %d Masterobjects with %d clones.\n",masters,clones);
  fprintf(fp,"TOPOBJECT: /%s with %d clones.\n",top->chain.name,top->cref);
  fprintf(fp,"---------------------------------------------------------------------------\n");
  fclose(fp);
  (--f->sp)->type = T_INT;
  f->sp->u.number = masters;
  return 0;
}
# endif

# ifdef FUNCDEF
FUNCDEF("object_list2", kf_object_list2, pt_object_list2,0)
# else
char pt_object_list2[] = { C_TYPECHECKED | C_STATIC, 0,0,0,6,T_INT};

/*
 * NAME:        kfun->object_list()
 * DESCRIPTION: dumps a master object list to stderr.
 *              returns number of masterobjects
 */
int kf_object_list2(register frame *f)
{
  register uindex i,masters=0,clones=0;
  register object *o,*top=(object *)NULL;
  char buf[26];
  FILE *fp;

  fp=fopen("ObjectList.csv","w");
  if (!fp)
        error("Cannot open dumpfile /ObjectList.csv!");

  fprintf(fp,"\"NumClones\",\"ProgSize\",\"Variables\",\"Sectors\",\"File\",\"Compiletime\"\n",P_ctime(buf,P_time()));
  for (i = o_nobjects(), o = otable; i > 0; --i, o++) {
    if ( (o) && (o->chain.name != (char *) NULL)) {
      if (o_control(o) != (control *) NULL)
      {
      	control *ctrl = (control *)o_control(o);
        fprintf(fp,"%d,%d,%d,%d,\"/%s\",\"%s\"\n",o->cref,ctrl->progsize,ctrl->nvariables,ctrl->nsectors,o->chain.name,P_ctime(buf, ctrl->compiled));
      }
    }
  }
  fclose(fp);
  (--f->sp)->type = T_INT;
  f->sp->u.number = masters;
  return 0;
}
# endif

/* End of Xyllomer Special */
