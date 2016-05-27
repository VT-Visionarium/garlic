/* ======================================================================
 *
 * HH   HH         vr_parse.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: October 8, 1998
 * HH   HH         Last Modified: May 5, 2013
 * HH   HH
 *
 * Header file for FreeVR configuration parsing information.  Defines the
 * tokens used to syntactically parse the configuration format, as
 * well as the mapping of tokens to strings for lexically parsing the
 * input into tokens.  Note that the ASCII string representations of those
 * tokens is linked to the enumerated tokens in the file: "vr_config.c".
 *
 * Copyright 2013, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include "vr_objects.h"
#include "vr_config.h"


#define	PARSE_CONCAT_STRINGS	/* define this to concatenate all neighboring strings */


/****************************************************/
/* valid tokens */
/*   Currently only used by Config parsing, but may also     */
/*   be used for interactive socket communication.           */
/* NOTE: currently socket communication uses a much more     */
/*   rudimentary parsing scheme.  Perhaps at some point it   */
/*   will make use of the generalized parsing available here,*/
/*   with it all merged into one big list, but with separate */
/*   mapping arrays.                                         */
/* NOTE: the ASCII representations of these tokens are       */
/*   implemented in vr_config.c in the array: _ConfigTSMap[].*/
typedef enum {
	/*** special token values ***/
	VRTOKEN_PARSEERROR = -6,
	VRTOKEN_EOF = -5,
	VRTOKEN_UNKNOWN = -4,
	VRTOKEN_COMMENT = -3,
	VRTOKEN_NIL = -2,
	VRTOKEN_DEFAULT = -1,

	/*** binary token values ***/
	VRTOKEN_FALSE = 0,
	VRTOKEN_TRUE = 1,
	VRTOKEN_NO = VRTOKEN_FALSE,
	VRTOKEN_YES = VRTOKEN_TRUE,
	VRTOKEN_OFF = VRTOKEN_FALSE,
	VRTOKEN_ON = VRTOKEN_TRUE,

	/*** a string/numeric value ***/
	VRTOKEN_STRING,
	VRTOKEN_NUMBER,
	VRTOKEN_VARIABLE,

	/*** valid punctuation ***/
	VRTOKEN_HASH,
	VRTOKEN_OPENCURL,
	VRTOKEN_CLOSECURL,
	VRTOKEN_COMMA,
	VRTOKEN_SEMICOLON,
	VRTOKEN_QUOTE,

	/*** valid math functions ***/
	VRTOKEN_OPENPAREN,
	VRTOKEN_CLOSEPAREN,

		VRTOKEN_BIOP_BEGIN,	/* beginning of the bi-operand math operator tokens */

	VRTOKEN_PLUS,
	VRTOKEN_TIMES,
	VRTOKEN_DIVIDE,
	VRTOKEN_BIT_AND,
	VRTOKEN_BIT_OR,
	VRTOKEN_LOGICAL_AND,
	VRTOKEN_LOGICAL_OR,

	VRTOKEN_SETEQUALS,
	VRTOKEN_PLUSEQUALS,
	VRTOKEN_TIMESEQUALS,

	VRTOKEN_EQUALITY,
	VRTOKEN_INEQUALITY,
	VRTOKEN_GREATER,
	VRTOKEN_GREATEROREQ,
	VRTOKEN_LESS,
	VRTOKEN_LESSOREQ,

		VRTOKEN_BIOP_END,	/* end of the bi-operand math operator tokens */

	/*** system commands ***/
	VRTOKEN_READCONFIG,
	VRTOKEN_READCONFIGSTR,
	VRTOKEN_NOP,
	VRTOKEN_EXIT,
	VRTOKEN_ABORT,
	VRTOKEN_EXEC,
	VRTOKEN_ECHO,
	VRTOKEN_SETENV,
	VRTOKEN_UNSETENV,
	VRTOKEN_PRINTOBJ,
	VRTOKEN_PRINTCONFIG,
	VRTOKEN_DBXPAUSE,
	VRTOKEN_INCLUDE,	/* actually this may be handled by m4 */
	VRTOKEN_SINCLUDE,	/* actually this may be handled by m4 */

	/*** conditionals ***/
	VRTOKEN_IF,
	VRTOKEN_IFEXEC,
	VRTOKEN_ELSE,

	/*** main options ***/
	VRTOKEN_SET,
	VRTOKEN_SETDEFAULT,
	VRTOKEN_USESYSTEM,
	VRTOKEN_SYSTEM,
	VRTOKEN_PROCESS,
	VRTOKEN_WINDOW,
	VRTOKEN_EYELIST,
	VRTOKEN_USER,
	VRTOKEN_PROP,
	VRTOKEN_INPUTDEV,
	VRTOKEN_INPUTMAP,

	/*** options for multiple objects ***/
	VRTOKEN_NAME,		/* currently not used */
	VRTOKEN_TYPE,
	VRTOKEN_DSOFILE,
	VRTOKEN_DSOFUNC,
	VRTOKEN_ARGS,
	VRTOKEN_MALLEABLE,
	VRTOKEN_DEBUG_LEVEL,
	VRTOKEN_DEBUG_EXACT,
	VRTOKEN_EYES,
	VRTOKEN_INPUT,		/* used as both an inputdevice option and process type */
	VRTOKEN_EXECSTART,
	VRTOKEN_EXECSTOP,
	VRTOKEN_EXECUPONERROR,
	VRTOKEN_EXITUPONERROR,
	VRTOKEN_NEARCLIP,	/* for windows, user, system & default */
	VRTOKEN_FARCLIP,	/* for windows, user, system & default */

	VRTOKEN_COPYOBJECT,

	/*** system options ***/
	/* VRTOKEN_MALLEABLE, */
	/* VRTOKEN_DEBUG_LEVEL, */
	VRTOKEN_SYSTEM_CAVECONFIG,
	VRTOKEN_SYSTEM_PROCS,
	VRTOKEN_SYSTEM_INPUTMAP,
	VRTOKEN_SYSTEM_MASTER,
	VRTOKEN_SYSTEM_SLAVES,

	VRTOKEN_SYSTEM_LOCKCPU,
	VRTOKEN_SYSTEM_LOCKCMD,
	VRTOKEN_SYSTEM_UNLOCKCMD,
	VRTOKEN_SYSTEM_VISCHAN,		/* NOTE: this is a multiple-use option: system, window & globals */
	VRTOKEN_SYSTEM_TRANSFORM,	/* NOTE: also used with globals */
	VRTOKEN_SYSTEM_TRANSLATE,	/* NOTE: also used with globals */
	VRTOKEN_SYSTEM_ROTATE,		/* NOTE: also used with globals */
	/* VRTOKEN_NEARCLIP, */
	/* VRTOKEN_FARCLIP, */
	/* VRTOKEN_EXECSTART, */
	/* VRTOKEN_EXECSTOP, */
	/* VRTOKEN_EXECUPONERROR, */
	/* VRTOKEN_EXITUPONERROR, */
	VRTOKEN_SYSTEM_PRECONTEXT,
	VRTOKEN_SYSTEM_PRECONFIG,
	VRTOKEN_SYSTEM_PREINPUT,
	VRTOKEN_SYSTEM_POSTCONTEXT,
	VRTOKEN_SYSTEM_POSTCONFIG,
	VRTOKEN_SYSTEM_POSTINPUT,

	/*** proc options ***/
	/* VRTOKEN_MALLEABLE, */
	/* VRTOKEN_TYPE, */
	/* VRTOKEN_ARGS, */
	/* VRTOKEN_INPUT, */
	VRTOKEN_PROCESS_MAIN,
	VRTOKEN_PROCESS_COMPUTE,
	VRTOKEN_PROCESS_TELNET,
	VRTOKEN_PROCESS_VISREN,
	VRTOKEN_PROCESS_AUDREN,
	VRTOKEN_PROCESS_HAPREN,
	VRTOKEN_PROCESS_OLFREN,
	VRTOKEN_PROCESS_TASREN,
	VRTOKEN_PROCESS_VESREN,

	/* VRTOKEN_DEBUG_LEVEL, */
	/* VRTOKEN_DEBUG_EXACT, */
	VRTOKEN_PROCESS_MACHINE,
	VRTOKEN_PROCESS_TYPE,
	VRTOKEN_PROCESS_SYNC,
	VRTOKEN_PROCESS_USEC,
	VRTOKEN_PROCESS_COLOR,
	VRTOKEN_PROCESS_STRING,
	VRTOKEN_PROCESS_DBGFILE,
	VRTOKEN_PROCESS_OBJECT,
	VRTOKEN_PROCESS_OBJECTS,
	VRTOKEN_PROCESS_STATSARGS,
	/* VRTOKEN_EXECSTART, */
	/* VRTOKEN_EXECSTOP, */

	/*** window options ***/
	/* VRTOKEN_MALLEABLE, */
	VRTOKEN_WINDOW_MOUNT,
	VRTOKEN_WINDOW_GRAPHICS,
	/* VRTOKEN_ARGS, */
	/* VRTOKEN_EYES, */
	/* VRTOKEN_NEARCLIP, */
	/* VRTOKEN_FARCLIP, */
	VRTOKEN_WINDOW_EYE_COORDS,
	VRTOKEN_WINDOW_COORDS,
	VRTOKEN_WINDOW_TRANSFORM,
	VRTOKEN_WINDOW_TRANSLATE,
	VRTOKEN_WINDOW_ROTATE,
	VRTOKEN_WINDOW_VIEWPORT,
	VRTOKEN_WINDOW_VIEWPORT_LEFT,
	VRTOKEN_WINDOW_VIEWPORT_RIGHT,
	VRTOKEN_WINDOW_FVIEWPORT,
	VRTOKEN_WINDOW_FVIEWPORT_LEFT,
	VRTOKEN_WINDOW_FVIEWPORT_RIGHT,
	VRTOKEN_WINDOW_VIEWMASK,
	VRTOKEN_WINDOW_COLORMASK,

	VRTOKEN_WINDOW_UISHOW,
	VRTOKEN_WINDOW_UILOC,
	VRTOKEN_WINDOW_UICOL,
	VRTOKEN_WINDOW_FPSSHOW,
	VRTOKEN_WINDOW_FPSLOC,
	VRTOKEN_WINDOW_FPSCOL,
	VRTOKEN_WINDOW_STATSSHOW,
	VRTOKEN_WINDOW_STATSPROCS,
	VRTOKEN_WINDOW_FRAMESHOW,
	VRTOKEN_WINDOW_SIMMASK,

	/*** eyelist options ***/
	/* VRTOKEN_MALLEABLE, */
	VRTOKEN_EYELIST_MONOFB,
	VRTOKEN_EYELIST_LEFTFB,
	VRTOKEN_EYELIST_RIGHTFB,
	VRTOKEN_EYELIST_LEFTVP,
	VRTOKEN_EYELIST_RIGHTVP,
	VRTOKEN_EYELIST_ANAGLFB,

	/*** input device options ***/
	/* VRTOKEN_MALLEABLE, */
	/* VRTOKEN_TYPE, */
	/* VRTOKEN_DSOFILE, */
	/* VRTOKEN_DSOFUNC, */
	/* VRTOKEN_ARGS, */
	VRTOKEN_INDEV_FUNCTION,
	VRTOKEN_INDEV_DOF,
	/* VRTOKEN_INPUT, */
	VRTOKEN_CONTROL,
	/* VRTOKEN_EXECSTART, */
	/* VRTOKEN_EXECSTOP, */

	VRTOKEN_INDEV_REFTRANSFORM,
	VRTOKEN_INDEV_REFTRANSLATE,
	VRTOKEN_INDEV_REFROTATE,
	VRTOKEN_INDEV_RECEIVER_TRANSFORM,
	VRTOKEN_INDEV_RECEIVER_TRANSLATE,
	VRTOKEN_INDEV_RECEIVER_ROTATE,

	/*** user options ***/
	/* VRTOKEN_MALLEABLE, */
	VRTOKEN_USER_IOD,
	VRTOKEN_USER_COLOR,
	VRTOKEN_USER_HEADSENSOR,
	VRTOKEN_USER_BODYSENSOR,
	VRTOKEN_USER_REFTRANSFORM,
	VRTOKEN_USER_REFTRANSLATE,
	VRTOKEN_USER_REFROTATE,
	/* VRTOKEN_NEARCLIP, */
	/* VRTOKEN_FARCLIP, */

/* TODO: everytoken from here down needs to be put in the map-to-string array */
	/*** prop options ***/
	/* VRTOKEN_MALLEABLE, */
	VRTOKEN_PROP_HEADSENSOR,
	VRTOKEN_PROP_REFTRANSFORM,
	VRTOKEN_PROP_REFTRANSLATE,
	VRTOKEN_PROP_REFROTATE,

	/*** inputmap options ***/
	/* VRTOKEN_INPUT, */
	/* VRTOKEN_CONTROL, */
	/* the rest of these may go away. */
	VRTOKEN_INMAP_SWITCH2,
	VRTOKEN_INMAP_SWITCHN,
	VRTOKEN_INMAP_VALUATOR,
	VRTOKEN_INMAP_POSITION,
	VRTOKEN_INMAP_POINTER,
	VRTOKEN_INMAP_PLANE,
	VRTOKEN_INMAP_KEYBOARD,
	VRTOKEN_INMAP_TEXT

} vrToken;


/****************************************************/
/* this is used both for mapping strings to tokens, */
/*   and for returning a parsed token/string pair.  */
typedef struct vrTokenInfo_st {
		vrToken		token;		/* the token enumeration */
		char		*string;	/* the associated string */
		int		linenum;	/* the line the token was on */
	struct	vrTokenInfo_st	*next;		/* for linked lists -- currently used for pre-parsed list */
	} vrTokenInfo;


/*******************************************************/
/* The vrFunctionInfo structure is used to map strings */
/*   to functions.                                     */
/* TODO: this might work better as a linked list. */
#if 0
typedef vrTokenInfo (*vrFunctionEval)(char *);
#endif
typedef struct vrFunctionInfo_st {
		char		*(*func)(char *);	/* the function to call */
		char		*string;		/* the associated string that names the function */
	} vrFunctionInfo;


/****************************************************/
#define MAX_PARSE_INPUT 8192
typedef struct {
		int		linenum;		/* the line "input" is from */
		char		*parse_name;		/* what is being parsed (eg. "Config") */
		char		*filename;		/* the file being read from */
		FILE		*fp;			/* the FILE data of filename */
		char		input[MAX_PARSE_INPUT];	/* the line currently parsing */
		char		*next_input;		/* next char in input to parse */
		int		nummaps;		/* the length of the tsmap_list */
		vrTokenInfo	*tsmap_list;		/* array to map strings to tokens */
		int		numfuncs;		/* the length of the function array */
		vrFunctionInfo	*funcmap_list;		/* array to map strings to functions */
		vrTokenInfo	*preparsed;		/* a list of already parsed tokens */
		int		suppress_inline;	/* a flag to suppress inline eval of a */
							/*   token (eg. string concat, math simp) */
	} vrParseInfo;


/****************************************************/

/*** function-as-argument declarations ***/
#if 1
typedef int (*vrParseEnumeratorString)(char *);
#else
typedef void (*vrParseEnumeratorString)(char *);
#endif

/*** function declarations ***/
vrTokenInfo	vrParseNextToken(vrParseInfo *parse);
vrTokenInfo	vrParseJustNextToken(vrParseInfo *parse);
vrTokenInfo	vrParseNextTokenPrint(vrParseInfo *parse);
void		vrParseReturnLastToken(vrTokenInfo token, vrParseInfo *parse);
vrTokenInfo	vrParseMathExpression(vrParseInfo *parse);

vrTokenInfo	vrParseSingleStringExpr(char **retvalue, char *setting, vrParseInfo *parse);
vrTokenInfo	vrParseStringListExpr(char ***array, int *array_len, char *setting, vrParseInfo *parse);
vrTokenInfo	vrParseSingleEnumerator(int *retvalue, vrParseEnumeratorString stringparser, char *setting, vrParseInfo *parse);
vrTokenInfo	vrParseSingleIntegerExpr(int *retvalue, char *setting, vrParseInfo *parse);
vrTokenInfo	vrParseIntegerListExpr(int *array, int maxlen, char *setting, vrParseInfo *parse);
vrTokenInfo	vrParseSingleFloatExpr(float *retvalue, char *setting, vrParseInfo *parse);
vrTokenInfo	vrParseFloatListExpr(float *array, int maxlen, char *setting, vrParseInfo *parse);
vrTokenInfo	vrParseFloatListExprAssign(float *array, int maxlen, vrTokenInfo *assignment_tok, char *setting, vrParseInfo *parse);
vrTokenInfo	vrParseMatrixFromCoords(vrMatrix **xform, double *coords_ll, double *coords_lr, double *coords_ul, char *setting, vrParseInfo *parse);
vrTokenInfo	vrParseMatrixTransform(vrMatrix **xform, char *setting, vrParseInfo *parse);
vrTokenInfo	vrParseMatrixTranslate(vrMatrix **xform, char *setting, vrParseInfo *parse);
vrTokenInfo	vrParseMatrixRotate(vrMatrix **xform, char *setting, vrParseInfo *parse);

vrTokenInfo	vrParseToEOS(vrParseInfo *parse);

/***  ***/
vrInputDesc	*vrParseInputDescription(char *description);
vrInputDTI	*vrParseInputDTI(char *string);

/*** argument function declarations ***/
int		vrArgParseString(char *args, char *argname, char **arglvalue);
int		vrArgParseFloat(char *args, char *argname, float *arglvalue);
int		vrArgParseFloatList(char *args, char *argname, float *arglvalue, int maxlen);
int		vrArgParseInteger(char *args, char *argname, int *arglvalue);
int		vrArgParseBool(char *args, char *argname, int *arglvalue);
int		vrArgParseChoiceInteger(char *args, char *argname, int *arglvalue, char *choices[], int values[]);

