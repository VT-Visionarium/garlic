/* ======================================================================
 *
 *  CCCCC          vr_config.c
 * CC   CC         Author(s): Ed Peters, Bill Sherman
 * CC              Created: June 4, 1998
 * CC   CC         Last Modified: December 2, 2013
 *  CCCCC
 *
 * Code file for FreeVR configuration and context information.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <string.h>
#include <stdlib.h>		/* used by getenv() */
#include <signal.h>

#include "vr_input.h"
#include "vr_config.h"
#include "vr_entity.h"
#include "vr_objects.h"
#include "vr_parse.h"
#include "vr_system.h"
#include "vr_debug.h"


/** define this to see every token as it's parsed (also requires PARSE_DBGLVL) **/
#if 1
#  define vrParseNextToken vrParseNextTokenPrint
#endif


/*****************************************/
/*** function-as-argument declarations ***/
typedef vrTokenInfo (*_ConfigParseOptionList)(vrObjectInfo *, vrTokenInfo, vrParseInfo *, vrConfigInfo *, void *);

/************************************************************************/
/* function declarations of local-only functions called before defined. */
static vrTokenInfo	_ConfigParseGlobalOption(vrTokenInfo token, vrParseInfo *parse_data, vrConfigInfo *config, vrObjectInfo *object, _ConfigParseOptionList objparser);
static vrTokenInfo	_ConfigParseStatement(vrConfigInfo *config, vrParseInfo *config_parse, vrObjectInfo *object, _ConfigParseOptionList objparser);
static int		_ConfigReadFile(vrConfigInfo *config, char *path, char *file);
static int		_ConfigReadString(vrConfigInfo *config, char *info_string, char *conf_string);
static void		_ConfigReadStringList(vrConfigInfo *config, int *argc, char **argv);
static void		_ConfigReadEnvVariable(vrConfigInfo *config, char *name);


/*******************************************************************/
/* _ConfigTSMap[]: array that maps vrTokens to their corresponding */
/*   string/ASCII representations.                                 */
static vrTokenInfo _ConfigTSMap[] = {
	/* angle brackets are parsed as math punctuation, so these ten are for output only */
		{ VRTOKEN_PARSEERROR,	"<parse error>" },
		{ VRTOKEN_EOF,		"<eof>" },
		{ VRTOKEN_UNKNOWN,	"<unk>" },
		{ VRTOKEN_NIL,		"<nil>" },
		{ VRTOKEN_DEFAULT,	"<default>" },
		{ VRTOKEN_FALSE,	"<false/no>" },
		{ VRTOKEN_TRUE,		"<true/yes>" },
		{ VRTOKEN_STRING,	"<string>" },
		{ VRTOKEN_NUMBER,	"<number>" },
		{ VRTOKEN_VARIABLE,	"<variable>" },

	/* normal punctuation */
		{ VRTOKEN_HASH,		"#" },
		{ VRTOKEN_OPENCURL,	"{" },
		{ VRTOKEN_CLOSECURL,	"}" },
		{ VRTOKEN_COMMA,	"," },
		{ VRTOKEN_SEMICOLON,	";" },
		{ VRTOKEN_QUOTE,	"\"" },

	/* math punctuation */
		{ VRTOKEN_OPENPAREN,	"(" },		/* NOTE: like normal punctuation, the open   */
		{ VRTOKEN_CLOSEPAREN,	")" },		/*   and close parens are single char tokens */

		{ VRTOKEN_PLUS,		"+" },
		{ VRTOKEN_TIMES,	"*" },
		{ VRTOKEN_DIVIDE,	"/" },
		{ VRTOKEN_BIT_AND,	"&" },
		{ VRTOKEN_BIT_OR,	"|" },
		{ VRTOKEN_LOGICAL_AND,	"&&" },
		{ VRTOKEN_LOGICAL_OR,	"||" },

		{ VRTOKEN_SETEQUALS,	"=" },
		{ VRTOKEN_PLUSEQUALS,	"+=" },
		{ VRTOKEN_TIMESEQUALS,	"*=" },

		{ VRTOKEN_EQUALITY,	"==" },
		{ VRTOKEN_INEQUALITY,	"!=" },
		{ VRTOKEN_GREATER,	">" },
		{ VRTOKEN_GREATEROREQ,	">=" },
		{ VRTOKEN_LESS,		"<" },
		{ VRTOKEN_LESSOREQ,	"<=" },
		/* NOTE: it might be possible to have a MINUS operand that coexists with    */
		/*   the "-" as part of the number -- if we're expecting a math expression, */
		/*   then the "-" can be assumed to be that, or if we're expecting a number */
		/*   then it's a number -- hmmm, maybe the code isn't quite formulated that */
		/*   way.                                                                   */

	/* keywords (the rest) */

	   /** system commands **/
		{ VRTOKEN_READCONFIG,	"readconfig" },
		{ VRTOKEN_READCONFIGSTR,"readconfigstring" },
		{ VRTOKEN_NOP,		"nop" },
		{ VRTOKEN_EXIT,		"exit" },
		{ VRTOKEN_ABORT,	"abort" },
		{ VRTOKEN_EXEC,		"exec" },
		{ VRTOKEN_ECHO,		"echo" },
		{ VRTOKEN_SETENV,	"setenv" },
		{ VRTOKEN_UNSETENV,	"unsetenv" },
		{ VRTOKEN_PRINTOBJ,	"printobject" },
		{ VRTOKEN_PRINTCONFIG,	"printconfig" },
		{ VRTOKEN_DBXPAUSE,	"dbxpause" },
		/* "include" if not handled by m4 */
		/* "sinclude" if not handled by m4 */

	   /** conditionals **/
		{ VRTOKEN_IF,		"if" },
		{ VRTOKEN_IFEXEC,	"ifexec" },
		{ VRTOKEN_ELSE,		"else" },

	   /** global options **/
		{ VRTOKEN_SET,		"set" },
		{ VRTOKEN_SETDEFAULT,	"setdefault" },
		{ VRTOKEN_USESYSTEM,	"usesystem" },

	   /** object specifiers **/
		{ VRTOKEN_SYSTEM,	"system" },
		{ VRTOKEN_PROCESS,	"process" },
		{ VRTOKEN_WINDOW,	"screen" },		/* deprecated */
		{ VRTOKEN_WINDOW,	"window" },
		{ VRTOKEN_EYELIST,	"eyelist" },
		{ VRTOKEN_USER,		"user" },
		{ VRTOKEN_PROP,		"prop" },
		{ VRTOKEN_INPUTDEV,	"inputdevice" },
		{ VRTOKEN_INPUTDEV,	"indev" },
		{ VRTOKEN_INPUTMAP,	"inputmap" },

	   /** object option specifiers **/
		{ VRTOKEN_NAME,		"name" },
		{ VRTOKEN_TYPE,		"type" },
		{ VRTOKEN_DSOFILE,	"dsofile" },
		{ VRTOKEN_DSOFUNC,	"dsofunc" },
		{ VRTOKEN_ARGS,		"args" },
		{ VRTOKEN_MALLEABLE,	"malleable" },
		{ VRTOKEN_MALLEABLE,	"changeable" },
		{ VRTOKEN_DEBUG_LEVEL,	"debuglevel" },
		{ VRTOKEN_DEBUG_EXACT,	"debugexact" },
		{ VRTOKEN_DEBUG_EXACT,	"debugthistoo" },
		{ VRTOKEN_INPUT,	"input" },		/* doubles as an inputdevice option and a process type */
		{ VRTOKEN_EXECSTART,	"execatstart" },
		{ VRTOKEN_EXECSTOP,	"execatstop" },
		{ VRTOKEN_EXECUPONERROR,"execuponerror" },
		{ VRTOKEN_EXITUPONERROR,"exituponerror" },
		{ VRTOKEN_NEARCLIP,	"near_clip" },
		{ VRTOKEN_NEARCLIP,	"nearclip" },
		{ VRTOKEN_NEARCLIP,	"near" },
		{ VRTOKEN_FARCLIP,	"far_clip" },
		{ VRTOKEN_FARCLIP,	"farclip" },
		{ VRTOKEN_FARCLIP,	"far" },

		{ VRTOKEN_COPYOBJECT,	"copyof" },

	   /** system options **/
		{ VRTOKEN_SYSTEM_CAVECONFIG,	"caveconfig" },
		{ VRTOKEN_SYSTEM_PROCS,		"procs" },
		{ VRTOKEN_SYSTEM_INPUTMAP,	"inputmap" },

		{ VRTOKEN_SYSTEM_MASTER,	"master" },
		{ VRTOKEN_SYSTEM_SLAVES,	"slaves" },

		{ VRTOKEN_SYSTEM_LOCKCPU,	"LockCPU" },
		{ VRTOKEN_SYSTEM_LOCKCMD,	"LockCmd" },
		{ VRTOKEN_SYSTEM_LOCKCMD,	"LockCommand" },
		{ VRTOKEN_SYSTEM_UNLOCKCMD,	"UnlockCmd" },
		{ VRTOKEN_SYSTEM_UNLOCKCMD,	"UnlockCommand" },
		{ VRTOKEN_SYSTEM_VISCHAN,	"VisrenMode" },
		{ VRTOKEN_SYSTEM_TRANSFORM,	"WorldTransform" },
		{ VRTOKEN_SYSTEM_TRANSLATE,	"WorldTranslate" },
		{ VRTOKEN_SYSTEM_ROTATE,	"WorldRotate" },
		{ VRTOKEN_SYSTEM_PRECONTEXT,	"PreContext" },
		{ VRTOKEN_SYSTEM_PRECONTEXT,	"PreContextPrint" },
		{ VRTOKEN_SYSTEM_PRECONFIG,	"PreConfig" },
		{ VRTOKEN_SYSTEM_PRECONFIG,	"PreConfigPrint" },
		{ VRTOKEN_SYSTEM_PREINPUT,	"PreInput" },
		{ VRTOKEN_SYSTEM_PREINPUT,	"PreInputPrint" },
		{ VRTOKEN_SYSTEM_POSTCONTEXT,	"PostContext" },
		{ VRTOKEN_SYSTEM_POSTCONTEXT,	"PostContextPrint" },
		{ VRTOKEN_SYSTEM_POSTCONFIG,	"PostConfig" },
		{ VRTOKEN_SYSTEM_POSTCONFIG,	"PostConfigPrint" },
		{ VRTOKEN_SYSTEM_POSTINPUT,	"PostInput" },
		{ VRTOKEN_SYSTEM_POSTINPUT,	"PostInputPrint" },

	   /** process options **/
		{ VRTOKEN_PROCESS_MAIN,		"main" },
		{ VRTOKEN_PROCESS_COMPUTE,	"compute" },
		{ VRTOKEN_PROCESS_TELNET,	"telnet" },
		{ VRTOKEN_PROCESS_VISREN,	"visren" },
		{ VRTOKEN_PROCESS_AUDREN,	"audren" },
		{ VRTOKEN_PROCESS_HAPREN,	"hapren" },
		{ VRTOKEN_PROCESS_OLFREN,	"olfren" },
		{ VRTOKEN_PROCESS_TASREN,	"tasren" },
		{ VRTOKEN_PROCESS_VESREN,	"vesren" },

		{ VRTOKEN_PROCESS_MACHINE,	"machine" },
		{ VRTOKEN_PROCESS_SYNC,		"sync" },
		{ VRTOKEN_PROCESS_USEC,		"usecmin" },
		{ VRTOKEN_PROCESS_COLOR,	"printcolor" },
		{ VRTOKEN_PROCESS_STRING,	"printstring" },
		{ VRTOKEN_PROCESS_DBGFILE,	"printfile" },
		{ VRTOKEN_PROCESS_OBJECT,	"object" },
		{ VRTOKEN_PROCESS_OBJECTS,	"objects" },
		{ VRTOKEN_PROCESS_STATSARGS,	"stats" },

	   /** window options **/
		{ VRTOKEN_WINDOW_MOUNT,		"mount" },
		{ VRTOKEN_WINDOW_MOUNT,		"mounttype" },
		{ VRTOKEN_WINDOW_GRAPHICS,	"graphics" },
		{ VRTOKEN_WINDOW_GRAPHICS,	"graphicstype" },
		{ VRTOKEN_WINDOW_EYE_COORDS,	"e2w_coords" },
		{ VRTOKEN_WINDOW_COORDS,	"rw2s_coords" },	/* deprecated */
		{ VRTOKEN_WINDOW_TRANSFORM,	"rw2s_transform" },	/* deprecated */
		{ VRTOKEN_WINDOW_TRANSLATE,	"rw2s_translate" },	/* deprecated */
		{ VRTOKEN_WINDOW_ROTATE,	"rw2s_rotate" },	/* deprecated */
		{ VRTOKEN_WINDOW_COORDS,	"rw2w_coords" },
		{ VRTOKEN_WINDOW_TRANSFORM,	"rw2w_transform" },
		{ VRTOKEN_WINDOW_TRANSLATE,	"rw2w_translate" },
		{ VRTOKEN_WINDOW_ROTATE,	"rw2w_rotate" },
		{ VRTOKEN_WINDOW_COORDS,	"coords" },
		{ VRTOKEN_WINDOW_VIEWPORT,	"viewport" },
		{ VRTOKEN_WINDOW_VIEWPORT,	"viewport:both" },
		{ VRTOKEN_WINDOW_VIEWPORT_LEFT,	"viewport:left" },
		{ VRTOKEN_WINDOW_VIEWPORT_RIGHT,"viewport:right" },
		{ VRTOKEN_WINDOW_FVIEWPORT,	"viewportf" },
		{ VRTOKEN_WINDOW_FVIEWPORT,	"viewportf:both" },
		{ VRTOKEN_WINDOW_FVIEWPORT_LEFT,"viewportf:left" },
		{ VRTOKEN_WINDOW_FVIEWPORT_RIGHT,"viewportf:right" },
		{ VRTOKEN_WINDOW_VIEWMASK,	"viewmask" },
		{ VRTOKEN_WINDOW_COLORMASK,	"colormask" },
		{ VRTOKEN_WINDOW_UISHOW,	"showui" },
		{ VRTOKEN_WINDOW_UILOC,		"placeui" },
		{ VRTOKEN_WINDOW_UICOL,		"colorui" },
		{ VRTOKEN_WINDOW_FPSSHOW,	"showfps" },
		{ VRTOKEN_WINDOW_FPSLOC,	"placefps" },
		{ VRTOKEN_WINDOW_FPSCOL,	"colorfps" },
		{ VRTOKEN_WINDOW_STATSSHOW,	"showstats" },
		{ VRTOKEN_WINDOW_STATSPROCS,	"statsprocs" },
		{ VRTOKEN_WINDOW_FRAMESHOW,	"showframe" },
		{ VRTOKEN_WINDOW_SIMMASK,	"simmask" },

	   /** eyelist options **/
		{ VRTOKEN_EYELIST_MONOFB,	"monofb" },		/* or "single" */
		{ VRTOKEN_EYELIST_LEFTFB,	"leftfb" },		/* or "dual_left" */
		{ VRTOKEN_EYELIST_RIGHTFB,	"rightfb" },		/* or "dual_right" */
		{ VRTOKEN_EYELIST_LEFTVP,	"leftvp" },		/* or "view port left" */
		{ VRTOKEN_EYELIST_RIGHTVP,	"rightvp" },		/* or "view port right" */
		{ VRTOKEN_EYELIST_ANAGLFB,	"anaglfb" },		/* or "anaglyph" */

	   /** user options **/
		{ VRTOKEN_USER_IOD,		"iod" },
		{ VRTOKEN_USER_COLOR,		"color" },
		{ VRTOKEN_USER_HEADSENSOR,	"headsensor" },
		{ VRTOKEN_USER_BODYSENSOR,	"bodysensor" },
		{ VRTOKEN_USER_REFTRANSFORM,	"r2b_transform" },	/* perhaps to be deprecated */
		{ VRTOKEN_USER_REFTRANSLATE,	"r2b_translate" },	/* perhaps to be deprecated */
		{ VRTOKEN_USER_REFROTATE,	"r2b_rotate" },		/* perhaps to be deprecated */

	   /** input (and output) device options **/
		{ VRTOKEN_INDEV_FUNCTION,	"function" },
		{ VRTOKEN_INDEV_DOF,		"dof" },
		{ VRTOKEN_CONTROL,		"control" },
		{ VRTOKEN_INDEV_REFTRANSFORM,	"t2rw_transform" },
		{ VRTOKEN_INDEV_REFTRANSLATE,	"t2rw_translate" },
		{ VRTOKEN_INDEV_REFROTATE,	"t2rw_rotate" },
		{ VRTOKEN_INDEV_REFTRANSFORM,	"for_transform" },	/* frame-of-reference -- same as t2rw, though */
		{ VRTOKEN_INDEV_REFTRANSLATE,	"for_translate" },	/*   more meaningful in the case of shifting  */
		{ VRTOKEN_INDEV_REFROTATE,	"for_rotate" },		/*   the coordinate-system for output devices.*/
		/* TODO: I'm not so sure we want the following three -- probably */
		/*   should be handled by user and prop stuff.                   */
		/* 11/10/99 or maybe this IS where this relationship should be declared. */
		{ VRTOKEN_INDEV_RECEIVER_TRANSFORM,	"r2e_transform" },
		{ VRTOKEN_INDEV_RECEIVER_TRANSLATE,	"r2e_translate" },
		{ VRTOKEN_INDEV_RECEIVER_ROTATE,	"r2e_rotate" },
};


	/**************************************************/
	/*** Configuration function evaluator functions ***/
	/**************************************************/

/**************************************************************/
char *vrConfigFunctionExecPipe(char *args)
{
	FILE	*pipe;
	char	buffer[1024];
	char	*result = NULL;
	char	*tofree;		/* pointer to memory that can be freed */

	/* make a pipe input from the command */
	pipe = popen(args, "r");

	if (pipe == NULL) {
		/* TODO: error message & return the empty string */
	}

	while (fgets(buffer, sizeof(buffer)-1, pipe)) {
		if (result == NULL) {
			result = vrShmemStrDup(buffer);
		} else {
			tofree = result;
			result = vrShmemStrCat(result, buffer);
			vrShmemFree(tofree);
		}
	}

	pclose(pipe);

#if 0
vrPrintf(RED_TEXT "Returning from function 'ExecPipe' with value of '%s'\n" NORM_TEXT, args);
#endif
	return (result);
}


/**************************************************************/
char *vrConfigFunctionExecStatus(char *args)
{
	char	result[128];

	sprintf(result, "%d", system(args));

#if 0
vrPrintf(RED_TEXT "Returning from function 'ExecStatus' with value of '%s'\n" NORM_TEXT, args);
#endif
	return (vrShmemStrDup(result));
}


/**************************************************************/
char *vrConfigFunctionIsInputDevice(char *args)
{
	if (vrInputDeviceInList(args))
		return (vrShmemStrDup("1"));
	else	return (vrShmemStrDup("0"));
}


/**************************************************************/
char *vrConfigFunctionIsVisrenDevice(char *args)
{
	if (vrVisrenDeviceInList(args))
		return (vrShmemStrDup("1"));
	else	return (vrShmemStrDup("0"));
}


/**************************************************************/
/* _ConfigFuncMap[]: array that maps vrFunctionInfos to their */
/*   corresponding string representations.                    */
static vrFunctionInfo _ConfigFuncMap[] = {
		{ vrConfigFunctionExecStatus,	"execstatus" },
		{ vrConfigFunctionExecPipe,	"execpipe" },
		{ vrConfigFunctionIsInputDevice,"isinputdevice" },
		{ vrConfigFunctionIsVisrenDevice,"isvisrendevice" },
};



	/*************************************************************/
	/*** Algorithmic syntax parsing of the FreeVR config file. ***/
	/*************************************************************/


/*****************/
/*** UseSystem ***/
/********************************************************************/
/*  Format is: 'USESYSTEM' { <string> | 'caveconfig' <filename> } ; */
static void _ConfigParseUsesystem(vrConfigInfo *config, vrParseInfo *parse)
{
	vrTokenInfo	token;

	/** next token is name of system to use **/
	token = vrParseNextToken(parse);

	if (token.token == VRTOKEN_SEMICOLON) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "'UseSystem' option prematurely terminated.\n" NORM_TEXT,
			token.linenum);
	} else if (token.token == VRTOKEN_SYSTEM_CAVECONFIG) {
		vrDbgPrintf("TODO: CAVEconfig system option not yet implemented.\n");
		/** next token is name of caveconfig file to read **/
		token = vrParseNextToken(parse);
		if (token.token == VRTOKEN_SEMICOLON) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "'UseSystem' option prematurely terminated.\n" NORM_TEXT,
				token.linenum);
		} else if (token.token != VRTOKEN_STRING) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "config filename should follow the 'UseSystem caveconfig' option.\n" NORM_TEXT,
				token.linenum);
		}
		return;

	} else if (token.token != VRTOKEN_STRING) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "string should follow the 'UseSystem' option.\n" NORM_TEXT,
			token.linenum);
	}
	config->system_name = strdup(token.string);
	vrDbgPrintfN(AALWAYS_DBGLVL, "FreeVR config: " BOLD_TEXT "Using system '%s'\n" NORM_TEXT, config->system_name);

	/** next token should be a statement terminator (ie. ';') **/
	token = vrParseNextToken(parse);
	if (token.token != VRTOKEN_SEMICOLON) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "'UseSystem' option not terminated with a ';'.\n" NORM_TEXT,
			token.linenum);
	}
}


/**************/
/*** System ***/
/**************/

/*  Format is: { procs { "=" | "+=" <procnames>* [;] | inputmap "=" <mapname> [";"] } */
/* _ConfigParseSystemOption(): recieves the first token to parse, which     */
/*   should be one of the system options (ie. "procs" or "inputmap",        */
/*   continue to read new tokens until hitting a semicolon, or a closecurl.)*/
/*   Return the last token read.                                            */
static vrTokenInfo _ConfigParseSystemOption(vrObjectInfo *object, vrTokenInfo token, vrParseInfo *parse, vrConfigInfo *config, _ConfigParseOptionList objparser)
{
	vrSystemInfo	*system;
	vrObject	*src_object;			/* used for copying an existing object */
	vrTokenInfo	orig_token = token;		/* store the incoming token for debug info */

	vrTrace("_ConfigParseSystemOption()", RED_TEXT "entering" NORM_TEXT);

	system = (vrSystemInfo *)object;

	switch (token.token) {

					/***************/
	case VRTOKEN_SEMICOLON:		/* Format: ";" */
					/***************/
		/* extra semicolons are never a problem */
		token = vrParseNextToken(parse);
		break;

					/***************/		/* need matching "{" */
	case VRTOKEN_CLOSECURL:		/* Format: "}" */
					/***************/
		/* if we hit the end of the list, then we're done */
		/*   (if this happens here, then it's probably an empty list) */
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read '}' in system option -- empty list?\n", token.linenum);
		break;

					/****************************************************/
	case VRTOKEN_MALLEABLE:		/* Format: "Malleable" assignment-expr number [";"] */
					/****************************************************/
		token = vrParseSingleIntegerExpr(&(system->malleable), "System Malleable", parse);
		break;

					/************************************/
	case VRTOKEN_COPYOBJECT:	/* Format: "copyof" object-name ";" */
					/************************************/
		/*** read the name of an object for copying ***/
		token = vrParseNextToken(parse);
		src_object = vrObjectSearch(config->context, object->object_type, token.string);
		if (src_object == NULL) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Can't find System object \"%s\" for copying.\n" NORM_TEXT,
				token.linenum, token.string);
		} else {
			vrObjectCopy(config->context, object, src_object);
			if (vrDbgDo(CONFIG_DBGLVL)) {
				vrFprintObjectInfo(stdout, object, verbose);
			}
		}
		/* a copyof operation should be terminated by a semicolon. */
		token = vrParseNextToken(parse);
		if (token.token != VRTOKEN_SEMICOLON) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Improper termination to the object copy option.\n" NORM_TEXT,
				token.linenum);
		}
		break;


	/*************************************/
	/* these fields are only for systems */

					/**********************************************************/
	case VRTOKEN_TYPE:		/* Format: "Type" assignment-expr { "sim" | "cave" | ... }*/
					/**********************************************************/
		token = vrParseSingleEnumerator((int *)(&(system->type)), vrUIStyleValue, "System Type", parse);
		break;

					/***************************************************/
	case VRTOKEN_INPUTMAP:		/* Format: "InputMap" assignment-expr string [";"] */
	case VRTOKEN_SYSTEM_INPUTMAP:	/***************************************************/
		token = vrParseSingleStringExpr(&(system->input_map_name), "System InputMap", parse);
		break;

					/*****************************************************/
	case VRTOKEN_PROCESS:		/* Format: "Procs" assignment-expr string-list [";"] */
	case VRTOKEN_SYSTEM_PROCS:	/*****************************************************/
		token = vrParseStringListExpr(&(system->proc_names), &(system->num_procs), "System Procs-list", parse);
		break;

					/*************************************************/
	case VRTOKEN_SYSTEM_MASTER:	/* Format: "Master" assignment-expr string [";"] */
					/*************************************************/
		token = vrParseSingleStringExpr(&(system->master_name), "System Master", parse);
		break;

					/******************************************************/
	case VRTOKEN_SYSTEM_SLAVES:	/* Format: "Slaves" assignment-expr string-list [";"] */
					/******************************************************/
		token = vrParseStringListExpr(&(system->slave_names), &(system->num_slaves), "System Slaves-list", parse);
		break;

					/***********************************************/
	/*****************************************************/
	/* these are fields that can be inherited/overridden */

					/*********************************************/
	case VRTOKEN_SYSTEM_PRECONTEXT:	/* Format: "PreContext" printstyle-value ";" */
					/*********************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.pre_context_print), vrPrintStyleValue, "Global PreContextPrint", parse);
		break;

					/********************************************/
	case VRTOKEN_SYSTEM_PRECONFIG:	/* Format: "PreConfig" printstyle-value ";" */
					/********************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.pre_config_print), vrPrintStyleValue, "Global PreConfigPrint", parse);
		break;

					/*******************************************/
	case VRTOKEN_SYSTEM_PREINPUT:	/* Format: "PreInput" printstyle-value ";" */
					/*******************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.pre_input_print), vrPrintStyleValue, "Global PreInputPrint", parse);
		break;

					/**********************************************/
	case VRTOKEN_SYSTEM_POSTCONTEXT:/* Format: "PostContext" printstyle-value ";" */
					/**********************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.post_context_print), vrPrintStyleValue, "Global PostContextPrint", parse);
		break;

					/*********************************************/
	case VRTOKEN_SYSTEM_POSTCONFIG:	/* Format: "PostConfig" printstyle-value ";" */
					/*********************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.post_config_print), vrPrintStyleValue, "Global PostConfigPrint", parse);
		break;

					/********************************************/
	case VRTOKEN_SYSTEM_POSTINPUT:	/* Format: "PostInput" printstyle-value ";" */
					/********************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.post_input_print), vrPrintStyleValue, "Global PostInputPrint", parse);
		break;

					/****************************************************/
	case VRTOKEN_EXECSTART:		/* Format: "ExecAtStart" assignment-expr string ";" */
					/****************************************************/
		token = vrParseSingleStringExpr(&(system->settings.exec_start), "System ExecAtStart", parse);
		break;

					/***************************************************/
	case VRTOKEN_EXECSTOP:		/* Format: "ExecAtStop" assignment-expr string ";" */
					/***************************************************/
		token = vrParseSingleStringExpr(&(system->settings.exec_stop), "System ExecAtStop", parse);
		break;

					/******************************************************/
	case VRTOKEN_EXECUPONERROR:	/* Format: "ExecUponError" assignment-expr string ";" */
					/******************************************************/
		token = vrParseSingleStringExpr(&(system->settings.exec_uponerror), "System ExecUponError", parse);
		break;

					/****************************************************************************/
	case VRTOKEN_EXITUPONERROR:	/* Format: "ExitUponError" assignment-expr { "yes" | "no" | "default" } ";" */
					/****************************************************************************/
		token = vrParseSingleIntegerExpr(&(system->settings.exit_uponerror), "System ExitUponError", parse);
		break;

					/********************************************************************/
	case VRTOKEN_SYSTEM_VISCHAN:	/* Format: "VisrenMode" assignment-expr { "mono" | "dualfb" | ... } */
					/********************************************************************/
		token = vrParseSingleEnumerator((int *)&(system->settings.visrenmode), vrVisrenModeValue, "System VisrenMode", parse);
		break;

					/**************************************************/
	case VRTOKEN_EYELIST:		/* Format: "EyeList" assignment-expr string [";"] */
					/**************************************************/
		token = vrParseSingleStringExpr(&(system->settings.eyelist_name), "System EyeList", parse);
		break;

					/***********************************************/
	case VRTOKEN_NEARCLIP:		/* Format: "NEAR" assignment-expr number [";"] */
					/***********************************************/
		token = vrParseSingleFloatExpr(&(system->settings.near_clip), "System NearClip", parse);
		break;

					/**********************************************/
	case VRTOKEN_FARCLIP:		/* Format: "FAR" assignment-expr number [";"] */
					/**********************************************/
		token = vrParseSingleFloatExpr(&(system->settings.far_clip), "System FarClip", parse);
		break;

					/**********************************************************************/
	case VRTOKEN_SYSTEM_LOCKCPU:	/* Format: "LockCPU" assignment-expr { "yes" | "no" | "default" } ";" */
					/**********************************************************************/
		token = vrParseSingleIntegerExpr(&(system->settings.lock_proc), "System LockCPU", parse);
		break;

					/*************************/
	default:			/* Error: unknown option */
					/*************************/
		/* First test whether the option is a global config command */
		token = _ConfigParseGlobalOption(token, parse, config, object, objparser);
vrPrintf("\n\n\nHere I am!!!\n\n\n");

		if (token.token == VRTOKEN_UNKNOWN) {
			/* If the unknown token is returned, then print a warning */
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
				RED_TEXT "Unknown System option -- got '%s'\n" NORM_TEXT,
				token.linenum, token.string);

#if 0 /* this is already taken care of within the call to _ConfigParseGlobalOption() */
			/* read and skip the rest of the statement */
			token = vrParseToEOS(parse);
			vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				token.linenum, token.string);
#else
			/* _ConfigParseGlobalOption() doesn't advance over the token, so we need to read past that token here. */
			token = vrParseNextToken(parse);
#endif
		} else {
			vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read global option '%s'\n",
				token.linenum, token.string);

			/* when a global option is successfully read, we need to advance the parser */
			token = vrParseNextToken(parse);
		}
		break;
	}

	if (vrDbgDo(CONFIG_DBGLVL)) {
		vrFprintf(stdout, "_ConfigParseSystemOption(): Just parsed '%s' token on line %d.\n", orig_token.string, orig_token.linenum);
		vrFprintObjectInfo(stdout, (vrObjectInfo *)system, verbose);
	}
	vrTrace("_ConfigParseSystemOption()", RED_TEXT "exiting" NORM_TEXT);
	return token;
}


/***************/
/*** Process ***/
/***************/

/*  Format is: ( type .... ) */
/* _ConfigParseProcOption(): receives the first token to parse, which       */
/*   should be one of the Process definition options. (ie. "type" or "lock" */
/*   continue to read new tokens until hitting a semicolon, or a closecurl.)*/
/*   Return the last token read.                                            */
static vrTokenInfo _ConfigParseProcOption(vrObjectInfo *object, vrTokenInfo token, vrParseInfo *parse, vrConfigInfo *config, _ConfigParseOptionList objparser)
{
	vrProcessInfo	*proc;
	vrObject	*src_object;			/* used for copying an existing object */
	vrTokenInfo	orig_token = token;		/* store the incoming token for debug info */
	int		temp_int;
#if 0 /* BS: this is part of code still under development/consideration */
	char		*temp_char_ptr;
#endif

	vrTrace("_ConfigParseProcOption()", RED_TEXT "entering" NORM_TEXT);

	proc = (vrProcessInfo *)object;

	if (proc == NULL) {
		/* NOTE: this should never be true.  If so, then fix where */
		/*   this is called by using a temporary proc object.      */
		vrErrPrintf("FreeVR config: Error, Hey, 'proc' is NULL no place to put the values\n" NORM_TEXT);
	}

	switch (token.token) {

					/***************/
	case VRTOKEN_SEMICOLON:		/* Format: ";" */
					/***************/
		/* extra semicolons are never a problem */
		token = vrParseNextToken(parse);
		break;

					/***************/		/* need matching "{" */
	case VRTOKEN_CLOSECURL:		/* Format: "}" */
					/***************/
		/* if we hit the end of the list, then we're done */
		/*   (if this happens here, then it's probably an empty list) */
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read '}' in process option -- empty list?\n", token.linenum);
		break;

					/****************************************************/
	case VRTOKEN_MALLEABLE:		/* Format: "Malleable" assignment-expr number [";"] */
					/****************************************************/
		token = vrParseSingleIntegerExpr(&(proc->malleable), "Proc Malleable", parse);
		break;

					/************************************/
	case VRTOKEN_COPYOBJECT:	/* Format: "copyof" object-name ";" */
					/************************************/
		/*** read the name of an object for copying ***/
		token = vrParseNextToken(parse);
		src_object = vrObjectSearch(config->context, object->object_type, token.string);
		if (src_object == NULL) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Can't find Process object \"%s\" for copying.\n" NORM_TEXT,
				token.linenum, token.string);
		} else {
			vrObjectCopy(config->context, object, src_object);
			if (vrDbgDo(CONFIG_DBGLVL)) {
				vrFprintObjectInfo(stdout, object, verbose);
			}
		}
		/* a copyof operation should be terminated by a semicolon. */
		token = vrParseNextToken(parse);
		if (token.token != VRTOKEN_SEMICOLON) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Improper termination to the object copy option.\n" NORM_TEXT,
				token.linenum);
		}
		break;


	/********************************************/
	/* the rest of the cases are only for procs */

					/*********************************************/
	case VRTOKEN_TYPE:		/* Format: "Type" assignment-expr enum [";"] */
					/*********************************************/
		token = vrParseNextToken(parse);
		if (token.token != VRTOKEN_SETEQUALS)
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Expecting an assignment operator for Proc Type.\n" NORM_TEXT,
				token.linenum);
		token = vrParseNextToken(parse);
		switch (token.token) {
		case VRTOKEN_PROCESS_MAIN:
			proc->type = VRPROC_MAIN;
			break;
		case VRTOKEN_PROCESS_COMPUTE:
			proc->type = VRPROC_COMPUTE;
			break;
		case VRTOKEN_PROCESS_TELNET:
			proc->type = VRPROC_TELNET;
			break;
		case VRTOKEN_INPUT:
			proc->type = VRPROC_INPUT;
			break;
		case VRTOKEN_PROCESS_VISREN:
			proc->type = VRPROC_VISREN;
			break;
		case VRTOKEN_PROCESS_AUDREN:
			proc->type = VRPROC_AUDREN;
			break;
		case VRTOKEN_PROCESS_HAPREN:
			proc->type = VRPROC_HAPREN;
			break;
		case VRTOKEN_PROCESS_OLFREN:
			proc->type = VRPROC_OLFREN;
			break;
		case VRTOKEN_PROCESS_TASREN:
			proc->type = VRPROC_TASREN;
			break;
		case VRTOKEN_PROCESS_VESREN:
			proc->type = VRPROC_VESREN;
			break;

		default:
			proc->type = VRPROC_UNKNOWN;
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Invalid Process type given (%s).\n" NORM_TEXT,
				token.linenum, token.string);
		}
		token = vrParseNextToken(parse);
		if (token.token != VRTOKEN_SEMICOLON && token.token != VRTOKEN_CLOSECURL)
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Process can only be one type.\n" NORM_TEXT,
				token.linenum);
		break;

					/***********************************************/
	case VRTOKEN_ARGS:		/* Format: "Args" assignment-expr string [";"] */
					/***********************************************/
		token = vrParseSingleStringExpr(&(proc->args), "Proc Args", parse);
		break;

					/************************************************/
	case VRTOKEN_PROCESS_STATSARGS:	/* Format: "Stats" assignment-expr string [";"] */
					/************************************************/
		token = vrParseSingleStringExpr(&(proc->stats_args), "Proc Stats", parse);
		break;

					/***********************************************/
	case VRTOKEN_PROCESS_SYNC:	/* Format: "Sync" assignment-expr number [";"] */
					/***********************************************/
		token = vrParseSingleIntegerExpr(&(proc->sync_id), "Proc Sync", parse);
		break;

					/**************************************************/
	case VRTOKEN_PROCESS_MACHINE:	/* Format: "Machine" assignment-expr string [";"] */
					/**************************************************/
		token = vrParseSingleStringExpr(&(proc->machine_name), "Proc Machine", parse);
		break;

					/*****************************************************/
	case VRTOKEN_DEBUG_LEVEL:	/* Format: "DebugLevel" assignment-expr number [";"] */
					/*****************************************************/
		token = vrParseSingleIntegerExpr(&temp_int, "Proc DebugLevel", parse);
		if (getenv("FREEVR_DEBUGLEVEL_NO") == NULL) {
			proc->settings.debug_level = temp_int;
			vrDbgPrintfN(CONFIG_DBGLVL, "FreeVR config: Setting %s Process DebugLevel to %d.\n",
				proc->name, proc->settings.debug_level);
		} else {
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "FreeVR config: Request to set Process %s DebugLevel to %d denied.\n",
				proc->name, temp_int);
		}
		break;

					/***************************************************/
	case VRTOKEN_DEBUG_EXACT:	/* Format: "DebugExact" assignment-expr number ";" */
					/***************************************************/
		token = vrParseSingleIntegerExpr(&temp_int, "Proc DebugExact", parse);
		if (getenv("FREEVR_DEBUGEXACT_NO") == NULL) {
			proc->settings.debug_exact = temp_int;
			vrDbgPrintfN(CONFIG_DBGLVL, "FreeVR config: Setting %s Process DebugExact to %d.\n",
				proc->name, proc->settings.debug_exact);
		} else {
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "FreeVR config: Request to set Process %s DebugExact to %d denied.\n",
				proc->name, temp_int);
		}
		break;

					/**************************************************/
	case VRTOKEN_PROCESS_USEC:	/* Format: "UsecMin" assignment-expr number [";"] */
					/**************************************************/
		token = vrParseSingleIntegerExpr(&(proc->usec_min), "Proc UsecMin", parse);
		break;

					/*****************************************************/
	case VRTOKEN_PROCESS_COLOR:	/* Format: "PrintColor" assignment-expr number [";"] */
					/*****************************************************/
		token = vrParseSingleIntegerExpr(&(proc->print_color), "Proc PrintColor", parse);
		break;

					/****************************************************/
	case VRTOKEN_PROCESS_STRING:	/* Format: "PrintString" assignment-expr string ";" */
					/****************************************************/
		token = vrParseSingleStringExpr(&(proc->print_string), "Proc PrintString", parse);
#if 0 /* 05/25/2006: I began implementing this (using code from vr_utils.c:vrShellCmd(), but then noticed it requires an extra string holder space, so decided to hold off a bit */
		/* TODO: since "proc->print_string" will be used in a printf, */
		/*   all instances of '%' need to be changed to '%%'.         */

		/* replace all occurances of '%' w/ "%%" first (from vr_utils.c:vrShellCmd() function) */
		while ((temp_char_ptr = strchr(proc->print_string, '%')) != NULL) {
			strncpy(buffer, proc->print_string, (size_t)(temp_char_ptr - proc->print_string));
			strcat(buffer, temp_char_ptr);
		}
#endif

		break;

					/**************************************************/
	case VRTOKEN_PROCESS_DBGFILE:	/* Format: "PrintFile" assignment-expr string ";" */
					/**************************************************/
		token = vrParseSingleStringExpr(&(proc->print_filename), "Proc PrintFile", parse);
		break;

					/**********************************************************************/
	case VRTOKEN_SYSTEM_LOCKCPU:	/* Format: "LockCPU" assignment-expr { "yes" | "no" | "default" } ";" */
					/**********************************************************************/
		token = vrParseSingleIntegerExpr(&(proc->settings.lock_proc), "Process LockCPU", parse);
		break;


	/** TODO: perhaps have process - dependent commands for locking and unlocking **/


					/****************************************************/
	case VRTOKEN_EXECSTART:		/* Format: "ExecAtStart" assignment-expr string ";" */
					/****************************************************/
		token = vrParseSingleStringExpr(&(proc->settings.exec_start), "Proc ExecAtStart", parse);
		break;

					/***************************************************/
	case VRTOKEN_EXECSTOP:		/* Format: "ExecAtStop" assignment-expr string ";" */
					/***************************************************/
		token = vrParseSingleStringExpr(&(proc->settings.exec_stop), "Proc ExecAtStop", parse);
		break;

					/*******************************************************/
	case VRTOKEN_PROCESS_OBJECT:	/* Format: "Objects" assignment-expr string-list [";"] */
	case VRTOKEN_PROCESS_OBJECTS:	/*******************************************************/
		/* TODO: perhaps we could also do VROTKEN_WINDOWS & VRTOKEN_DEVICES */
		/*   and verify that the token matches the type of process.         */
		token = vrParseStringListExpr(&(proc->thing_names), &(proc->num_thing_names), "Proc Objects-list", parse);
		break;

					/*************************/
	default:			/* Error: unknown option */
					/*************************/
		/* First test whether the option is a global config command */
		token = _ConfigParseGlobalOption(token, parse, config, object, objparser);

		if (token.token == VRTOKEN_UNKNOWN) {
			/* If the unknown token is returned, then print a warning */
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
				RED_TEXT "Unknown Process option -- got '%s'\n" NORM_TEXT,
				token.linenum, token.string);

			/* NOTE: there is no need to call vrParseToEOS() here to read and skip to the next */
			/*   statement, as that is handled within the call to _ConfigParseGlobalOption().  */

			/* I don't know why, but we need to read the next token here. */
			token = vrParseNextToken(parse);
		} else {
			vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read global option '%s'\n",
				token.linenum, token.string);

			/* when a global option is successfully read, we need to advance the parser */
			token = vrParseNextToken(parse);
		}
		break;
	}

	if (vrDbgDo(CONFIG_DBGLVL)) {
		vrFprintf(stdout, "_ConfigParseProcOption(): Just parsed '%s' token on line %d.\n", orig_token.string, orig_token.linenum);
		vrFprintObjectInfo(stdout, (vrObjectInfo *)proc, verbose);
	}
	vrTrace("_ConfigParseProcOption()", RED_TEXT "exiting" NORM_TEXT);
	return token;
}


/**************/
/*** Window ***/
/**************/

static vrTokenInfo _ConfigParseWindowOption(vrObjectInfo *object, vrTokenInfo token, vrParseInfo *parse, vrConfigInfo *config, _ConfigParseOptionList objparser)
{
	vrWindowInfo	*window;
	vrObject	*src_object;			/* used for copying an existing object */
	vrTokenInfo	orig_token = token;		/* store the incoming token for debug info */
	char		*parse_string = NULL;

	vrTrace("_ConfigParseWindowOption()", RED_TEXT "entering" NORM_TEXT);

	window = (vrWindowInfo *)object;
	switch (token.token) {

					/***************/
	case VRTOKEN_SEMICOLON:		/* Format: ";" */
					/***************/
		/* extra semicolons are never a problem */
		token = vrParseNextToken(parse);
		break;

					/***************/		/* need matching "{" */
	case VRTOKEN_CLOSECURL:		/* Format: "}" */
					/***************/
		/* if we hit the end of the list, then we're done */
		/*   (if this happens here, then it's probably an empty list) */
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read '}' in window option -- empty list?\n", token.linenum);
		break;

					/****************************************************/
	case VRTOKEN_MALLEABLE:		/* Format: "Malleable" assignment-expr number [";"] */
					/****************************************************/
		token = vrParseSingleIntegerExpr(&(window->malleable), "Window Malleable", parse);
		break;

					/************************************/
	case VRTOKEN_COPYOBJECT:	/* Format: "copyof" object-name ";" */
					/************************************/
		/*** read the name of an object for copying ***/
		token = vrParseNextToken(parse);
		src_object = vrObjectSearch(config->context, object->object_type, token.string);
		if (src_object == NULL) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Can't find Window object \"%s\" for copying.\n" NORM_TEXT,
				token.linenum, token.string);
		} else {
			vrObjectCopy(config->context, object, src_object);
			if (vrDbgDo(CONFIG_DBGLVL)) {
				vrFprintObjectInfo(stdout, object, verbose);
			}
		}
		/* a copyof operation should be terminated by a semicolon. */
		token = vrParseNextToken(parse);
		if (token.token != VRTOKEN_SEMICOLON) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Improper termination to the object copy option.\n" NORM_TEXT,
				token.linenum);
		}
		break;

	/**********************************************/
	/* the rest of the cases are only for windows */

					/************************************************/
	case VRTOKEN_WINDOW_MOUNT:	/* Format: "Mount" assignment-expr string [";"] */
					/************************************************/
		token = vrParseSingleStringExpr(&(parse_string), "Window MountType", parse);
		window->mount = vrWindowType(parse_string);
		break;

					/**************************************************/
	case VRTOKEN_EYELIST:		/* Format: "EyeList" assignment-expr string [";"] */
					/**************************************************/
		token = vrParseSingleStringExpr(&(window->settings.eyelist_name), "Window EyeList", parse);
		break;

					/***************************************************/
	case VRTOKEN_WINDOW_GRAPHICS:	/* Format: "Graphics" assignment-expr string [";"] */
					/***************************************************/
		token = vrParseSingleStringExpr(&(window->graphics), "Window GraphicsType", parse);
		break;

					/***********************************************/
	case VRTOKEN_ARGS:		/* Format: "Args" assignment-expr string [";"] */
					/***********************************************/
		token = vrParseSingleStringExpr(&(window->args), "Window Args", parse);
		break;

					/********************************************************************/
	case VRTOKEN_SYSTEM_VISCHAN:	/* Format: "VisrenMode" assignment-expr { "mono" | "dualfb" | ... } */
					/********************************************************************/
		token = vrParseSingleEnumerator((int *)(&(window->settings.visrenmode)), vrVisrenModeValue, "Window VisrenMode", parse);
		break;

					/****************************************************/
	case VRTOKEN_WINDOW_EYE_COORDS:	/* Format: "e2w_coords" assignment-expr num*9 [";"] */
					/****************************************************/
		/* TODO: is it reasonable to overload the use of the "coords_ll,lr,ul" fields? */
		token = vrParseMatrixFromCoords(&window->e2w_xform, window->coords_ll, window->coords_lr, window->coords_ul, "Window Coordinates", parse);
		break;

					/********************************************************/
	case VRTOKEN_WINDOW_COORDS:	/* Format: "rw2w_coords" assignment-expr num*9 [";"] */
					/********************************************************/
		token = vrParseMatrixFromCoords(&window->rw2w_xform, window->coords_ll, window->coords_lr, window->coords_ul, "Window Coordinates", parse);
		break;

					/********************************************************/
	case VRTOKEN_WINDOW_TRANSFORM:	/* Format: "rw2w_transform" assignment-expr num*16 [";"] */
					/********************************************************/
		token = vrParseMatrixTransform(&window->rw2w_xform, "Window Transform", parse);
		/* NOTE: the methods for specifying the rw2w xform other than the coords method don't ultimately */
 		/*   set the coords_{ll,lr,ul} values, which are needed for the perspective calculations!        */
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "rw2w_transform is not yet fully implemented.\n" NORM_TEXT,
			token.linenum);
		vrSleep(500000);
		break;

					/**************************************************************/
	case VRTOKEN_WINDOW_TRANSLATE:	/* Format: "rw2w_translate" assignment-expr num num num [";"] */
					/**************************************************************/
		token = vrParseMatrixTranslate(&window->rw2w_xform, "Window Translate", parse);
		/* NOTE: the methods for specifying the rw2w xform other than the coords method don't ultimately */
 		/*   set the coords_{ll,lr,ul} values, which are needed for the perspective calculations!        */
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "rw2w_translate is not yet fully implemented.\n" NORM_TEXT,
			token.linenum);
		vrSleep(500000);
		break;

					/***************************************************************/
	case VRTOKEN_WINDOW_ROTATE:	/* Format: "rw2w_rotate" assignment-expr num num num num [";"] */
					/***************************************************************/
		token = vrParseMatrixRotate(&window->rw2w_xform, "Window Rotate", parse);
		/* NOTE: the methods for specifying the rw2w xform other than the coords method don't ultimately */
 		/*   set the coords_{ll,lr,ul} values, which are needed for the perspective calculations!        */
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "rw2w_rotate is not yet fully implemented.\n" NORM_TEXT,
			token.linenum);
		vrSleep(500000);
		break;

					/***********************************************/
	case VRTOKEN_NEARCLIP:		/* Format: "NEAR" assignment-expr number [";"] */
					/***********************************************/
		token = vrParseSingleFloatExpr(&(window->settings.near_clip), "Window NearClip", parse);
		break;

					/**********************************************/
	case VRTOKEN_FARCLIP:		/* Format: "FAR" assignment-expr number [";"] */
					/**********************************************/
		token = vrParseSingleFloatExpr(&(window->settings.far_clip), "Window FarClip", parse);
		break;

					/**************************************************/
	case VRTOKEN_WINDOW_UISHOW:	/* Format: "showUI" assignment-expr number [";"] */
					/**************************************************/
		token = vrParseSingleIntegerExpr(&(window->ui_show), "Window Show-UI", parse);
		break;

					/**********************************************************/
	case VRTOKEN_WINDOW_UILOC:	/* Format: "placeUI" assignment-expr number number [";"] */
					/**********************************************************/
		token = vrParseFloatListExpr(window->ui_loc, 2, "Window Place-UI", parse);
		break;

					/*****************************************************************/
	case VRTOKEN_WINDOW_UICOL:	/* Format: "colorUI" assignment-expr number number number [";"] */
					/*****************************************************************/
		token = vrParseFloatListExpr(window->ui_color, 3, "Window Color-UI", parse);
		break;

					/**************************************************/
	case VRTOKEN_WINDOW_FPSSHOW:	/* Format: "showFps" assignment-expr number [";"] */
					/**************************************************/
		token = vrParseSingleIntegerExpr(&(window->fps_show), "Window Show-FPS", parse);
		break;

					/**********************************************************/
	case VRTOKEN_WINDOW_FPSLOC:	/* Format: "placeFps" assignment-expr number number [";"] */
					/**********************************************************/
		token = vrParseFloatListExpr(window->fps_loc, 2, "Window Place-FPS", parse);
		break;

					/*****************************************************************/
	case VRTOKEN_WINDOW_FPSCOL:	/* Format: "colorFps" assignment-expr number number number [";"] */
					/*****************************************************************/
		token = vrParseFloatListExpr(window->fps_color, 3, "Window Color-FPS", parse);
		break;

					/****************************************************/
	case VRTOKEN_WINDOW_STATSSHOW:	/* Format: "showStats" assignment-expr number [";"] */
					/****************************************************/
		token = vrParseSingleIntegerExpr(&(window->stats_show), "Window Show-Stats", parse);
		break;

					/*****************************************************/
	case VRTOKEN_WINDOW_STATSPROCS:	/* Format: "statsProcs" assignment-expr string [";"] */
					/*****************************************************/
		token = vrParseSingleStringExpr(&(window->stats_procs), "Window statsProcs", parse);
		break;

					/****************************************************/
	case VRTOKEN_WINDOW_FRAMESHOW:	/* Format: "showFrame" assignment-expr number [";"] */
					/****************************************************/
		token = vrParseSingleIntegerExpr(&(window->show_in_simulator), "Window Show-Frame", parse);
		break;

					/**************************************************/
	case VRTOKEN_WINDOW_SIMMASK:	/* Format: "simMask" assignment-expr number [";"] */
					/**************************************************/
		token = vrParseSingleIntegerExpr(&(window->simulator_mask), "Window Simulator-Mask", parse);
		break;

					/************************************************************************/
	case VRTOKEN_WINDOW_VIEWPORT:	/* Format: "viewport[:both]" assignment-expr number number number number [";"] */
					/************************************************************************/
		token = vrParseIntegerListExpr((int *)&window->viewport_left, 4, "Window Viewport", parse);
		window->viewport_right =  window->viewport_left;
		break;

					   /***********************************************************************/
	case VRTOKEN_WINDOW_VIEWPORT_LEFT: /* Format: "viewport:left" assignment-expr number number number number [";"] */
					   /***********************************************************************/
		token = vrParseIntegerListExpr((int *)&window->viewport_left, 4, "Window Viewport:Left", parse);
		break;

					    /******************************************************************************/
	case VRTOKEN_WINDOW_VIEWPORT_RIGHT: /* Format: "viewport:right" assignment-expr number number number number [";"] */
					    /******************************************************************************/
		token = vrParseIntegerListExpr((int *)&window->viewport_right, 4, "Window Viewport:Right", parse);
		break;

					/************************************************************************/
	case VRTOKEN_WINDOW_FVIEWPORT:	/* Format: "viewportf[:both]" assignment-expr number number number number [";"] */
					/************************************************************************/
		token = vrParseFloatListExpr((float *)&window->viewportF_left, 4, "Window ViewportF", parse);
		window->viewportF_right =  window->viewportF_left;
		break;

					   /***********************************************************************/
	case VRTOKEN_WINDOW_FVIEWPORT_LEFT: /* Format: "viewportf:left" assignment-expr number number number number [";"] */
					   /***********************************************************************/
		token = vrParseFloatListExpr((float *)&window->viewportF_left, 4, "Window ViewportF:Left", parse);
		break;

					    /******************************************************************************/
	case VRTOKEN_WINDOW_FVIEWPORT_RIGHT: /* Format: "viewportf:right" assignment-expr number number number number [";"] */
					    /******************************************************************************/
		token = vrParseFloatListExpr((float *)&window->viewportF_right, 4, "Window ViewportF:Right", parse);
		break;


					/******************************/
					/* Error: Not Yet Implemented */
					/******************************/
	case VRTOKEN_WINDOW_VIEWMASK:
	case VRTOKEN_WINDOW_COLORMASK:
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
			RED_TEXT "Unimplemented Window option -- got '%s'\n" NORM_TEXT,
			token.linenum, token.string);

		/* read and skip the rest of the statement */
		token = vrParseToEOS(parse);
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
			token.linenum, token.string);
		break;


					/*************************/
	default:			/* Error: unknown option */
					/*************************/
		/* First test whether the option is a global config command */
		token = _ConfigParseGlobalOption(token, parse, config, object, objparser);

		if (token.token == VRTOKEN_UNKNOWN) {
			/* If the unknown token is returned, then print a warning */
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
				RED_TEXT "Unknown Window option -- got '%s'\n" NORM_TEXT,
				token.linenum, token.string);

			/* NOTE: there is no need to call vrParseToEOS() here to read and skip to the next */
			/*   statement, as that is handled within the call to _ConfigParseGlobalOption().  */

			/* I don't know why, but we need to read the next token here. */
			token = vrParseNextToken(parse);
		} else {
			vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read global option '%s'\n",
				token.linenum, token.string);

			/* when a global option is successfully read, we need to advance the parser */
			token = vrParseNextToken(parse);
		}
		break;
	}

	if (vrDbgDo(CONFIG_DBGLVL)) {
		vrFprintf(stdout, "_ConfigParseWindowOption(): Just parsed '%s' token on line %d.\n", orig_token.string, orig_token.linenum);
		vrFprintObjectInfo(stdout, (vrObjectInfo *)window, verbose);
	}
	vrTrace("_ConfigParseWindowOption()", RED_TEXT "exiting" NORM_TEXT);
	return token;
}


/***************/
/*** Eyelist ***/
/***************/

static vrTokenInfo _ConfigParseEyelistOption(vrObjectInfo *object, vrTokenInfo token, vrParseInfo *parse, vrConfigInfo *config, _ConfigParseOptionList objparser)
{
	vrEyelistInfo	*eyelist;
	vrObject	*src_object;			/* used for copying an existing object */
	vrTokenInfo	orig_token = token;		/* store the incoming token for debug info */

	vrTrace("_ConfigParseEyelistOption()", RED_TEXT "entering" NORM_TEXT);

	eyelist = (vrEyelistInfo *)object;
	switch (token.token) {

					/***************/
	case VRTOKEN_SEMICOLON:		/* Format: ";" */
					/***************/
		/* extra semicolons are never a problem */
		token = vrParseNextToken(parse);
		break;

					/***************/		/* need matching "{" */
	case VRTOKEN_CLOSECURL:		/* Format: "}" */
					/***************/
		/* if we hit the end of the list, then we're done */
		/*   (if this happens here, then it's probably an empty list) */
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read '}' in eyelist option -- empty list?\n", token.linenum);
		break;

					/****************************************************/
	case VRTOKEN_MALLEABLE:		/* Format: "Malleable" assignment-expr number [";"] */
					/****************************************************/
		token = vrParseSingleIntegerExpr(&(eyelist->malleable), "Eyelist Malleable", parse);
		break;

					/************************************/
	case VRTOKEN_COPYOBJECT:	/* Format: "copyof" object-name ";" */
					/************************************/
		/*** read the name of an object for copying ***/
		token = vrParseNextToken(parse);
		src_object = vrObjectSearch(config->context, object->object_type, token.string);
		if (src_object == NULL) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Can't find Eyelist object \"%s\" for copying.\n" NORM_TEXT,
				token.linenum, token.string);
		} else {
			vrObjectCopy(config->context, object, src_object);
			if (vrDbgDo(CONFIG_DBGLVL)) {
				vrFprintObjectInfo(stdout, object, verbose);
			}
		}
		/* a copyof operation should be terminated by a semicolon. */
		token = vrParseNextToken(parse);
		if (token.token != VRTOKEN_SEMICOLON) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Improper termination to the object copy option.\n" NORM_TEXT,
				token.linenum);
		}
		break;


	/***********************************************/
	/* the rest of the cases are only for eyelists */

					/******************************************************/
	case VRTOKEN_EYELIST_MONOFB:	/* Format: "MonoFB" assignment-expr string-list [";"] */
					/******************************************************/
		token = vrParseStringListExpr(&(eyelist->monofb_names), &(eyelist->num_monofb_names), "MonoFB list", parse);
		break;

					/******************************************************/
	case VRTOKEN_EYELIST_LEFTFB:	/* Format: "LeftFB" assignment-expr string-list [";"] */
					/******************************************************/
		token = vrParseStringListExpr(&(eyelist->leftfb_names), &(eyelist->num_leftfb_names), "LeftFB list", parse);
		break;

					/*******************************************************/
	case VRTOKEN_EYELIST_RIGHTFB:	/* Format: "RightFB" assignment-expr string-list [";"] */
					/*******************************************************/
		token = vrParseStringListExpr(&(eyelist->rightfb_names), &(eyelist->num_rightfb_names), "RightFB list", parse);
		break;

					/*****************************************************/
	case VRTOKEN_EYELIST_LEFTVP:	/* Format: "LeftVP" assignment-expr string-list [";"] */
					/*****************************************************/
		token = vrParseStringListExpr(&(eyelist->leftvp_names), &(eyelist->num_leftvp_names), "LeftVP list", parse);
		break;

					/********************************************************/
	case VRTOKEN_EYELIST_RIGHTVP:	/* Format: "RightVP" assignment-expr string-list [";"] */
					/********************************************************/
		token = vrParseStringListExpr(&(eyelist->rightvp_names), &(eyelist->num_rightvp_names), "RightVP list", parse);
		break;

					/*******************************************************/
	case VRTOKEN_EYELIST_ANAGLFB:	/* Format: "AnaglFB" assignment-expr string-list [";"] */
					/*******************************************************/
		token = vrParseStringListExpr(&(eyelist->anaglfb_names), &(eyelist->num_anaglfb_names), "AnaglFB list", parse);
		break;


					/*************************/
	default:			/* Error: unknown option */
					/*************************/
		/* First test whether the option is a global config command */
		token = _ConfigParseGlobalOption(token, parse, config, object, objparser);

		if (token.token == VRTOKEN_UNKNOWN) {
			/* If the unknown token is returned, then print a warning */
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
				RED_TEXT "Unknown Eyelist option -- got '%s'\n" NORM_TEXT,
				token.linenum, token.string);

			/* NOTE: there is no need to call vrParseToEOS() here to read and skip to the next */
			/*   statement, as that is handled within the call to _ConfigParseGlobalOption().  */

			/* I don't know why, but we need to read the next token here. */
			token = vrParseNextToken(parse);
		} else {
			vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read global option '%s'\n",
				token.linenum, token.string);

			/* when a global option is successfully read, we need to advance the parser */
			token = vrParseNextToken(parse);
		}
		break;
	}

	if (vrDbgDo(CONFIG_DBGLVL)) {
		vrFprintf(stdout, "_ConfigParseEyelistOption(): Just parsed '%s' token on line %d.\n", orig_token.string, orig_token.linenum);
		vrFprintObjectInfo(stdout, (vrObjectInfo *)eyelist, verbose);
	}
	vrTrace("_ConfigParseEyelistOption()", RED_TEXT "exiting" NORM_TEXT);
	return token;
}


/************/
/*** User ***/
/************/

static vrTokenInfo _ConfigParseUserOption(vrObjectInfo *object, vrTokenInfo token, vrParseInfo *parse, vrConfigInfo *config, _ConfigParseOptionList objparser)
{
	vrUserInfo	*user;
	vrObject	*src_object;			/* used for copying an existing object */
	vrTokenInfo	orig_token = token;		/* store the incoming token for debug info */
#ifdef UNPICKY_COMPILER
	char		*parse_string = NULL;
#endif

	vrTrace("_ConfigParseUserOption()", RED_TEXT "entering" NORM_TEXT);

	user = (vrUserInfo *)object;
	switch (token.token) {

					/***************/
	case VRTOKEN_SEMICOLON:		/* Format: ";" */
					/***************/
		/* extra semicolons are never a problem */
		token = vrParseNextToken(parse);
		break;

					/***************/		/* need matching "{" */
	case VRTOKEN_CLOSECURL:		/* Format: "}" */
					/***************/
		/* if we hit the end of the list, then we're done */
		/*   (if this happens here, then it's probably an empty list) */
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read '}' in user option -- empty list?\n", token.linenum);
		break;

					/****************************************************/
	case VRTOKEN_MALLEABLE:		/* Format: "Malleable" assignment-expr number [";"] */
					/****************************************************/
		token = vrParseSingleIntegerExpr(&(user->malleable), "User Malleable", parse);
		break;

					/************************************/
	case VRTOKEN_COPYOBJECT:	/* Format: "copyof" object-name ";" */
					/************************************/
		/*** read the name of an object for copying ***/
		token = vrParseNextToken(parse);
		src_object = vrObjectSearch(config->context, object->object_type, token.string);
		if (src_object == NULL) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Can't find User object \"%s\" for copying.\n" NORM_TEXT,
				token.linenum, token.string);
		} else {
			vrObjectCopy(config->context, object, src_object);
			if (vrDbgDo(CONFIG_DBGLVL)) {
				vrFprintObjectInfo(stdout, object, verbose);
			}
		}
		/* a copyof operation should be terminated by a semicolon. */
		token = vrParseNextToken(parse);
		if (token.token != VRTOKEN_SEMICOLON) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Improper termination to the object copy option.\n" NORM_TEXT,
				token.linenum);
		}
		break;


	/********************************************/
	/* the rest of the cases are only for users */

					/**********************************************/
	case VRTOKEN_USER_IOD:		/* Format: "IOD" assignment-expr number [";"] */
					/**********************************************/
		token = vrParseSingleFloatExpr(&(user->iod), "User IOD", parse);
		break;

					/*****************************************************/
	case VRTOKEN_USER_COLOR:	/* Format: "Color" assignment-expr num num num [";"] */
					/*****************************************************/
		token = vrParseFloatListExpr(user->color, 3, "User Color", parse);
		break;

					/***********************************************/
	case VRTOKEN_NEARCLIP:		/* Format: "NEAR" assignment-expr number [";"] */
					/***********************************************/
		token = vrParseSingleFloatExpr(&(user->settings.near_clip), "User NearClip", parse);
		break;

					/**********************************************/
	case VRTOKEN_FARCLIP:		/* Format: "FAR" assignment-expr number [";"] */
					/**********************************************/
		token = vrParseSingleFloatExpr(&(user->settings.far_clip), "User FarClip", parse);
		break;

					/******************************/
					/* Error: Not Yet Implemented */
					/******************************/
	case VRTOKEN_USER_HEADSENSOR:
	case VRTOKEN_USER_BODYSENSOR:
	case VRTOKEN_USER_REFTRANSFORM:
	case VRTOKEN_USER_REFTRANSLATE:
	case VRTOKEN_USER_REFROTATE:
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
			RED_TEXT "Unimplemented User option -- got '%s'\n" NORM_TEXT,
			token.linenum, token.string);

		/* read and skip the rest of the statement */
		token = vrParseToEOS(parse);
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
			token.linenum, token.string);
		break;


					/*************************/
	default:			/* Error: unknown option */
					/*************************/
		/* First test whether the option is a global config command */
		token = _ConfigParseGlobalOption(token, parse, config, object, objparser);

		if (token.token == VRTOKEN_UNKNOWN) {
			/* If the unknown token is returned, then print a warning */
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
				RED_TEXT "Unknown User option -- got '%s'\n" NORM_TEXT,
				token.linenum, token.string);

			/* NOTE: there is no need to call vrParseToEOS() here to read and skip to the next */
			/*   statement, as that is handled within the call to _ConfigParseGlobalOption().  */

			/* I don't know why, but we need to read the next token here. */
			token = vrParseNextToken(parse);
		} else {
			vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read global option '%s'\n",
				token.linenum, token.string);

			/* when a global option is successfully read, we need to advance the parser */
			token = vrParseNextToken(parse);
		}
		break;
	}

	if (vrDbgDo(CONFIG_DBGLVL)) {
		vrFprintf(stdout, "_ConfigParseUserOption(): Just parsed '%s' token on line %d.\n", orig_token.string, orig_token.linenum);
		vrFprintObjectInfo(stdout, (vrObjectInfo *)user, verbose);
	}
	vrTrace("_ConfigParseUserOption()", RED_TEXT "exiting" NORM_TEXT);
	return token;
}


/************/
/*** Prop ***/
/************/
/* TODO: */
static vrTokenInfo _ConfigParsePropOption(vrObjectInfo *object, vrTokenInfo token, vrParseInfo *parse, vrConfigInfo *config, _ConfigParseOptionList objparser)
{
	vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): " RED_TEXT "Sorry, Parsing of Prop options not yet implemented.\n" NORM_TEXT, token.linenum);

	return token;
}


/*************************/
/*** Input description ***/
/*************************/
/* the "input" object parsing -- happens within "inputdevice" objects */
/*   basically just parse:                                            */
	/* Format: "Input" "name" set-assignment-only string [";"] */
static vrTokenInfo _ConfigParseInputDescription(vrConfigInfo *config, vrParseInfo *parse, vrInputDevice *device)
{
	vrTokenInfo	token;
#ifdef UNPICKY_COMPILER
	vrTokenInfo	assignment_token;
#endif
	vrInput		*input;

	vrTrace("_ConfigParseInputDescription()", RED_TEXT "entering" NORM_TEXT);

	/* code example taken from _ConfigParseObjectDefinition(), below */


	/** next token is name of object to define/declare **/
	token = vrParseNextToken(parse);
	if (token.token != VRTOKEN_STRING) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "string should follow the '%s' option -- got \"%s\".\n" NORM_TEXT,
			token.linenum, "input", token.string);
		return token;
	}


	/** find an existing input object with the matching name in this list of inputs for this device **/
	input = (vrInput *)vrObjectListSearch(VROBJECT_INPUT, (vrObjectInfo *)(device->inputs), token.string);

	/* if one doesn't exist yet, make a new one. */
	if (input == NULL) {
		/* create a new input object definition (adding it to the inputdevice's private list of input objects) */
		input = (vrInput *)vrObjectListNew(config->context, VROBJECT_INPUT, (vrObjectInfo **)(&(device->inputs)), token.string);
		input->line_created = parse->linenum;
		strncpy(input->file_created, parse->parse_name, sizeof(input->file_created));
		vrDbgPrintfN(CONFIG_DBGLVL, "Created new %s: '%s' (%d)\n", "input object", input->name, input->id);
	} else {
		/* if one did exist, report a warning that the old description will be overwritten */
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
			RED_TEXT "Replacing definition of input \"%s::%s\".\n" NORM_TEXT,
			token.linenum, device->name, token.string);
		vrObjectClear((vrObject *)input);
	}

	/* set the last modified line and file for the input-object */
	input->line_lastmod = parse->linenum;
	strncpy(input->file_lastmod, parse->parse_name, sizeof(input->file_lastmod));

	/* store the current r2e transform for inputs that use this */
	input->r2e_xform = vrMatrixCopy(vrMatrixCreate(), device->r2e_xform);

	token = vrParseSingleStringExpr(&(input->desc_str), "Input Description", parse);

#if 0 /* TODO: this stuff doesn't work */
	/* Now that this input is part of "an object" let it point back to its objective self */
	input->type.generic->my_object = input;
vrPrintf("I just assigned 'input->type.generic->my_object = input' which means %#p = %#p\n", &(input->type.generic->my_object), input);
vrPrintf("input->name = '%s'  input->type = %#p   input->type.generic = %#p   input->type.2switch = %#p\n", input->name, input->type, input->type.generic, input->type.2switch);
#endif

	vrTrace("_ConfigParseInputDescription()", RED_TEXT "exiting" NORM_TEXT);

	return token;
}


/***************************/
/*** Control description ***/
/***************************/
/* the "control" object parsing -- happens within "inputdevice" objects */
/*   basically just parse:                                            */
	/* Format: "Control" "name" set-assignment-only string [";"] */
static vrTokenInfo _ConfigParseControlDescription(vrConfigInfo *config, vrParseInfo *parse, vrInputDevice *device)
{
	vrTokenInfo	token;
#ifdef UNPICKY_COMPILER
	vrTokenInfo	assignment_token;
#endif
	vrInput		*input;

	vrTrace("_ConfigParseControlDescription()", RED_TEXT "entering" NORM_TEXT);

	/* code example taken from _ConfigParseInputDescription(), above */


	/** next token is name of object to define/declare **/
	token = vrParseNextToken(parse);
	if (token.token != VRTOKEN_STRING) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "string should follow the '%s' option -- got \"%s\".\n" NORM_TEXT,
			token.linenum, "control", token.string);
		return token;
	}


	/** find an existing control object with the matching name in this list of controls for this device **/
	input = (vrInput *)vrObjectListSearch(VROBJECT_INPUT, (vrObjectInfo *)(device->self_controls), token.string);

	/* if one doesn't exist yet, make a new one. */
	if (input == NULL) {
		/* create a new control object definition (adding it to the inputdevice's private list of input objects) */
		input = (vrInput *)vrObjectListNew(config->context, VROBJECT_INPUT, (vrObjectInfo **)(&(device->self_controls)), token.string);
		input->line_created = parse->linenum;
		strncpy(input->file_created, parse->parse_name, sizeof(input->file_created));
		vrDbgPrintfN(CONFIG_DBGLVL, "Created new %s: '%s' (%d)\n", "control object", input->name, input->id);
	} else {
		/* if one did exist, report a warning that the old description will be overwritten */
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
			RED_TEXT "input device %s already has a control called '%s'."
			"  Will replace with new definition.\n" NORM_TEXT,
			token.linenum, device->name, token.string);
		vrObjectClear((vrObject *)input);
	}

	/* set the last modified line and file for the input-object */
	input->line_lastmod = parse->linenum;
	strncpy(input->file_lastmod, parse->parse_name, sizeof(input->file_lastmod));

	/* store the current r2e transform for inputs that use this   */
	/* (which I guess is unlikely for controls, but what the hey) */
	input->r2e_xform = vrMatrixCopy(vrMatrixCreate(), device->r2e_xform);

	token = vrParseSingleStringExpr(&(input->desc_str), "Control Description", parse);

	vrTrace("_ConfigParseControlDescription()", RED_TEXT "exiting" NORM_TEXT);

	return token;
}


/*******************/
/*** InputDevice ***/
/*******************/

static vrTokenInfo _ConfigParseIndevOption(vrObjectInfo *object, vrTokenInfo token, vrParseInfo *parse, vrConfigInfo *config, _ConfigParseOptionList objparser)
{
	vrInputDevice	*device;
	vrObject	*src_object;			/* used for copying an existing object */
	vrTokenInfo	orig_token = token;		/* store the incoming token for debug info */

	vrTrace("_ConfigParseIndevOption()", RED_TEXT "entering" NORM_TEXT);

	device = (vrInputDevice *)object;
	switch (token.token) {

					/***************/
	case VRTOKEN_SEMICOLON:		/* Format: ";" */
					/***************/
		/* extra semicolons are never a problem */
		token = vrParseNextToken(parse);
		break;

					/***************/ /* "{" */
	case VRTOKEN_CLOSECURL:		/* Format: "}" */
					/***************/
		/* if we get this, then it's the end of the option list. */
		/*   (if this happens here, then it's probably an empty list) */
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read '}' in indev option -- empty list?\n", token.linenum);
		break;

					/****************************************************/
	case VRTOKEN_MALLEABLE:		/* Format: "Malleable" assignment-expr number [";"] */
					/****************************************************/
		token = vrParseSingleIntegerExpr(&(device->malleable), "InputDevice Malleable", parse);
		break;

					/************************************/
	case VRTOKEN_COPYOBJECT:	/* Format: "copyof" object-name ";" */
					/************************************/
		/*** read the name of an object for copying ***/
		token = vrParseNextToken(parse);
		src_object = vrObjectSearch(config->context, object->object_type, token.string);
		if (src_object == NULL) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Can't find InputDevice object \"%s\" for copying.\n" NORM_TEXT,
				token.linenum, token.string);
		} else {
			vrObjectCopy(config->context, object, src_object);
			if (vrDbgDo(CONFIG_DBGLVL)) {
				vrFprintObjectInfo(stdout, object, verbose);
			}
		}
		/* a copyof operation should be terminated by a semicolon. */
		token = vrParseNextToken(parse);
		if (token.token != VRTOKEN_SEMICOLON) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "Improper termination to the object copy option.\n" NORM_TEXT,
				token.linenum);
		}
		break;


	/****************************************************/
	/* the rest of the cases are only for input devices */

					/***********************************************/
	case VRTOKEN_TYPE:		/* Format: "Type" assignment-expr string [";"] */
					/***********************************************/
		token = vrParseSingleStringExpr(&(device->type), "InputDevice Type", parse);
		break;

					/**************************************************/
	case VRTOKEN_DSOFILE:		/* Format: "DsoFile" assignment-expr string [";"] */
					/**************************************************/
		token = vrParseSingleStringExpr(&(device->dso_file), "InputDevice DsoFile", parse);
		break;

					/**************************************************/
	case VRTOKEN_DSOFUNC:		/* Format: "DsoFunc" assignment-expr string [";"] */
					/**************************************************/
		token = vrParseSingleStringExpr(&(device->dso_func), "InputDevice DsoFunc", parse);
		break;

					/***********************************************/
	case VRTOKEN_ARGS:		/* Format: "Args" assignment-expr string [";"] */
					/***********************************************/
		token = vrParseSingleStringExpr(&(device->args), "InputDevice Args", parse);
		break;

					/*******************************************************/
	case VRTOKEN_INPUT:		/* Format: "Input" "name" assignment-expr string [";"] */
					/*******************************************************/
		token = _ConfigParseInputDescription(config, parse, device);
		break;

					/*********************************************************/
	case VRTOKEN_CONTROL:		/* Format: "Control" "name" assignment-expr string [";"] */
					/*********************************************************/
		token = _ConfigParseControlDescription(config, parse, device);
		break;

					/********************************************************/
	case VRTOKEN_INDEV_REFTRANSFORM:/* Format: "t2rw_transform" assignment-expr num*16 [";"] */
					/********************************************************/
		token = vrParseMatrixTransform(&device->t2rw_xform, "InputDevice Transform", parse);
		break;

					/*************************************************************/
	case VRTOKEN_INDEV_REFTRANSLATE:/* Format: "t2rw_translate" assignment-expr num num num [";"] */
					/*************************************************************/
		token = vrParseMatrixTranslate(&device->t2rw_xform, "InputDevice Translate", parse);
		break;

					/**************************************************************/
	case VRTOKEN_INDEV_REFROTATE:	/* Format: "t2rw_rotate" assignment-expr num num num num [";"] */
					/**************************************************************/
		token = vrParseMatrixRotate(&device->t2rw_xform, "InputDevice Rotate", parse);
		break;

					/********************************************************/
	case VRTOKEN_INDEV_RECEIVER_TRANSFORM:/* Format: "r2e_transform" assignment-expr num*16 [";"] */
					/********************************************************/
		token = vrParseMatrixTransform(&device->r2e_xform, "InputDevice Transform", parse);
		break;

					/*************************************************************/
	case VRTOKEN_INDEV_RECEIVER_TRANSLATE:/* Format: "r2e_translate" assignment-expr num num num [";"] */
					/*************************************************************/
		token = vrParseMatrixTranslate(&device->r2e_xform, "InputDevice Translate", parse);
		break;

					/**************************************************************/
	case VRTOKEN_INDEV_RECEIVER_ROTATE:/* Format: "r2e_rotate" assignment-expr num num num num [";"] */
					/**************************************************************/
		token = vrParseMatrixRotate(&device->r2e_xform, "InputDevice Rotate", parse);
		break;

					/******************************/
					/* Error: Not Yet Implemented */
					/******************************/
	case VRTOKEN_INDEV_FUNCTION:
	case VRTOKEN_INDEV_DOF:
	case VRTOKEN_EXECSTART:
	case VRTOKEN_EXECSTOP:
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
			RED_TEXT "Unimplemented InputDevice option -- got '%s'\n" NORM_TEXT,
			token.linenum, token.string);

		/* read and skip the rest of the statement */
		token = vrParseToEOS(parse);
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
			token.linenum, token.string);
		break;


					/*************************/
	default:			/* Error: unknown option */
					/*************************/
		/* First test whether the option is a global config command */
		token = _ConfigParseGlobalOption(token, parse, config, object, objparser);

		if (token.token == VRTOKEN_UNKNOWN) {
			/* If the unknown token is returned, then print a warning */
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
				RED_TEXT "Unknown InputDevice option -- got '%s'\n" NORM_TEXT,
				token.linenum, token.string);

			/* NOTE: there is no need to call vrParseToEOS() here to read and skip to the next */
			/*   statement, as that is handled within the call to _ConfigParseGlobalOption().  */

			/* I don't know why, but we need to read the next token here. */
			token = vrParseNextToken(parse);
		} else {
			vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): read global option '%s'\n",
				token.linenum, token.string);

			/* when a global option is successfully read, we need to advance the parser */
			token = vrParseNextToken(parse);
		}
		break;
	}

	if (vrDbgDo(CONFIG_DBGLVL)) {
		vrFprintf(stdout, "_ConfigParseIndevOption(): Just parsed '%s' token on line %d.\n", orig_token.string, orig_token.linenum);
		vrFprintObjectInfo(stdout, (vrObjectInfo *)device, verbose);
	}
	vrTrace("_ConfigParseIndevOption()", RED_TEXT "exiting" NORM_TEXT);
	return token;
}



/****************/
/*** InputMap ***/
/****************/
/* TODO: */
static void _ConfigParseInmap(vrConfigInfo *config, vrParseInfo *parse)
{
	vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "Sorry, InputMap parsing not yet implemented.\n" NORM_TEXT);
}


/*********************************/
/*** Generic Object Definition ***/
/*********************************/


static void _ConfigParseObjectDefinition(vrConfigInfo *config, vrParseInfo *parse, vrObject objtype, char *objtypename, _ConfigParseOptionList objparser)
{
static	char		tmp_malleable_object[2048];	/* memory that can be changed in lieu of an object */
	int		opencurl_linenum;
	vrTokenInfo	token;
	vrTokenInfo	assignment_token;
	vrObjectInfo	*object = NULL;
	vrObjectInfo	*src_object = NULL;
	vrObjectInfo	*real_object = NULL;

	vrTrace("_ConfigParseObjectDefinition()", RED_TEXT "entering" NORM_TEXT);

	/** next token is name of object to define/declare **/
	token = vrParseNextToken(parse);
	if (token.token != VRTOKEN_STRING) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "string should follow the '%s' option -- got \"%s\".\n" NORM_TEXT,
			token.linenum, objtypename, token.string);
		return;
	}

	/* find an existing object with the matching name */
	object = vrObjectSearch(config->context, objtype, token.string);

	/* if the object doesn't exist yet, make a new one. */
	if (object == NULL) {
		/* create a new object definition */
		object = vrObjectNew(config->context, objtype, token.string);
		object->line_created = parse->linenum;
		strncpy(object->file_created, parse->parse_name, sizeof(object->file_created));
		vrDbgPrintfN(CONFIG_DBGLVL, "Created new %s: '%s' (%d)\n", objtypename, object->name, object->id);
	} else {
		vrDbgPrintfN(CONFIG_DBGLVL, "Found existing %s: '%s' (%d)\n", objtypename, object->name, object->id);
	}

	/* If the found object is not malleable, then make changes in */
	/*   some temporary memory.                                   */
	if (!object->malleable) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "Can't change unmalleable %s object \"%s\".\n" NORM_TEXT,
			token.linenum, vrObjectTypeName(object->object_type), object->name);
		real_object = object;
		object = (vrObjectInfo *)tmp_malleable_object;
		object->object_type = real_object->object_type;
		object->name = vrShmemStrDup("-- unmalleable --");
	}


	/** next token should be one of ("=" | "+=" | ";") **/
	token = vrParseNextToken(parse);

	if (token.token == VRTOKEN_SEMICOLON) {
		/* declaration of a object -- no settings */
		/*   ie. all the work was done above -- no more work to do */
		return;
	}
	assignment_token = token;
	if (assignment_token.token == VRTOKEN_SETEQUALS) {
		/* this will set the values from scratch (so clear old ones) */
		vrObjectClear(object);
	}
	if ((assignment_token.token == VRTOKEN_SETEQUALS) || (assignment_token.token == VRTOKEN_PLUSEQUALS)) {
		/* both '=' and '+=' will read in settings and assign them to the object */

		/** next token should be an option-keyword or an "{" **/  /* need this matching "}" */
		token = vrParseNextToken(parse);

		switch (token.token) {

						/************************************/
		case VRTOKEN_COPYOBJECT:	/* Format: "copyof" object-name ";" */
						/************************************/
			/*** read the name of an object for copying ***/
			if (assignment_token.token == VRTOKEN_PLUSEQUALS) {
				vrErrPrintf("FreeVR config: Error (line %d): "
					RED_TEXT "Can't merge objects with '+=' operation.\n" NORM_TEXT,
					token.linenum);
			}

			token = vrParseNextToken(parse);
			src_object = vrObjectSearch(config->context, objtype, token.string);
			if (src_object == NULL) {
				vrErrPrintf("FreeVR config: Error (line %d): "
					RED_TEXT "Can't find object \"%s\" for copying.\n" NORM_TEXT,
					token.linenum, token.string);
			} else {
				vrObjectCopy(config->context, object, src_object);
				if (vrDbgDo(CONFIG_DBGLVL)) {
					vrFprintObjectInfo(stdout, object, verbose);
				}
			}
			/* a copyof operation should be terminated by a semicolon. */
			token = vrParseNextToken(parse);
			if (token.token != VRTOKEN_SEMICOLON) {
				vrErrPrintf("FreeVR config: Error (line %d): "
					RED_TEXT "Improper termination to the object copy option.\n" NORM_TEXT,
					token.linenum);
			}
			break;

						/**************************************/
		case VRTOKEN_OPENCURL:		/* Format: "{" { object-option } "}" */
						/**************************************/
			/*** read a bunch of keyword options ***/
			token = vrParseNextToken(parse); /* skip past the "{" */  /* need this matching "}" */
			opencurl_linenum = token.linenum;
			do {
				token = (*objparser)(object, token, parse, config, objparser);
			} while (token.token != VRTOKEN_CLOSECURL && token.token != VRTOKEN_OPENCURL && token.token != VRTOKEN_EOF);

			/* if the above loop ended because of an EOF, then the */
			/*   opencurl was unmatched, so print an error message.*/
			if (token.token == VRTOKEN_EOF) {
				vrErrPrintf("FreeVR config: Error (line %d): "
					RED_TEXT "Unmatched '{'\n" NORM_TEXT,   /* need this matching '}' */
					opencurl_linenum);
			}

			/* if the above loop ended because of an opencurl, then */
			/*   there is a nested section (or missing closecurl),  */
			/*   so printed an error message.                       */
			if (token.token == VRTOKEN_OPENCURL) {
				vrErrPrintf("FreeVR config: Error (line %d): "
					RED_TEXT "Encountered a '{' -- nesting of blocks not allowed\n"
					"\t(still in block started on line %d).\n" NORM_TEXT,
					token.linenum, opencurl_linenum);  /* need this matching '}' */
			}
			break;

						/*****************************/
		default:			/* Format: object-option ";" */
						/*****************************/
			/*** read a single keyword option ***/
			token = (*objparser)(object, token, parse, config, objparser);

			/* a single keyworld should be terminated by a semicolon. */
			if (token.token != VRTOKEN_SEMICOLON) {
				vrErrPrintf("FreeVR config: Error (line %d): "
					RED_TEXT "Improper termination to the '%s' option.\n" NORM_TEXT,
					token.linenum, objtypename);
			}
		}

		/* set the last modified line and file for the object */
		object->line_lastmod = parse->linenum;
		strncpy(object->file_lastmod, parse->parse_name, sizeof(object->file_lastmod));
	}

	vrTrace("_ConfigParseObjectDefinition()", RED_TEXT "exiting" NORM_TEXT);
	return;
}


/********************************************/
/*** Default and Global Variable Settings ***/
/********************************************/

static void _ConfigParseAssignments(vrConfigInfo *config, vrParseInfo *parse)
{
	vrTokenInfo	token;
	int		temp_int;

	/** next token is name of setting to set **/
	token = vrParseNextToken(parse);

	switch (token.token) {
					/***************************************************/
	case VRTOKEN_DEBUG_LEVEL:	/* Format: "DebugLevel" assignment-expr number ";" */
					/***************************************************/
		temp_int =  config->defaults.debug_level;	/* set the current value in case there's an error */
		token = vrParseSingleIntegerExpr(&temp_int, "Global DebugLevel", parse);
		if (getenv("FREEVR_DEBUGLEVEL_NO") == NULL) {
			config->defaults.debug_level = temp_int;
			vrDbgPrintfN(CONFIG_DBGLVL, "FreeVR config: Setting Global DebugLevel to %d.\n", config->defaults.debug_level);
		} else {
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "FreeVR config: Request to set Global DebugLevel to %d denied.\n", temp_int);
		}
		break;

					/***************************************************/
	case VRTOKEN_DEBUG_EXACT:	/* Format: "DebugExact" assignment-expr number ";" */
					/***************************************************/
		temp_int =  config->defaults.debug_exact;	/* set the current value in case there's an error */
		token = vrParseSingleIntegerExpr(&temp_int, "Global DebugExact", parse);
		if (getenv("FREEVR_DEBUGEXACT_NO") == NULL) {
			config->defaults.debug_exact = temp_int;
			vrDbgPrintfN(CONFIG_DBGLVL, "FreeVR config: Setting Global DebugExact to %d.\n", config->defaults.debug_exact);
		} else {
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "FreeVR config: Request to set Global DebugExact to %d denied.\n", temp_int);
		}
		break;

					/*********************************************/
	case VRTOKEN_SYSTEM_PRECONTEXT:	/* Format: "PreContext" printstyle-value ";" */
					/*********************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.pre_context_print), vrPrintStyleValue, "Global PreContextPrint", parse);
		break;

					/********************************************/
	case VRTOKEN_SYSTEM_PRECONFIG:	/* Format: "PreConfig" printstyle-value ";" */
					/********************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.pre_config_print), vrPrintStyleValue, "Global PreConfigPrint", parse);
		break;

					/*******************************************/
	case VRTOKEN_SYSTEM_PREINPUT:	/* Format: "PreInput" printstyle-value ";" */
					/*******************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.pre_input_print), vrPrintStyleValue, "Global PreInputPrint", parse);
		break;

					/**********************************************/
	case VRTOKEN_SYSTEM_POSTCONTEXT:/* Format: "PostContext" printstyle-value ";" */
					/**********************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.post_context_print), vrPrintStyleValue, "Global PostContextPrint", parse);
		break;

					/*********************************************/
	case VRTOKEN_SYSTEM_POSTCONFIG:	/* Format: "PostConfig" printstyle-value ";" */
					/*********************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.post_config_print), vrPrintStyleValue, "Global PostConfigPrint", parse);
		break;

					/********************************************/
	case VRTOKEN_SYSTEM_POSTINPUT:	/* Format: "PostInput" printstyle-value ";" */
					/********************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.post_input_print), vrPrintStyleValue, "Global PostInputPrint", parse);
		break;

					/****************************************************/
	case VRTOKEN_EXECSTART:		/* Format: "ExecAtStart" assignment-expr string ";" */
					/****************************************************/
		token = vrParseSingleStringExpr(&(config->defaults.exec_start), "Global ExecAtStart", parse);
		break;

					/***************************************************/
	case VRTOKEN_EXECSTOP:		/* Format: "ExecAtStop" assignment-expr string ";" */
					/***************************************************/
		token = vrParseSingleStringExpr(&(config->defaults.exec_stop), "Global ExecAtStop", parse);
		break;

					/******************************************************/
	case VRTOKEN_EXECUPONERROR:	/* Format: "ExecUponError" assignment-expr string ";" */
					/******************************************************/
		token = vrParseSingleStringExpr(&(config->defaults.exec_uponerror), "Global ExecUponError", parse);
		break;

					/****************************************************************************/
	case VRTOKEN_EXITUPONERROR:	/* Format: "ExitUponError" assignment-expr { "yes" | "no" | "default" } ";" */
					/****************************************************************************/
		token = vrParseSingleIntegerExpr(&(config->defaults.exit_uponerror), "Global ExitUponError", parse);
		break;

					/*****************************************************************/
	case VRTOKEN_SYSTEM_LOCKCPU:	/* Format: "LockCPU" assignment-expr ("yes" | "no" | number) ";" */
					/*****************************************************************/
		token = vrParseSingleIntegerExpr(&(config->defaults.lock_proc), "Global LockCPU", parse);
		break;

					/****************************************************/
	case VRTOKEN_SYSTEM_LOCKCMD:	/* Format: "LockCommand" assignment-expr string ";" */
					/****************************************************/
		token = vrParseSingleStringExpr(&(config->defaults.proc_lock_cmd), "Global LockCommand", parse);
		break;

					/******************************************************/
	case VRTOKEN_SYSTEM_UNLOCKCMD:	/* Format: "UnlockCommand" assignment-expr string ";" */
					/******************************************************/
		token = vrParseSingleStringExpr(&(config->defaults.proc_unlock_cmd), "Global UnlockCommand", parse);
		break;

					/********************************************************************/
	case VRTOKEN_SYSTEM_VISCHAN:	/* Format: "VisrenMode" assignment-expr { "mono" | "dualfb" | ... } */
					/********************************************************************/
		token = vrParseSingleEnumerator((int *)&(config->defaults.visrenmode), vrVisrenModeValue, "Global VisrenMode", parse);
		break;

					/********************************************/
	case VRTOKEN_EYELIST:		/* Format: "EyeList" assignment-expr string */
					/********************************************/
		token = vrParseSingleStringExpr(&(config->defaults.eyelist_name), "Global Eyelist", parse);
		break;

					/***********************************************/
	case VRTOKEN_NEARCLIP:		/* Format: "NEAR" assignment-expr number [";"] */
					/***********************************************/
		token = vrParseSingleFloatExpr(&(config->defaults.near_clip), "Global NearClip", parse);
		break;

					/**********************************************/
	case VRTOKEN_FARCLIP:		/* Format: "FAR" assignment-expr number [";"] */
					/**********************************************/
		token = vrParseSingleFloatExpr(&(config->defaults.far_clip), "Global FarClip", parse);
		break;

					/******************************/
					/* Error: Not Yet Implemented */
					/******************************/
	case VRTOKEN_SYSTEM_TRANSFORM:
	case VRTOKEN_SYSTEM_TRANSLATE:
	case VRTOKEN_SYSTEM_ROTATE:
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
			RED_TEXT "Unimplemented global setting \"%s\".\n" NORM_TEXT,
			token.linenum, token.string);

		/* read and skip the rest of the statement */
		token = vrParseToEOS(parse);
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
			token.linenum, token.string);
		break;

					/**************************/
	default:			/* Error: unknown setting */
					/**************************/
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Config Parse Warning (line %d): "
			RED_TEXT "Unknown global setting \"%s\".\n" NORM_TEXT,
			token.linenum, token.string);

		/* read and skip the rest of the statement */
		token = vrParseToEOS(parse);
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
			token.linenum, token.string);
		break;
	}
}


/***********************************/
/*** PrintObject command parsing ***/
/***********************************/

/*************************************************************/
/*  Format is: "PRINTOBJECT" <object-type> <object-name> ";" */
static void _ConfigParsePrintobject(vrConfigInfo *config, vrParseInfo *parse)
{
	vrTokenInfo	token_type;
	vrTokenInfo	token_name;
	vrTokenInfo	token;
	vrObjectInfo	*object = NULL;
	char		*type_name = "unknown";

	/* TODO: need to check this parsing for format errors */
	/** next token is name of object type to print, followed by the name of the object **/
	token_type = vrParseNextToken(parse);
	token_name = vrParseNextToken(parse);

	switch (token_type.token) {
	case VRTOKEN_SYSTEM:
		type_name = "system";
		object = vrObjectSearch(config->context, VROBJECT_SYSTEM, token_name.string);
		break;

	case VRTOKEN_PROCESS:
		type_name = "process";
		object = vrObjectSearch(config->context, VROBJECT_PROCESS, token_name.string);
		break;

	case VRTOKEN_WINDOW:
		type_name = "window";
		object = vrObjectSearch(config->context, VROBJECT_WINDOW, token_name.string);
		break;

	case VRTOKEN_EYELIST:
		type_name = "eyelist";
		object = vrObjectSearch(config->context, VROBJECT_EYELIST, token_name.string);
		break;

	case VRTOKEN_USER:
		type_name = "user";
		object = vrObjectSearch(config->context, VROBJECT_USER, token_name.string);
		break;

	case VRTOKEN_PROP:
		type_name = "prop";
		object = vrObjectSearch(config->context, VROBJECT_PROP, token_name.string);
		break;

	case VRTOKEN_INPUTDEV:
		type_name = "input device";
		object = vrObjectSearch(config->context, VROBJECT_INDEV, token_name.string);
		break;

	case VRTOKEN_INPUT:
		type_name = "input object";
		object = vrObjectSearch(config->context, VROBJECT_INPUT, token_name.string);
		break;

	default:
		type_name = "unknown";
		object = NULL;
		break;
	}

	if (object == NULL) {
		vrErrPrintf(RED_TEXT "Warning, No %s object '%s'\n", type_name, token_name.string);
	} else if (vrDbgDo(AALWAYS_DBGLVL)) {
		vrFprintf(stdout, "Config (PrintObject): %s '%s' = ", type_name, token_name.string);
		vrFprintObjectInfo(stdout, object, verbose);
	}

	/** next token should be a statement terminator (ie. ';') **/
	token = vrParseNextToken(parse);
	if (token.token != VRTOKEN_SEMICOLON) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "'PrintObject' option not terminated with a ';'.\n" NORM_TEXT,
			token.linenum);
	}
}


/***********************************/
/*** PrintConfig command parsing ***/
/***********************************/

/*************************************************************/
/*  Format is: "PRINTCONFIG" <object-type> <object-name> ";" */
static void _ConfigParsePrintconfig(vrConfigInfo *config, vrParseInfo *parse)
{
	vrTokenInfo	file_name;
	vrTokenInfo	token;

	/* TODO: need to check this parsing for format errors */
	/** next token is name of the file to which to print **/
	file_name = vrParseNextToken(parse);

	/* Make sure a filename was given */
	if (vrTokenTerminal(file_name.token)) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "No filename given for 'PrintConfig' option.\n" NORM_TEXT,
			file_name.linenum);

		return;
	}

	/** next token should be a statement terminator (ie. ';') **/
	token = vrParseNextToken(parse);
	if (token.token != VRTOKEN_SEMICOLON) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "'PrintConfig' option not terminated with a ';'.\n" NORM_TEXT,
			token.linenum);
	}

	/******************************/
	/*** now handle the request ***/
	vrDbgPrintf("Config (PrintConfig): will print to file '%s'\n", file_name.string);
	config->config_print = strdup(file_name.string);
}


/********************************/
/*** DbxPause command parsing ***/
/********************************/

/******************************/
/*  Format is: "DBXPAUSE" ";" */
static void _ConfigParseDbxpause(vrConfigInfo *config, vrParseInfo *parse)
{
	vrTokenInfo	token;

	/** next token should be a statement terminator (ie. ';') **/
	token = vrParseNextToken(parse);
	if (token.token != VRTOKEN_SEMICOLON) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "'DbxPause' option not terminated with a ';'.\n" NORM_TEXT,
			token.linenum);
	}

	/* PAUSE to allow debugger to attatch to the process. */
	vrMsgPrintf("Process pausing, use 'dbx -p %d' to debug.\n", getpid());
	pause();
}


/******************************/
/*** Setenv command parsing ***/
/******************************/

/*******************************************/
/*  Format is: "SETENV" <name> <value> ";" */
static void _ConfigParseSetenv(vrConfigInfo *config, vrParseInfo *parse)
{
	char		*value_string;
	char		*assignment_string;
	vrTokenInfo	token_varname;
	vrTokenInfo	token_terminator;

	/* get the variable and it's value */
	token_varname = vrParseNextToken(parse);
	value_string = getenv(token_varname.string);

	/* use standard single string setting parser -- NOTE, it's okay if value_string is NULL */
	token_terminator = vrParseSingleStringExpr(&value_string, "Setenv", parse);

	/* do the assignment */
	/*   NOTE: must be done in non-shared memory since it will be stored in the */
	/*   system.  Fortunately though, we haven't forked yet, so all processes   */
	/*   will have these environment variable settings.                         */
	assignment_string = malloc(strlen(token_varname.string) + strlen(value_string) + 2);
	sprintf(assignment_string, "%s=%s", token_varname.string, value_string);

	/** the last token should be a statement terminator (ie. ';') **/
	if (token_terminator.token != VRTOKEN_SEMICOLON) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "'Setenv' option not terminated with a ';'.\n" NORM_TEXT,
			token_terminator.linenum);
	}

	vrDbgPrintfN(CONFIG_DBGLVL, "ConfigParse: setenv setting '%s'\n", assignment_string);
	if (putenv(assignment_string)) {
		vrErrPrintf(RED_TEXT "_ConfigParseSetenv: Unable to set the variable\n" NORM_TEXT);
	}
	vrDbgPrintfN(CONFIG_DBGLVL, "_ConfigParseSetenv: getenv(%s) now returns '%s'\n",
		token_varname.string, getenv(token_varname.string));
}


/*************************************/
/*  Format is: "UNSETENV" <name> ";" */
static void _ConfigParseUnsetenv(vrConfigInfo *config, vrParseInfo *parse)
{
	char		*assignment_string = vrShmemAlloc0(1024);
	vrTokenInfo	token_varname;
	vrTokenInfo	token_terminator;

	/* TODO: need to check this parsing for more format errors */
	token_varname = vrParseNextToken(parse);

	sprintf(assignment_string, "%s", token_varname.string);
	vrDbgPrintfN(CONFIG_DBGLVL, "ConfigParse: unsetting '%s'\n", assignment_string);
	if (putenv(assignment_string)) {
		vrErrPrintf(RED_TEXT "_ConfigParseSetenv: Unable to unset the variable\n" NORM_TEXT);
	}
	vrDbgPrintfN(CONFIG_DBGLVL, "_ConfigParseSetenv: getenv(%s) now returns '%s'\n",
		token_varname.string,
		(getenv(token_varname.string) ? getenv(token_varname.string) : "<null>"));

	/** next token should be a statement terminator (ie. ';') **/
	token_terminator = vrParseNextToken(parse);
	if (token_terminator.token != VRTOKEN_SEMICOLON) {
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "'Unsetenv' option not terminated with a ';'.\n" NORM_TEXT,
			token_terminator.linenum);
	}
}



/**************************/
/*** If command parsing ***/
/**************************/

/***************************************************************************/
/*  Format is: "IF" <condition> <statement(s)> ["ELSE" <statement(s)>] ";" */
static vrTokenInfo _ConfigParseIf(vrConfigInfo *config, vrParseInfo *parse, vrObjectInfo *object, _ConfigParseOptionList objparser)
{
	vrTokenInfo	token;
	vrTokenInfo	statement_token = { VRTOKEN_NOP, "no if statement" };
	vrTokenInfo	skip_token;		/* NOTE: only used for printing debugging info */
	vrTokenInfo	else_token;
	int		math_value;

	/** next token(s) are the math expression **/
	token = vrParseMathExpression(parse);
	math_value = vrAtoI(token.string);
	vrDbgPrintfN(PARSE_DBGLVL, "value of 'if' math-expression is '%s' (%d)\n", token.string, math_value);
	vrDbgPrintfN(VARIABLE_DBGLVL, "value of 'if' math-expression is '%s' (%d)\n", token.string, math_value);

	/** then read and perhaps execute the statement(s) **/
	if (math_value) {
		statement_token = _ConfigParseStatement(config, parse, object, objparser);
		vrDbgPrintfN(PARSE_DBGLVL, "Parsed statement '%s'\n", statement_token.string);
	} else {
		skip_token = vrParseToEOS(parse);
		vrDbgPrintfN(PARSE_DBGLVL, "Skipped statement '%s'\n", skip_token.string);
	}

	/** now get the next token, and if it's "else" then do the opposite **/
	else_token = vrParseNextToken(parse);

	/* If the next token is not "else", then put it back into the parser */
	if (else_token.token != VRTOKEN_ELSE) {
		vrDbgPrintfN(PARSE_DBGLVL, "Else: returning non-else token\n");
		vrParseReturnLastToken(else_token, parse);
		return statement_token;
	}

	/* now do or skip the next statement (the else clause) */
	if (!math_value) {
		statement_token = _ConfigParseStatement(config, parse, object, objparser);
		vrDbgPrintfN(PARSE_DBGLVL, "Else: Parsed statement '%s'\n", statement_token.string);
	} else {
		skip_token = vrParseToEOS(parse);
		vrDbgPrintfN(PARSE_DBGLVL, "Else: Skipped statement '%s'\n", skip_token.string);
	}

	return statement_token;
}


/*******************************************************************************/
/*  Format is: "IFEXEC" <shell cmd> <statement(s)> ["ELSE" <statement(s)>] ";" */
static vrTokenInfo _ConfigParseIfexec(vrConfigInfo *config, vrParseInfo *parse, vrObjectInfo *object, _ConfigParseOptionList objparser)
{
	vrTokenInfo	token;
	vrTokenInfo	statement_token = { VRTOKEN_NOP, "no if statement" };
	vrTokenInfo	skip_token;		/* NOTE: only used for printing debugging info */
	vrTokenInfo	else_token;
	int		exec_value;

	/** next token is the shell command **/
	token = vrParseNextToken(parse);
	if (token.token == VRTOKEN_STRING) {
		exec_value = system(token.string);
	} else {
		vrErrPrintf(RED_TEXT "Error, expected string to execute (line %d), skipping '%s'\n",
			token.linenum, token.string);
	}
	vrDbgPrintfN(PARSE_DBGLVL, "value of 'ifexec' shell command is '%s' (%d)\n", token.string, exec_value);

	/** then read and perhaps execute the statement(s) **/
	if (!exec_value) {
		statement_token = _ConfigParseStatement(config, parse, object, objparser);
		vrDbgPrintfN(PARSE_DBGLVL, "Parsed statement '%s'\n", statement_token.string);
	} else {
		skip_token = vrParseToEOS(parse);
		vrDbgPrintfN(PARSE_DBGLVL, "Skipped statement '%s'\n", skip_token.string);
	}

	/** now get the next token, and if it's "else" then do the opposite **/
	else_token = vrParseNextToken(parse);

	/* If the next token is not "else", then put it back into the parser */
	if (else_token.token != VRTOKEN_ELSE) {
		vrDbgPrintfN(PARSE_DBGLVL, "Else: returning non-else token\n");
		vrParseReturnLastToken(else_token, parse);
		return statement_token;
	}

	/* now do or skip the next statement (the else clause) */
	if (exec_value) {
		statement_token = _ConfigParseStatement(config, parse, object, objparser);
		vrDbgPrintfN(PARSE_DBGLVL, "Else: Parsed statement '%s'\n", statement_token.string);
	} else {
		skip_token = vrParseToEOS(parse);
		vrDbgPrintfN(PARSE_DBGLVL, "Else: Skipped statement '%s'\n", skip_token.string);
	}

	return statement_token;
}



/******************/
/*** ReadConfig ***/
/********************************************************************/
/*  Format is: 'READCONFIG' { <filename> } ; */
static void _ConfigParseReadconfig(vrConfigInfo *config, vrParseInfo *parse)
{
	vrTokenInfo	token;

	/** next token is name of file to read **/
	token = vrParseNextToken(parse);

#if 0 /* Since this wasn't implemented, and we're now using '|' for bitwise or, and we have another means to read pipes, we'll disable this proposed feature */
	if (token.token == VRTOKEN_VERTBAR) {
		token = vrParseNextToken(parse);
		if (token.token == VRTOKEN_STRING)
			vrErrPrintf(RED_TEXT "Sorry, Shell Command Config Input not yet implemented.\n" NORM_TEXT);
		else	vrErrPrintf(RED_TEXT "Error, expected a string (line %d), skipping '%s'\n",
				token.linenum, token.string);
	} else
#endif
	if (token.token == VRTOKEN_STRING) {
		/* First try to read the file in the global space, but if it */
		/*   doesn't exist, then read it from the home directory, and */
		/*   if that doesn't exist, then read it from the local directory. */
		/* TODO: consider whether this is the proper ordering. */
		switch (token.string[0]) {
		case '/':
		case '~':
			_ConfigReadFile(config, NULL, token.string);
			break;
		default:
			if (!_ConfigReadFile(config, config->context->homedir, token.string))
				if (!_ConfigReadFile(config, getenv("HOME"), token.string))
					_ConfigReadFile(config, ".", token.string);
			break;
		}
	} else {
		vrErrPrintf(RED_TEXT "Error, expected a filename string (line %d), skipping '%s'\n",
			token.linenum, token.string);
	}
}


/******************/
/*** ReadConfig ***/
/********************************************************************/
/*  Format is: 'READCONFIGSTRING' { <string> } ; */
static void _ConfigParseReadconfigstring(vrConfigInfo *config, vrParseInfo *parse)
{
	vrTokenInfo	token;
	char		info_string[256];	/* string used to indicate source of config data */

	/** next token is name of string to parse **/
	token = vrParseNextToken(parse);

	if (token.token == VRTOKEN_STRING) {
		sprintf(info_string, "on line %d of '%s'", parse->linenum, parse->parse_name);
		if (!_ConfigReadString(config, info_string, token.string)) {
			vrErrPrintf(RED_TEXT "Error, unable to parse configuration string (line %d), skipping '%s'\n",
				parse->linenum, token.string);
		}
	} else {
		vrErrPrintf(RED_TEXT "Error, expected a configuration string (line %d), skipping '%s'\n",
			token.linenum, token.string);
	}
}


/*************************************/
/*** Global option parsing routine ***/
/*************************************/

/************************************************************/
/* Parse all the global options */
/* NOTE: unlike the specific object parsing routines, this function does NOT   */
/*   take a vrObjectInfo argument.  Since these options operate on the general */
/*   configuration of the system.  Note also that unlike the statement parsing */
/*   function, this function DOES take a vrTokenInfo argument, which is passed */
/*   from either _ConfigParseStatement() itself, or from one of the specific   */
/*   object parsers.                                                           */
/* NOTE also: this parsing routine (unlike the specific object parsers) does   */
/*   NOT advance to the next token, thus it will generally return the token    */
/*   that was passed to it, so care must be taken to make sure that advancement*/
/*   is made, or you can end up in an infinite loop (well, a loop that won't   */
/*   end until the config file or string is entirely consumed, producing lots  */
/*   of error messages in the process.                                         */
static vrTokenInfo _ConfigParseGlobalOption(vrTokenInfo token, vrParseInfo *parse_data, vrConfigInfo *config, vrObjectInfo *object, _ConfigParseOptionList objparser)
{
	vrTokenInfo	skip_token;

	vrTrace("_ConfigParseGlobalOption()", RED_TEXT "entering" NORM_TEXT);

	switch (token.token) {
	/**********************/
	/** Error conditions **/
	case VRTOKEN_PARSEERROR:
		vrErrPrintf("ConfigParse: Encountered an error on line %d of %s: '%s'\n", -1,
			"<fill this in>", token.string);
		break;


	/***************************/
	/** Statement Terminators **/
	case VRTOKEN_SEMICOLON:
	case VRTOKEN_CLOSECURL:
		/* just skip them to read the next keyword */
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): skipping '%s' in global option -- empty list?\n", token.linenum, token.string);
		break;

	/*********************/
	/** Statement block **/
	case VRTOKEN_OPENCURL:
		/* recursively handle statements until reaching a closing curly brace */
		vrDbgPrintfN(PARSE_DBGLVL, "ConfigParse: Open curl on line %d, recursively parsing statements.\n", parse_data->linenum);
		do {
			token = _ConfigParseStatement(config, parse_data, object, objparser);
		} while (token.token != VRTOKEN_EOF && token.token != VRTOKEN_EXIT && token.token != VRTOKEN_PARSEERROR && token.token != VRTOKEN_CLOSECURL);
		vrDbgPrintfN(PARSE_DBGLVL, "ConfigParse: Close curl on line %d, closing one level of recursion.\n", parse_data->linenum);
		break;

	/****************************/
	/** Tokens with no effects **/
	case VRTOKEN_NOP:
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "ConfigParse: got **** NOP ****\n");
		break;

	/******************************/
	/** Tokens with side effects **/
	case VRTOKEN_EOF:
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "ConfigParse: got **** EOF ****\n");
		break;

	case VRTOKEN_EXIT:
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "ConfigParse: got **** Exit token ****\n");
		break;

	case VRTOKEN_ABORT:
		vrDbgPrintfN(ALWAYS_DBGLVL, "ConfigParse: got **** Termination ****\n");
		vrExit();
		exit(-2);

	/****************************************/
	/** Tokens for builtin/system commands **/
	case VRTOKEN_EXEC:
		token = vrParseNextToken(parse_data);
		if (token.token == VRTOKEN_STRING) {
			system(token.string);
		} else {
			vrErrPrintf(RED_TEXT "Error, expected string to execute (line %d), skipping '%s'\n",
				token.linenum, token.string);
		}

		/* check that command is terminated */
		token = vrParseNextToken(parse_data);
		if (token.token != VRTOKEN_SEMICOLON) {
			vrErrPrintf("FreeVR config: Error (line %d): "
				RED_TEXT "'Exec' option not terminated with a ';'.\n" NORM_TEXT,
				token.linenum);
		}

		break;

	case VRTOKEN_ECHO:
		vrMsgPrintf("FreeVR Config" BOLD_TEXT ">>" NORM_TEXT);
		token = vrParseNextToken(parse_data);
		while (token.token != VRTOKEN_SEMICOLON && token.token != VRTOKEN_CLOSECURL && token.token != VRTOKEN_EOF) {
#if 0 /* whether or not to put separating whitespace between tokens -- '0' does */
			vrMsgPrintf("%s ", token.string);
#else
			vrMsgPrintf("%s", token.string);
#endif
			token = vrParseNextToken(parse_data);
		}
		vrMsgPrintf("\n");
		break;

	case VRTOKEN_SETENV:
		_ConfigParseSetenv(config, parse_data);
		break;

	case VRTOKEN_UNSETENV:
		_ConfigParseUnsetenv(config, parse_data);
		break;

	case VRTOKEN_PRINTOBJ:
		_ConfigParsePrintobject(config, parse_data);
		break;

	case VRTOKEN_PRINTCONFIG:
		_ConfigParsePrintconfig(config, parse_data);
		break;

	case VRTOKEN_DBXPAUSE:
		_ConfigParseDbxpause(config, parse_data);
		break;

	case VRTOKEN_IF:
		token = _ConfigParseIf(config, parse_data, object, objparser);
		break;

	case VRTOKEN_IFEXEC:
		/* TODO: also should reset the token -- needed for exit */
		_ConfigParseIfexec(config, parse_data, object, objparser);
		break;

	case VRTOKEN_INCLUDE:
	case VRTOKEN_SINCLUDE:
		/* TODO: implement parsing for these tokens */

		vrErrPrintf("ConfigParse (line %d): token \"%s\" not yet implemented.\n",
			token.linenum, token.string);
		token.token = VRTOKEN_UNKNOWN;

		/* read and skip the rest of the statement */
		skip_token = vrParseToEOS(parse_data);
		vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
			skip_token.linenum, skip_token.string);
		break;

	/********************************************************/
	/** Tokens for Setting Global Assignments and defaults **/
	case VRTOKEN_SET:
	case VRTOKEN_SETDEFAULT:
		_ConfigParseAssignments(config, parse_data);
		break;

	/*****************************/
	/** Tokens for main options **/
	case VRTOKEN_USESYSTEM:
		_ConfigParseUsesystem(config, parse_data);
		break;

	case VRTOKEN_SYSTEM:
		_ConfigParseObjectDefinition(config, parse_data,
			VROBJECT_SYSTEM, token.string, (void *)_ConfigParseSystemOption);
		break;

	case VRTOKEN_PROCESS:
		_ConfigParseObjectDefinition(config, parse_data,
			VROBJECT_PROCESS, token.string, (void *)_ConfigParseProcOption);
		break;

	case VRTOKEN_WINDOW:
		_ConfigParseObjectDefinition(config, parse_data,
			VROBJECT_WINDOW, token.string, (void *)_ConfigParseWindowOption);
		break;

	case VRTOKEN_EYELIST:
		_ConfigParseObjectDefinition(config, parse_data,
			VROBJECT_EYELIST, token.string, (void *)_ConfigParseEyelistOption);
		break;

	case VRTOKEN_USER:
		_ConfigParseObjectDefinition(config, parse_data,
			VROBJECT_USER, token.string, (void *)_ConfigParseUserOption);
		break;

	case VRTOKEN_PROP:
		_ConfigParseObjectDefinition(config, parse_data,
			VROBJECT_PROP, token.string, (void *)_ConfigParsePropOption);
		break;

	case VRTOKEN_INPUTDEV:
		_ConfigParseObjectDefinition(config, parse_data,
			VROBJECT_INDEV, token.string, (void *)_ConfigParseIndevOption);
		break;

	case VRTOKEN_INPUTMAP:
		_ConfigParseInmap(config, parse_data);
		break;

	case VRTOKEN_READCONFIG:
		_ConfigParseReadconfig(config, parse_data);
		break;

	case VRTOKEN_READCONFIGSTR:
		_ConfigParseReadconfigstring(config, parse_data);
		break;

	default:
		if (objparser != NULL) {
			/* If we're already in the middle of parsing a specific object type */
			/*   (e.g. because we're in an "if" statement), then we also need   */
			/*   to parse those token possibilities.                            */

			/* NOTE: presumably, the "object" field is also non-NULL */
			//token = (*objparser)(object, token, parse_data, config, objparser);
			token = (*objparser)(object, token, parse_data, config, NULL);	/* BS: 04/03/12 -- this is a test */
		} else {
			token.token = VRTOKEN_UNKNOWN;
			vrDbgPrintfN(CONFIG_ERROR_DBGLVL, "Config Parse Warning (line %d): "
				RED_TEXT "Unknown keyword (\"%s\") -- skipping rest of statement\n" NORM_TEXT,
				token.linenum, token.string);
			skip_token = vrParseToEOS(parse_data);

			vrDbgPrintfN(CONFIG_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				skip_token.linenum, skip_token.string);
		}
		break;
	}

	vrTrace("_ConfigParseGlobalOption()", RED_TEXT "exiting" NORM_TEXT);
	return token;
}



/**********************************************/
/*** MAIN configuration file parse routines ***/
/**********************************************/

/********************************************************************/
static vrTokenInfo _ConfigParseStatement(vrConfigInfo *config, vrParseInfo *config_parse, vrObjectInfo *object, _ConfigParseOptionList objparser)
{
	vrTokenInfo	token;

	/*******************/
	/** get the token **/
	token = vrParseNextToken(config_parse);
	vrDbgPrintfN(CONFIG_DBGLVL, "ConfigParse: (line %d) got token **** (%3d) '%s' ****\n",
		token.linenum, token.token, token.string);

	return (_ConfigParseGlobalOption(token, config_parse, config, object, objparser));
}



/********************************************************************/
static void _ConfigParseMain(vrConfigInfo *config, vrParseInfo *config_parse)
{
	vrTokenInfo	token;

	/* Read every line of the config input, returning when reaching the */
	/*   end, encountering an error, or encountering the config "exit"  */
	/*   {command|operation}.                                           */
	do {
		token = _ConfigParseStatement(config, config_parse, NULL, NULL);
	} while (token.token != VRTOKEN_EOF && token.token != VRTOKEN_EXIT && token.token != VRTOKEN_PARSEERROR);

	vrDbgPrintfN(CONFIG_WARN_DBGLVL, "FreeVR config: Done Parsing \"%s\".  Read %d lines.\n",
		config_parse->parse_name, config_parse->linenum);

	return;
}


/*********************************************/
static int _ConfigReadFile(vrConfigInfo *config, char *path, char *file)
{
	FILE		*fp_configfile;
	char		*fullfilename;
	vrParseInfo	config_parse;

	if (path == NULL) {
		fullfilename = file;
	} else {
		fullfilename = (char *)vrShmemAlloc(strlen(path)+strlen(file)+3);
		sprintf(fullfilename, "%s/%s", path, file);
	}

	fp_configfile = fopen(fullfilename, "r");
	if (fp_configfile == NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "FreeVR config: could not open file \"%s\".\n", fullfilename);
		return 0;
	}
	vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR config: " BOLD_TEXT "Reading file \"%s\".\n" NORM_TEXT, fullfilename);

	/* initialize the parsing info structure for reading a config file */
	config_parse.linenum = 0;
	config_parse.parse_name = vrShmemAlloc(20 + strlen(fullfilename));
	sprintf(config_parse.parse_name, "Config file: '%s'", fullfilename);
	config_parse.filename = vrShmemStrDup(fullfilename);
	config_parse.fp = fp_configfile;
	memset(config_parse.input, '\0', MAX_PARSE_INPUT);
	config_parse.next_input = "";
	config_parse.preparsed = NULL;			/* No tokens in preparse holding bin */
	config_parse.suppress_inline = 0;

	/* we may want to put these four lines at the top of _ConfigParseMain() */
	config_parse.tsmap_list = _ConfigTSMap;
	config_parse.nummaps = (sizeof(_ConfigTSMap) / sizeof(vrTokenInfo));
	config_parse.funcmap_list = _ConfigFuncMap;
	config_parse.numfuncs = (sizeof(_ConfigFuncMap) / sizeof(vrFunctionInfo));

	_ConfigParseMain(config, &config_parse);

	return 1;
}


/*********************************************/
static int _ConfigReadString(vrConfigInfo *config, char *info_string, char *conf_string)
{
	vrParseInfo	config_parse;

	if (strlen(conf_string) > MAX_PARSE_INPUT) {
		vrErrPrintf("_ConfigReadString(): too many input characters in \"%s\" (%d > %d).\n", info_string, strlen(conf_string), MAX_PARSE_INPUT);
		return 0;
	}
	vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR config: " BOLD_TEXT "Reading string \"%s\".\n" NORM_TEXT, info_string);

	config_parse.linenum = 1;
	config_parse.parse_name = vrShmemStrDup("Config string");
	config_parse.filename = vrShmemStrDup(info_string);
	config_parse.fp = NULL;		/* this indicates reading a string. */
#if 0 /* 3/3/3: for some reason the memcpy() call seg-faults on Linux */
	memset(config_parse.input, '\0', MAX_PARSE_INPUT);	/* TODO: this seems unnecessary if we're going to copy in the same number of bytes */
	memcpy(config_parse.input, conf_string, MAX_PARSE_INPUT);
#else
	strncpy(config_parse.input, conf_string, MAX_PARSE_INPUT);
#endif
	config_parse.next_input = config_parse.input;
	config_parse.preparsed = NULL;			/* No tokens in preparse holding bin */
	config_parse.suppress_inline = 0;

	/* we may want to put these four lines at the top of _ConfigParseMain() */
	config_parse.tsmap_list = _ConfigTSMap;
	config_parse.nummaps = (sizeof(_ConfigTSMap) / sizeof(vrTokenInfo));
	config_parse.funcmap_list = _ConfigFuncMap;
	config_parse.numfuncs = (sizeof(_ConfigFuncMap) / sizeof(vrFunctionInfo));

	_ConfigParseMain(config, &config_parse);

	return 1;
}


/*********************************************/
/* _ConfigReadStringList(): reads a list of of strings in argv[], removing */
/*   arguments that are used -- so the application can then parse the   */
/*   rest.  Argc is dereferenced so it can be updated to reflect the    */
/*   number of remaining arguments -- though another way to implement   */
/*   this would simply be to set any used arguments to a zero-length    */
/*   strings.                                                           */
/* A NULL-terminated string-list of unknown length can also be parsed.  */
/*   This is indicated by passing a NULL pointer as argc.  For this,    */
/*   the last argument of argv[] should be NULL,                        */
static void _ConfigReadStringList(vrConfigInfo *config, int *argc, char **argv)
{
	char	info_string[1024];
	int	count;

	if (argv == NULL)
		return;

	if (argc == NULL) {
		for (count = 0; argv[count] != NULL; count++) {
			sprintf(info_string, "Argp[%d] config string", count);
			_ConfigReadString(config, info_string, argv[count]);
		}
	} else {
		for (count = 1; count < *argc; count++) {
			sprintf(info_string, "Argv[%d] config string", count);
			_ConfigReadString(config, info_string, argv[count]);
		}
	}

	return;
}


/*********************************************/
static void _ConfigReadEnvVariable(vrConfigInfo *config, char *name)
{
	char	*env_string;
	char	info_string[1024];

	env_string = getenv(name);
	if (env_string == NULL) {
		vrDbgPrintfN(CONFIG_DBGLVL, "_ConfigReadEnvVariable(): " RED_TEXT "environment variable '%s' does not exist.\n" NORM_TEXT, name);
		return;
	}
	sprintf(info_string, "Envvar[%s] config string", name);
	_ConfigReadString(config, info_string, env_string);

	return;
}


/*********************************************/
static void _DefaultConfig(vrConfigInfo *config)
{
static	char	*def_string =

		/* NOTE: Since FreeVR configuration is not based on particular whitespace,*/
		/*   the use of newlines isn't really necessary.  However, it does serve  */
		/*   to assist in finding which line is the culprit since errors are      */
		/*   reported based on line number -- hence the usage of  newslines here. */
		/*   Of course, the conditionally compiled stuff still causes havoc.      */
		/* NOTE also that there are two systems defined here: "default" and       */
		/*   "simulator".  The default system that is used is "simulator", but    */
		/*   when the existing system isn't found, it will instead default to     */
		/*   "default."                                                           */
		"setDefault EyeList = \"" DEFAULT_EYELIST_NAME "\";\n"
		"setDefault VisrenMode = mono;\n"
		"\n"
		"#set DebugLevel = 49;\n"
		"\n"
		"#setDefault PreContextPrint = \"brief\";\n"
		"#setDefault PostContextPrint = \"brief\";\n"
		"setDefault PostConfigPrint = \"brief\";\n"
		"\n"
		"usesystem \"simulator\";\n"
		"\n"
		"system \"default\" = {\n"
		"	malleable = yes;\n"
		"	type = \"simulator\";\n"
		"	procs = \"" DEFAULT_VISRENPROC_NAME "\", \"" DEFAULT_INPUTPROC_NAME "\";\n"
		"	inputmap = \"default\";\n"
		"	visrenmode = \"default\";\n"
		"}\n"
		"\n"
		"system \"simulator\" = {\n"
		"	malleable = yes;\n"
		"	type = \"simulator\";\n"
		"	procs = \"" DEFAULT_VISRENPROC_NAME "\", \"simulator-input\";\n"
		"	inputmap = \"default\";\n"
		"	visrenmode = \"default\";\n"
		"}\n"
		"\n"
		"process \"" DEFAULT_INPUTPROC_NAME "\" = {\n"
		"	malleable = no;\n"
		"	type = input;\n"
		"	usecmin = 15000;\n"
		"	printcolor = 35;\n"
		"	objects = \"" DEFAULT_INDEV_NAME "\";\n"
		"}\n"
		""
		"process \"simulator-input\" = {\n"
		"	malleable = yes;\n"		/* s/b "no" set to "yes" for testing of barriers */
		"	type = input;\n"
		"	usecmin = 15000;\n"
		"	printcolor = 35;\n"
		"	objects = \"simulator-indev\";\n"
#if defined(WIN_WGL)
		/* TODO: Until the Win32 device has 6-sensors, we also need to include the static device */
		"	objects += \"" DEFAULT_INDEV_NAME "\";\n"
#endif
		"}\n"
		"\n"
		"process \"" DEFAULT_VISRENPROC_NAME "\" = {\n"
		"	malleable = yes;\n"	/* [6/11/02] -- made malleable for testing, TODO: should it remain malleable? */
		"	type = visren;\n"
		"	sync = 1;\n"
		"	printcolor = 34;\n"
		"	objects = \"" DEFAULT_WINDOW_NAME "\";\n"
		"}\n"
		"\n"
		"process \"" DEFAULT_TELNETPROC_NAME "\" = {\n"
		"	malleable = yes;\n"
		"	type = telnet;\n"
		"	usecmin = 5000;\n"
		"	printcolor = 32;\n"
		"	args = \"port = 3000;\";\n"
		"	args += \"portrange = 0;\";\n"
		"	#objects = \"??\";	# no use for this (yet)\n"
		"}\n"
		"\n"
		"eyelist \"" DEFAULT_EYELIST_NAME "\" = {\n"
		"	malleable = no;\n"
		"	monofb = \"" DEFAULT_USER_NAME ":cyclops\";\n"
		"	leftfb = \"" DEFAULT_USER_NAME ":lefteye\";\n"
		"	rightfb = \"" DEFAULT_USER_NAME ":righteye\";\n"
		"	leftvp = \"" DEFAULT_USER_NAME ":lefteye\";\n"
		"	rightvp = \"" DEFAULT_USER_NAME ":righteye\";\n"
		"	anaglfb = \"" DEFAULT_USER_NAME ":lefteye:blue\", \"" DEFAULT_USER_NAME ":righteye:red\";\n"
		"}\n"
		"\n"
		"window \"" DEFAULT_WINDOW_NAME "\" = {\n"
		"	malleable = yes;\n"	/* unlike most of the built-in defaults, this one is malleable */
		"	GraphicsType = \"glx\";\n"
		"	mount = \"simulator\";\n"
	/* TODO: the default geometry should probably be "undetermined" allowing */ "\n"
	/*   the app-user to drag out the window.                                */ "\n"
		"	args = \"geometry=350x350+400+400; decoration=window\";\n"
		"	visrenmode = \"default\";\n"
		"	statsProcs = \"self, main, simulator-input\";\n"	/* NOTE: this generates a warning when the "default" system is used (ie. rather than "simulator"). */
		"}\n"
		"\n"
		/* TODO: still need to figure props out */ "\n"
		"prop \"" DEFAULT_PROP_NAME "\" = {\n"
#if 0
		"	malleable = no;\n"
#endif
"\n\n\n\n"
		"}\n"
		"\n"
		"user \"" DEFAULT_USER_NAME "\" = {\n"
		"	malleable = yes;\n"
		"	iod = 0.23;\n"
		"	color = .5647, .3254, .1764;\n"
		"}\n"
		"\n"
		"inputdevice \"" DEFAULT_INDEV_NAME "\" = {\n"
		"	type = \"static\";\n"
		"	args = \"y=5\";\n"
		"	input \"2switch[-]\" = \"2switch(constant[0])\";\n"
		"	input \"2switch[1]\" = \"2switch(constant[0])\";\n"
		"	input \"2switch[2]\" = \"2switch(constant[0])\";\n"
		"	input \"2switch[3]\" = \"2switch(constant[0])\";\n"
		"	input \"valuator[x]\" = \"valuator(constant[0.0])\";\n"
		"	input \"valuator[y]\" = \"valuator(constant[0.0])\";\n"
		"	input \"6sensor[0]\" = \"6sensor(constant[])\";\n"
		"	input \"6sensor[1]\" = \"6sensor(constant[id])\";\n"
		"}\n"
		"\n"
#if defined(WIN_GLX) /* {  { */
		/* Define an X11 input device for GLX windowing systems */ "\n\n"
		"inputdevice \"simulator-indev\" = {\n"
		"	type = \"xwindows\";\n"
		"	args = \"window = " DEFAULT_WINDOW_NAME ";\";\n"
		"	input \"Escape Key\" = \"2switch(keyboard:key[Escape])\";\n"
		"	input \"Left Mouse Button\" = \"2switch(mouse:button[left])\";\n"
		"	input \"Middle Mouse Button\" = \"2switch(mouse:button[middle])\";\n"
		"	input \"Right Mouse Button\" = \"2switch(mouse:button[right])\";\n"
		"	input \"Mouse Left-Right\" = \"valuator(pointer[x])\";\n"
		"	input \"Mouse Up-Down\" = \"valuator(pointer[-y])\";\n"
		"	input \"Simulated Head\" = \"6sensor(sim6[0])\";\n"
		"	input \"Simulated Wand\" = \"6sensor(sim6[1])\";\n"
		"\n"
		"	control \"system_pause_toggle\" = \"2switch(keyboard:key[pause])\";\n"
		"	control \"print_struct\" = \"2switch(keyboard:key[print])\";\n"
		"	control \"print_help\" = \"2switch(keyboard:key[?])\";	# ie. '/'\n"
		"\n"
		"	control \"sensor_next\" = \"2switch(keyboard:key[n])\";\n"	/* fixed 03/17/11 */
		"	control \"setsensor(0)\" = \"2switch(keyboard:key[s])\";\n"
		"	control \"setsensor(1)\" = \"2switch(keyboard:key[w])\";\n"
		"	control \"sensor_reset\" = \"2switch(keyboard:key[r])\";\n"
		"	control \"sensor_resetall\" = \"2switch(keyboard:key[8])\";\n"
		"	control \"sensor_left\" = \"2switch(keyboard:key[leftarrow])\";\n"
		"	control \"sensor_right\" = \"2switch(keyboard:key[rightarrow])\";\n"
		"	control \"sensor_in\" = \"2switch(keyboard:key[uparrow])\";\n"
		"	control \"sensor_out\" = \"2switch(keyboard:key[downarrow])\";\n"
		"	control \"sensor_cw\" = \"2switch(keyboard:key[.])\";\n"
		"	control \"sensor_ccw\" = \"2switch(keyboard:key[,])\";\n"
		"	control \"sensor_nazim\" = \"2switch(keyboard:key[h])\";\n"
		"	control \"sensor_nelev\" = \"2switch(keyboard:key[j])\";\n"
		"	control \"sensor_pelev\" = \"2switch(keyboard:key[k])\";\n"
		"	control \"sensor_pazim\" = \"2switch(keyboard:key[l])\";\n"
		"	control \"sensor_up\" = \"2switch(keyboard:key[f])\";\n"
		"	control \"sensor_down\" = \"2switch(keyboard:key[v])\";\n"
		"	control \"sensor_swap_upin\" = \"2switch(keyboard:key[leftshift])\";\n"
		"	control \"sensor_rotate_sensor\" = \"2switch(keyboard:key[leftalt])\";\n"
		"	control \"pointer_valuator\" = \"2switch(keyboard:key[space])\";\n"
		"	control \"pointer_rot_override\" = \"2switch(keyboard:key[tab])\";\n"
		"	control \"pointer_xy_override\" = \"2switch(keyboard:key[q])\";\n"
		"	control \"pointer_xz_override\" = \"2switch(keyboard:key[a])\";\n"
		"	control \"toggle_relative\" = \"2switch(keyboard:key[1])\";\n"
		"	control \"toggle_space_limit\" = \"2switch(keyboard:key[2])\";\n"
		"	control \"toggle_return_to_zero\" = \"2switch(keyboard:key[3])\";\n"
		"}\n"
		"\n"
#elif defined(WIN_WGL) /* } { */
		/* Define a Win32 input device for WGL windowing systems */ "\n\n"
		"inputdevice \"simulator-indev\" = {\n"
		"	type = \"win32\";\n"
		"	args = \"window = default;\";\n"
		"	t2rw_translate = 2.0, 4.0, 0.0;\n"
		"	args += \"y=5; x=-2; z=0; azim = 180; elev = 30; roll = 20;\";\n"
		"\n"
		"	input \"2switch[ESC]\" = \"2switch(keyboard:key[Escape])\";\n"
		"	input \"2switch[left]\" = \"2switch(mouse:button[left])\";\n"
		"	input \"2switch[middle]\" = \"2switch(mouse:button[middle])\";\n"
		"	input \"2switch[right]\" = \"2switch(mouse:button[right])\";\n"
		"\n"
		"	input \"valuator[0]\" = \"valuator(pointer[x])\";\n"
		"	input \"valuator[1]\" = \"valuator(pointer[-y])\";\n"
		"\n"
		"	control \"print_struct\" = \"2switch(keyboard:key[print])\";\n"
		"}\n"
#else /* } { */
		/* Define a static input device for unknown windowing systems */ "\n\n"
		"inputdevice \"simulator-indev\" = {\n"
		"	type = \"static\";\n"
		"	args = \"y=5\";\n"
		"	input \"2switch[-]\" = \"2switch(constant[0])\";\n"
		"	input \"2switch[1]\" = \"2switch(constant[0])\";\n"
		"	input \"2switch[2]\" = \"2switch(constant[0])\";\n"
		"	input \"2switch[3]\" = \"2switch(constant[0])\";\n"
		"	input \"valuator[x]\" = \"valuator(constant[0.0])\";\n"
		"	input \"valuator[y]\" = \"valuator(constant[0.0])\";\n"
		"	input \"6sensor[head]\" = \"6sensor(constant[])\";\n"
		"	input \"6sensor[wand]\" = \"6sensor(constant[id])\";\n"
		"}\n"
#endif /* } } */
		"\n";

	vrRemoveAllObjects(config->context);

	_ConfigReadString(config, "Default Config String", def_string);

	return;
}


/*********************************************/
/* _SystemConfig(): fills in the these fields of vrConfigInfo: */
/*   num_procs, num_windows, num_input_devices, procs[...],    */
/*   (and maybe num_users, num_props).                         */
static void _SystemConfig(vrConfigInfo *config)
{
	int		proccount;
	vrSystemInfo	*systemToUse = NULL;
	vrProcessInfo	*this_proc = NULL;
	char		**orig_proc_names;

	vrTrace("_SystemConfig", "ENTERING");

	/* First, state that nothing has been initialized */
	config->system_init = 0;
	config->procs_init = 0;
	config->windows_init = 0;	/* TODO: currently never set to 1 */
	config->users_init = 0;
	config->props_init = 0;		/* TODO: currently never set to 1 */
	config->inputs_init = 0;

	/* TODO: systemToUse should come from doing a search of the list */
	/*   of system comparing the use option (config->system).        */
	/*   10/8/99 -- isn't that what the following line does?  Perhaps */
	/*   this TODO comment outlasted its need.  I need to find a way  */
	/*   to show what RCS revision each line of the code comes from.  */
	systemToUse = vrObjectSearch(config->context, VROBJECT_SYSTEM, config->system_name);
	if (systemToUse == NULL) {
		/* if no matching name is found, use the default -- the first on the list */
		vrErrPrintf(RED_TEXT "Warning, could not find system \"%s\", using default (with boring input).\n" NORM_TEXT,
			config->system_name);
		vrSleep(5000000);	/* sleep long enough for user to see the warning */
		systemToUse = (vrSystemInfo *)vrObjectFirst(config->context, VROBJECT_SYSTEM);
		if (systemToUse == NULL) {
			/* if still no system to use, there's a problem with the library */
			vrErrPrintf(RED_TEXT "LIBRARY ERROR: No default system available.\n" NORM_TEXT);
			vrExit();
			exit(-1);
		}
		config->system_name = systemToUse->name;
	}
	config->system = systemToUse;


	/******************************************************/
	/* Initialize all the vrConfigInfo process structures */

	/* add a process to the count for the main (parent) process */
	systemToUse->num_procs++;

	/* add the main process to the system list */
	orig_proc_names = systemToUse->proc_names;

	/* TODO: Filling in the systemToUse structure seems somewhat redundant. */
	systemToUse->proc_names = (char **)vrShmemAlloc0((systemToUse->num_procs) * sizeof(char *));
	systemToUse->procs = (vrProcessInfo **)vrShmemAlloc0((systemToUse->num_procs) * sizeof(vrProcessInfo *));

	systemToUse->proc_names[0] = vrThisProc->name;
	systemToUse->procs[0] = vrThisProc;

	config->procs = systemToUse->procs;
	config->num_procs = 1;			/* the main process is already setup */
	config->num_windows = 0;
	config->num_eyelists = 0;
	config->num_input_devices = 0;

	vrTrace("_SystemConfig", "about to loop though the procs the first time");

	/* process #0 is handled as a special case -- the main(parent) process. */
	for (proccount = 0; proccount < systemToUse->num_procs - 1; proccount++) {
		/*** find the process info in the config list, and copy ***/
		/***   it to the vrConfigInfo & the System structure.   ***/
		this_proc = vrObjectSearch(config->context, VROBJECT_PROCESS, orig_proc_names[proccount]);
		if (this_proc == NULL) {
			vrErrPrintf("_SystemConfig(): "
				RED_TEXT "ERROR: Can't find process \"%s\" -- check config!\n"
					"\t-> System will continue with diminished functionality.\n" NORM_TEXT,
				orig_proc_names[proccount]);

			config->context->startup_error |= VRSTARTUP_BADPROC;

			/* define a dummy ("error") process object */
			this_proc = (vrProcessInfo *)vrShmemAlloc0(sizeof(vrProcessInfo));
			vrProcessClear(this_proc);
			this_proc->type = VRPROC_NOCONFIG;
			this_proc->name = vrShmemStrDup(orig_proc_names[proccount]);
			this_proc->proc_done = 1;	/* this process won't be started, so it's already done */
		}

		/*** if the process has already been initialized, then this is a duplicate  ***/
		/***   execution, so create a copy of the process object and work from that ***/
		if (this_proc->used) {
			vrProcessInfo	*new_proc = vrObjectNew(config->context, VROBJECT_PROCESS, "-");
			vrObjectCopy(config->context, new_proc, this_proc);
			new_proc->used = 0;
			new_proc->name = vrShmemStrCat(this_proc->name, " dup");
			strncpy(new_proc->file_lastmod, "Created on the fly in _SystemConfig()", sizeof(new_proc->file_lastmod));
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "FreeVR Config: duplicate process given, creating copy: '%s'\n", new_proc->name);
			this_proc->used++;		/* the proc with the original name will have a count of how many copies were begun from it */

			this_proc = new_proc;
		}

		this_proc->used = 1;
		systemToUse->procs[proccount+1] = this_proc;
		systemToUse->proc_names[proccount+1] = this_proc->name;


		/*** add up the number of windows, input_devices, ... ***/
		switch (this_proc->type) {
		case VRPROC_VISREN:
			config->num_windows += this_proc->num_thing_names;
			break;
		case VRPROC_INPUT:
			config->num_input_devices += this_proc->num_thing_names;
			break;
		default: /* VRPROC_PARENT, VRPROC_MAIN, VRPROC_NONE, ... */
			break;
		}

		/* increment the count of the number of configured processes */
		config->num_procs++;
	}

	/* verify that we've added to the configuration the correct number of processes */
	if (config->num_procs != systemToUse->num_procs) {
		/* something is wrong -- I think */
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "FreeVR Config: The number of configured processes (%d) does not match the number in the system specification (%d)\n", config->num_procs, systemToUse->num_procs);
	}

	/* Initialize the Input Mapping */
	config->input_map_name = systemToUse->input_map_name;

	vrTrace("_SystemConfig", "EXITING");
}


/*********************************************/
/* _ProcsConfig(): fills in the these fields of vrConfigInfo: */
/*   windows[...], input_devices[...],                        */
static void _ProcsConfig(vrConfigInfo *config)
{
	int		proccount;
	int		maincount = 0;			/* number of main processes encountered */
	int		thingcount;			/* for looping through thing names (some may be invalid) */
	int		next_thing;			/* indicates the next available thing for the process */
	int		window_num = 0;
	int		indev_num = 0;
	vrProcessInfo	*this_proc = NULL;
	vrWindowInfo	*this_window = NULL;
	vrInputDevice	*this_indev = NULL;

	vrTrace("_ProcsConfig", "ENTERING");

	if (config->num_procs == 0) {
		vrErrPrintf("_ProcsConfig(): " RED_TEXT "Hmmm, no processes in the configuration.\n" NORM_TEXT);
		config->context->startup_error |= VRSTARTUP_NOPROCS;
		return;
	}

	config->windows = (vrWindowInfo **)vrShmemAlloc0((config->num_windows) * sizeof(vrWindowInfo *));
	config->input_devices = (vrInputDevice **)vrShmemAlloc0(config->num_input_devices * sizeof(vrInputDevice *));

	for (proccount = 1; proccount < config->num_procs; proccount++) {
		this_proc = config->procs[proccount];

		if (this_proc == NULL)  /* this happens when the wrong process name is given in the config */
			continue;	/* skip to the next process in the list */

		/* Assign an id number to this process.  NOTE: one is added to the */
		/*   process' position in the array in order to make it a 1-based  */
		/*   number, and have the value of 0 indicate the process is not   */
		/*   part of the system in use.                                    */
		/* Need to subtract one for each main-process type defined because */
		/*   they all get copied to be process[0] anyway.                  */
		this_proc->num = proccount + 1 - maincount;

		/*************************************************************/
		/* Append the things of this process to the appropriate list */
		if (this_proc->num_thing_names > 0)
			this_proc->things = (void *)vrShmemAlloc0(this_proc->num_thing_names * sizeof(void *));

		if (vrDbgDo(CONFIG_DBGLVL)) {
			vrMsgPrintf("Doing process config for ");
			vrFprintProcessInfo(stdout, this_proc, verbose);
		}

		switch (this_proc->type) {

		    /***********/
		case VRPROC_MAIN:
			/* If there is a "main" process defined, then it's values */
			/*   should override the builtin default values.          */

			this_proc->pid = config->procs[0]->pid;
			vrObjectCopy(config->context, config->procs[0], this_proc);

			/* Need to copy some additional details */
			config->procs[0]->name = this_proc->name;
			config->procs[0]->id = this_proc->id;
			strcpy(config->procs[0]->file_created, this_proc->file_created);
			config->procs[0]->line_created = this_proc->line_created;
			strcpy(config->procs[0]->file_lastmod, this_proc->file_lastmod);
			config->procs[0]->line_lastmod = this_proc->line_lastmod;

			config->procs[0]->num = 0;
			maincount++;
			if (maincount > 1) {
				/* It doesn't make sense to have more than one main process defined */
				vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "FreeVR: Warning, more than one \"main\" process type in the configuration!\n");
			}

			/* Now shift all the other process objects down one, done */
			/*   to get rid of the duplicate main process object.     */
			config->num_procs--;
			for (thingcount = proccount; thingcount < config->num_procs; thingcount++) {
				config->procs[thingcount] = config->procs[thingcount+1];
			}
			proccount--;
			break;

		    /************/
		case VRPROC_INPUT:
			for (next_thing = 0, thingcount = 0; thingcount < this_proc->num_thing_names; thingcount++) {
				this_indev = vrObjectSearch(config->context, VROBJECT_INDEV, this_proc->thing_names[thingcount]);
				if (this_indev == NULL) {
					vrErrPrintf("_ProcsConfig(): " RED_TEXT "ERROR: Can't find input device '%s'\n"
						"\t-> Perhaps it is misspelled in the configuration file.\n" NORM_TEXT,
						this_proc->thing_names[thingcount]);
					config->context->startup_error |= VRSTARTUP_BADINDEV;
					config->num_input_devices--;
				} else {
					config->input_devices[indev_num] = this_indev;
					this_proc->things[next_thing] = (void *)this_indev;
					if (this_indev->proc != NULL) {
						/* print a warning if this indev has already been assigned */
						vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_ProcsConfig(): "
							RED_TEXT "Warning, input-device '%s' has already been assigned to process '%s', will reassign.\n" NORM_TEXT,
							this_indev->name,
							this_indev->proc->name);
					}
					this_indev->proc = this_proc;
					indev_num++;
					next_thing++;
				}
			}
			this_proc->num_things = next_thing;	/* set num_things to number of valid things found */

			break;

		    /*************/
		case VRPROC_VISREN:
			for (next_thing = 0, thingcount = 0; thingcount < this_proc->num_thing_names; thingcount++) {
				this_window = vrObjectSearch(config->context, VROBJECT_WINDOW, this_proc->thing_names[thingcount]);
				if (this_window == NULL) {
					vrErrPrintf("_ProcsConfig(): " RED_TEXT "ERROR: Can't find window '%s'."
						"\t-> Perhaps it is misspelled in the configuration file.\n" NORM_TEXT,
						this_proc->thing_names[thingcount]);
					config->context->startup_error |= VRSTARTUP_BADWINDOW;
					config->num_windows--;
				} else {
					config->windows[window_num] = this_window;
					this_proc->things[next_thing] = (void *)this_window;
					if (this_window->proc != NULL) {
						/* print a warning if this window has already been assigned */
						vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_ProcsConfig(): "
							RED_TEXT "Warning, window '%s' has already been assigned to process '%s', will reassign.\n" NORM_TEXT,
							this_window->name,
							this_window->proc->name);
					}
					this_window->proc = this_proc;
					window_num++;
					next_thing++;
				}
			}
			this_proc->num_things = next_thing;	/* set num_things to number of valid things found */

			break;
		default: /* VRPROC_COMPUTE, VRPROC_AUDREN, VRPROC_NONE, ... */
			break;
		}
		if (vrDbgDo(CONFIG_DBGLVL)) {
			vrMsgPrintf("Finished process config for ");
			vrFprintProcessInfo(stdout, this_proc, verbose);
		}
	}

	vrTrace("_ProcsConfig", "EXITING");
}


/*********************************************/
/* _WindowsConfig(): fills in these fields of ? : */
/*   TODO: figure out whether this function is necessary.  */
/*   perhaps it's only needed to fill in user information. */
static void _WindowsConfig(vrConfigInfo *config)
{
static	char		*whitespace = " \t\r\b";
	int		count;
	int		windowcount;
	vrWindowInfo	*this_window;
	vrVisrenModeType visrenmode;
	char		*this_eyelist_name;
	vrEyelistInfo	*this_eyelist;
	vrEyelistInfo	*add_eyelist;
	vrEyeInfo	*this_eye;
	vrEyeInfo	*first_eye;
	vrEyeInfo	*next_eye;
	int		max_num_eyes = 20;	/* TODO: is their a more accurate way to calc? */

	char		*list_dup;		/* duplicate copy of process list for destructive analysis */
	char		*process_name;		/* string with single process name from list of names */
	char		*end_name;		/* pointer to the end of the name string */
	char		*next;			/* pointer to the next part of the list to parse */
	int		stats_count;		/* the number of processes found in the stats list so far */
	vrProcessInfo	*process;		/* the process associated with the given name */

	vrTrace("_WindowsConfig", "ENTERING");

	if (config->num_windows == 0) {
		vrErrPrintf("Hmmm, no windows in the configuration.\n");
		return;
	}

	/* Allocate the memory for all the possible eyelists used in the config.          */
	/*   There cannot be more eyelists than windows (and there probably are less,     */
	/*   but we just allocate the memory for the maximum number possible.  This is    */
	/*   slightly wasteful, but not worth the trouble of being more memory efficient. */
	config->eyelists = (vrEyelistInfo **)vrShmemAlloc0(config->num_windows * sizeof(vrEyelistInfo *));
	config->num_eyelists = 0;

	config->eyes = (vrEyeInfo **)vrShmemAlloc0(max_num_eyes * sizeof(vrEyeInfo *));
	config->num_eyes = 0;

	/* Add each distinct eyelist to the list of eyelists in vrConfigInfo. */
	for (windowcount = 0; windowcount < config->num_windows; windowcount++) {
		this_window = config->windows[windowcount];

		/* Assign an id number to this window.  NOTE: one is added to the */
		/*   window's position in the array in order to make it a 1-based */
		/*   number, and have the value of 0 indicate the window is not   */
		/*   part of the system in use.                                   */
		this_window->num = windowcount + 1;

		/*****************************************/
		/* determine the eyelist for this window */
		this_eyelist_name = this_window->settings.eyelist_name;
		if (this_eyelist_name == NULL)
			this_eyelist_name = config->system->settings.eyelist_name;
		if (this_eyelist_name == NULL)
			this_eyelist_name = config->defaults.eyelist_name;
		if (this_eyelist_name == NULL) {
			vrMsgPrintf("_WindowsConfig(): "
				RED_TEXT "No Eyelist assigned, using builtin default.\n" NORM_TEXT);
			this_eyelist_name = "default";
		}
		this_eyelist = vrObjectSearch(config->context, VROBJECT_EYELIST, this_eyelist_name);
		this_window->eyelist = this_eyelist;

		if (this_eyelist == NULL) {
			/* Without an eyelist, something is seriously amiss, but don't want to crash */
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "FreeVR: Warning, there is no eyelist!\n");
			continue;
		}

		/************************************************************************/
		/* Add eyelist to list of configed eyelists if not on the list already. */
		/*   NOTE: this is different than for Procs & Windows because */
		/*   the same user can (and is likely) to be handled by more  */
		/*   than one window.                                         */
		add_eyelist = this_eyelist;
		for (count = 0; count < config->num_eyelists; count++) {
			if (add_eyelist == config->eyelists[count])
				add_eyelist = NULL;
		}
		if (add_eyelist != NULL) {
			vrDbgPrintfN(CONFIG_DBGLVL, "_WindowsConfig(): "
				BOLD_TEXT "Adding new eyelist [%#p \"%s\"] to config\n" NORM_TEXT,
				add_eyelist, add_eyelist->name);
			config->eyelists[config->num_eyelists] = add_eyelist;
			config->num_eyelists++;

			/* Parse the eyelist strings into Eye objects */
			vrEyelistInitialize(config->context, add_eyelist);
		}

		/* determine the visrenmode for this window */
		visrenmode = this_window->settings.visrenmode;
		if (visrenmode == VRVISREN_NONE_SELECTED)
			visrenmode = config->system->settings.visrenmode;
		if (visrenmode == VRVISREN_NONE_SELECTED)
			visrenmode = config->defaults.visrenmode;
		if (visrenmode == VRVISREN_NONE_SELECTED) {
			visrenmode = VRVISREN_MONO;
			vrDbgPrintfN(AALWAYS_DBGLVL, "_WindowsConfig(): "
				RED_TEXT "No Visual Rendering Mode selected for window \"%s,\" using Mono mode.\n" NORM_TEXT, this_window->name);
		}
#if 0 /* VRVISREN_STEREO is now (08/20/10) deprecated */
/* NOTE: [08/20/10] Okay, I think this fragment of code is of little value.  Despite my earlier   */
/*   complaint, as expressed in the vrPrintf(): statement below, we can't know whether there is a */
/*   quad-buffer availble for stereo rendering until we begin the process of opening the window.  */
/*   Therefore, we can't know this information yet.  I think the best solution is to simply not   */
/*   allow for this ill-defined "stereo" visrenmode that is suppose to come up with the "best"    */
/*   stereo solution.  We should just force the configuror to do their job.                       */
vrPrintf("_WindowsConfig(): "RED_TEXT "HEY, about to use dualeye_buffer! (%d) -- this is where we should disable any visrenmode that assumes a stereo buffer if we didn't get one!\n", this_window->dualeye_buffer);
		if (visrenmode == VRVISREN_STEREO) {
			if (this_window->dualeye_buffer)
				visrenmode = VRVISREN_DUALFB;
			else	visrenmode = VRVISREN_DUALVP;
		}
#endif

		/* set the window's field to the given mode (so later queries will know) */
		this_window->visrenmode = visrenmode;

/* TODO: 05/02/2006 -- this section of code (setting the first_eye) may need to come after the call to _GlxOpenFunc() */
		/* Choose the particular list based on visrenmode options */
		switch (visrenmode) {
		default:  /* TODO: make sure this always works -- might monofb be NULL? */
		case VRVISREN_MONO:
			first_eye = this_eyelist->monofb;
			break;
		case VRVISREN_LEFT:
			first_eye = this_eyelist->leftvp;
			break;
		case VRVISREN_RIGHT:
			first_eye = this_eyelist->rightvp;
			break;
		case VRVISREN_DUALFB:
			first_eye = this_eyelist->leftfb;
			break;
		case VRVISREN_DUALVP:
			first_eye = this_eyelist->leftvp;
			break;
		case VRVISREN_ANAGLYPHIC:
		case VRVISREN_CHECKERBOARD:		/* this is probably temporary, since checkerboard currently overrides the analgyphic mode -- but won't forever (I hope) */
		case VRVISREN_VIBRATE:			/* TODO: I don't know where this goes yet! */
			first_eye = this_eyelist->anaglfb;
			break;
		}

		if (first_eye == NULL) {
			/* Something has gone wrong -- why would this happen? */
			first_eye = this_eyelist->monofb;
			vrDbgPrintfN(AALWAYS_DBGLVL, "_WindowsConfig(): "
				RED_TEXT "Problem encountered attempting to set the stereo mode \"%s\" (%d) in window \"%s,\" will resort to monoscopic rendering.\n" NORM_TEXT,
				vrVisrenmodeTypeName(visrenmode), visrenmode, this_window->name);
		}
		vrDbgPrintfN(CONFIG_DBGLVL, "_WindowsConfig(): "
			BOLD_TEXT "visrenmode for Window \"%s\" is %s, with first eye \"%s\"\n" NORM_TEXT,
			this_window->name, vrVisrenModeName(this_window->visrenmode), first_eye->name);

		/****************************************************************************/
		/* Now add each individual eye to the list of eyes in the window and config */

		this_window->eyes = (vrEyeInfo **)vrShmemAlloc0(max_num_eyes * sizeof(vrEyeInfo *));
		this_window->num_eyes = 0;

		for (next_eye = first_eye; next_eye != NULL; next_eye = next_eye->next) {
			this_eye = next_eye;
			for (count = 0; count < this_window->num_eyes; count++) {
				if (this_eye == this_window->eyes[count])
					this_eye = NULL;
			}
			if (this_eye != NULL) {
				/* this_eye is not yet in the windows list. */
				/*  (only rarely should an eye already be on the window's list) */
				vrDbgPrintfN(CONFIG_DBGLVL, "_WindowsConfig(): "
					BOLD_TEXT "Adding new eye [%#p \"%s\"] to window \"%s\"\n" NORM_TEXT,
					this_eye, this_eye->name, this_window->name);
				this_window->eyes[this_window->num_eyes] = this_eye;
				this_window->num_eyes++;
			}
			this_eye = next_eye;
			for (count = 0; count < config->num_eyes; count++) {
				if (this_eye == config->eyes[count])
					this_eye = NULL;
			}
			if (this_eye != NULL) {
				vrDbgPrintfN(CONFIG_DBGLVL, "_WindowsConfig(): "
					BOLD_TEXT "Adding new eye [%#p \"%s\"] to config\n" NORM_TEXT, this_eye, this_eye->name);
				config->eyes[config->num_eyes] = this_eye;
				config->num_eyes++;
			}
		}

		/* NOTE: I'm sure this could be written a little cleaner, */
		/*   rather than doing the same stuff twice.              */

		/* Choose a second list based on visrenmode options */
		switch (visrenmode) {
		default:
		case VRVISREN_MONO:
		case VRVISREN_LEFT:
		case VRVISREN_RIGHT:
		case VRVISREN_ANAGLYPHIC:
			first_eye = NULL;	/* only one list */
			break;
		case VRVISREN_DUALFB:
			first_eye = this_eyelist->rightfb;
			break;
		case VRVISREN_DUALVP:
			first_eye = this_eyelist->rightvp;
			break;
		}

		/* Now add each individual eye to the list of eyes in the window and configuration */
		for (next_eye = first_eye; next_eye != NULL; next_eye = next_eye->next) {
			this_eye = next_eye;
			for (count = 0; count < this_window->num_eyes; count++) {
				if (this_eye == this_window->eyes[count])
					this_eye = NULL;
			}
			if (this_eye != NULL) {
				/* this_eye is not yet in the windows list. */
				/*  (only rarely should an eye already be on the window's list) */
				vrDbgPrintfN(CONFIG_DBGLVL, "_WindowsConfig(): "
					BOLD_TEXT "Adding new eye [%#p \"%s\"] to window \"%s\"\n" NORM_TEXT,
					this_eye, this_eye->name, this_window->name);
				this_window->eyes[this_window->num_eyes] = this_eye;
				this_window->num_eyes++;
			}
			this_eye = next_eye;
			for (count = 0; count < config->num_eyes; count++) {
				if (this_eye == config->eyes[count])
					this_eye = NULL;
			}
			if (this_eye != NULL) {
				vrDbgPrintfN(CONFIG_DBGLVL, "_WindowsConfig(): "
					BOLD_TEXT "Adding new eye [%#p \"%s\"] to window \"%s\"\n" NORM_TEXT,
					this_eye, this_eye->name, this_window->name);
				config->eyes[config->num_eyes] = this_eye;
				config->num_eyes++;
			}
		}

		if (this_window->num_eyes == 0) {
			vrErrPrintf("_WindowsConfig(): "
				RED_TEXT "Window \"%s\" has no eyes associated with it\n" NORM_TEXT, this_window->name);
		}


		/**********************************************************/
		/* Setup the array of pointers to process statistics data */
		stats_count = 0;
		if (this_window->stats_procs != NULL) {
			list_dup = vrShmemStrDup(this_window->stats_procs);
			process_name = list_dup;
			do {
				process_name += strspn(process_name, whitespace);	/* skip white */
				end_name = strchr(process_name, ',');
				if (end_name != NULL) {
					next = end_name+1;
					end_name[0] = '\0';
				}

				/* If the process_name is an empty string, then move on */
				if (process_name[0] == '\0') {
					process_name = next;
					continue;
				}

				/* Compare name with special-purpose names */
				if (!strcasecmp(process_name, "self")) {
					process = this_window->proc;
				} else if (!strcasecmp(process_name, "main")) {
					process = config->procs[0];
				} else if (!strcasecmp(process_name, "sim")) {
					process = config->procs[0];

				} else if (0) {
					/* TODO: we may want to have the ability to get the Nth process */
					/*   of a certain type -- eg: "input[0], or visren[1]".         */

				} else {
					/* Find the existing process from name and set the stats pointer */
					process = vrObjectArraySearch(VROBJECT_PROCESS, (vrObjectInfo **)(config->procs), config->num_procs, process_name);
				}

				if (process != NULL) {
					this_window->stats[stats_count] = &(process->stats);
					stats_count++;

					/* Make sure the stats are enabled for the process */
					if (process->stats_args == NULL)
						process->stats_args = vrShmemStrDup("");
				} else {
					vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Configuration initialization Warning: "
						RED_TEXT "Window '%s' -- no process '%s' to show statistics of.\n" NORM_TEXT,
						this_window->name, process_name);
				}

				/* get ready to parse the next process name in the list */
				process_name = next;
			} while (end_name != NULL && stats_count < VR_MAXSTATS);
		}

		/* If no process statistics were specifically listed for this window then add "self" */
		if (stats_count == 0) {
			vrDbgPrintf("_WindowsConfig(): Window %s had no associated stats processes, so defaulted to 'self'\n", this_window->name);
			process = this_window->proc;
			this_window->stats[stats_count] = &(process->stats);
			stats_count++;

			/* Make sure the stats are enabled for the process */
			if (process->stats_args == NULL)
				process->stats_args = vrShmemStrDup("");
		}


		/*************************************************************/
		/* Fill in the rest of the windowInfo structure with default */
		/*    values, and create/set graphics handling callbacks.    */
		vrVisrenGetInfo(this_window);
	}

	vrTrace("_WindowsConfig", "Calling PreOpenInit");
	/* NOTE: this assumes that all the windows require the same pre-window */
	/*   opening initialization as window #0.                              */
	vrCallbackInvoke(config->windows[0]->PreOpenInit);

	vrTrace("_WindowsConfig", "EXITING");
}


/*********************************************/
static void _EyesConfig(vrConfigInfo *config)
{
	int		count;
	int		eyecount;
	int		max_num_users = 0;
	vrEyeInfo	*this_eye;
	vrUserInfo	*this_user;

	vrTrace("_EyesConfig", "ENTERING");

	if (config->num_eyes == 0) {
		vrErrPrintf("Hmmm, no eyes in the configuration.\n");
		return;
	}

	/* Unfortunately, the only way to determine the number of users is to */
	/*   go through all the eyes and count the unique ones.  And since    */
	/*   we'd like to know the number of users before we do this, we're   */
	/*   sort of stuck.  But we do know that the number of users cannot   */
	/*   exceed the number of eyes.                                       */
	max_num_users = config->num_eyes;

	/* allocate the memory for the maximum number of users. */
	/*   slightly wasteful, but not worth the trouble of    */
	/*   being more memory efficient.                       */
	config->users = (vrUserInfo **)vrShmemAlloc0(max_num_users * sizeof(vrUserInfo *));
	config->num_users = 0;

	/* ?? TODO: the config file needs to (only?) keep track of the eyes */

	for (eyecount = 0; eyecount < config->num_eyes; eyecount++) {
		this_eye = config->eyes[eyecount];

		/* Assign an id number to this eye.  NOTE: one is added to the eye's */
		/*   position in the array in order to make it a 1-based number, and */
		/*   have the value of 0 indicate the eye is not part of the system  */
		/*   in use.                                                         */
		this_eye->num = eyecount + 1;


		/*************************************************************/
		/* Append this eye's user to the list of users, if necessary */
		this_user = this_eye->user;
		if (this_user == NULL) {
			vrErrPrintf("Eye '%s' doesn't have a user\n", config->eyes[eyecount]);
			/* TODO: what to do here?  exit?  or continue? */
		}

		/* Add user to list of configed users if not on the list already. */
		/*   NOTE: this is different than for Procs & Windows because */
		/*   the same user can (and is likely) to be handled by more  */
		/*   than one window.                                         */
		for (count = 0; count < config->num_users; count++) {
			if (this_user == config->users[count])
				this_user = NULL;
		}
		if (this_user != NULL) {
			vrDbgPrintfN(CONFIG_DBGLVL, "_EyesConfig(): "
				BOLD_TEXT "Adding new user [%#p \"%s\"] to config\n" NORM_TEXT,
				this_user, this_user->name);
			config->users[config->num_users] = this_user;
			config->num_users++;
		}
	}

	vrTrace("_EyesConfig", "EXITING");
}


/*********************************************/
static void _UsersConfig(vrConfigInfo *config)
{
	int		usercount;
	vrUserInfo	*this_user;

	vrTrace("_UsersConfig", "ENTERING");

	if (config->num_users == 0) {
		vrErrPrintf("Hmmm, no users in the configuration.\n");
		return;
	}

	for (usercount = 0; usercount < config->num_users; usercount++) {
		this_user = config->users[usercount];

		/* Assign an id number to this user.  NOTE: one is added to the */
		/*   user's position in the array in order to make it a 1-based */
		/*   number, and have the value of 0 indicate the user is not   */
		/*   part of the system in use.                                 */
		this_user->num = usercount + 1;

		/* Create a dummy sensor that the user's head points to. */
		/*   Once the input devices are initialized, then one of */
		/*   those should replace the head sensor for each user. */
		/* TODO: use the same dummy sensor for each user (ie. create the dummy before this loop) */
		this_user->head = vrInputCreateDataContainerArrayOfType(VRINPUT_6SENSOR, 1, NULL);	/* TODO: 05/26/2006 -- I think we can now use vrInputAddDummyToInputMap() to do this! */
		vrDbgPrintfN(CONFIG_DBGLVL, "_UsersConfig(): "
			BOLD_TEXT "made a dummy position sensor (%#p) for the head of user %d (%#p).\n" NORM_TEXT,
			this_user->head, usercount, this_user);
	}

	vrTrace("_UsersConfig", "EXITING");
}


/*********************************************/
/* This is a bit hacky, because while we can define/declare props*/
/*   in the config file, there is currently no way to associate  */
/*   props to anything else, so for now we just setup a single   */
/*   prop in vrConfigInfo.                                       */
static void _PropsConfig(vrConfigInfo *config)
{
	vrTrace("_PropsConfig", "ENTERING");

	config->num_props = 1;
	config->props = (vrPropInfo **)vrShmemAlloc0(config->num_props * sizeof(vrPropInfo *));

	config->props[0] = vrObjectSearch(config->context, VROBJECT_PROP, DEFAULT_PROP_NAME);

	vrTrace("_PropsConfig", "EXITING");
}


/*********************************************/
static void _InputsConfig(vrConfigInfo *config)
{
	int		count;
	vrInputDevice	*this_indev;

	vrTrace("_InputsConfig", "ENTERING");

	for (count = 0; count < config->num_input_devices; count++) {
		this_indev = config->input_devices[count];
		vrGetInputDeviceInfo(this_indev);
	}

	vrTrace("_InputsConfig", "EXITING");
}


/*********************************************/
/* This is just to initialize an empty configuration structure */
void vrConfigInitialize(vrConfigInfo *config)
{
	char		*env_string;		/* for reading environment variables */

	config->object_type = VROBJECT_CONFIGINFO;
	config->configured = 0;

	/********************************************/
	/*** set some default vrConfigInfo values ***/
	/********************************************/
	vrTrace("vrConfigInitialize", "before default settings");

	vrSettingsClear(&config->defaults);

	/** set some debug printing values **/
	config->defaults.debug_level = DEFAULT_DEBUG_LEVEL;
	env_string = getenv("FREEVR_DEBUGLEVEL");
	if (env_string != NULL)
		config->defaults.debug_level = vrAtoI(env_string);
	env_string = getenv("FREEVR_DEBUGLEVEL_NO");
	if (env_string != NULL)
		config->defaults.debug_level = vrAtoI(env_string);

	config->defaults.debug_exact = 0;
	env_string = getenv("FREEVR_DEBUGEXACT");
	if (env_string != NULL)
		config->defaults.debug_exact = vrAtoI(env_string);
	env_string = getenv("FREEVR_DEBUGEXACT_NO");
	if (env_string != NULL)
		config->defaults.debug_exact = vrAtoI(env_string);

	/** set the default configuration info printing sytles **/
	config->defaults.pre_context_print = none;
	config->defaults.pre_config_print = none;
	config->defaults.pre_input_print = none;
	config->defaults.post_context_print = none;
	config->defaults.post_config_print = none;
	config->defaults.post_input_print = none;

	/** set the default value of exit-uponerror **/
	config->defaults.exit_uponerror = 0;

	/** set the default rendering parameters **/
	/* NOTE: config->defaults->visrenmode is left as default and handled in _WindowsConfig() */
	config->defaults.near_clip =   0.1;
	config->defaults.far_clip = 1000.0;

	/***********************************/
	/*** clear some storage pointers ***/
	/***********************************/
	config->eyes = NULL;
}

static void segv_catcher(int sig)
{
    printf("pid %d  caught signal %d, sleeping\n",
            getpid(), sig);
    printf("try running:\n\n"
        "  gdb --pid %d\n\n", getpid());
    while(1)
        sleep(1);
}

/*********************************************/
vrContextInfo *vrConfigure(int *argc, char **argv, char **appargs)
{
	vrContextInfo	*context;
	vrConfigInfo	*config;

signal(SIGSEGV, segv_catcher);

	/**********************************************/
	/*** make sure context is initialized first ***/
	/**********************************************/
	if (vrContext == NULL)
		context = vrContextInitialize();	/* Performer version will alloc from a pfSharedArena */
	else	context = vrContext;

	config = context->config;

	if (config->configured)
		return context;

	vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: " BOLD_TEXT "**** Configuring the System! ****\n" NORM_TEXT);

	/**********************************************/
	/*** read all the configuration information ***/
	/**********************************************/
	vrTrace("vrConfigure", "before reading config files");
	_DefaultConfig(config);

	_ConfigReadFile(config, context->homedir, VRCONFIG_RCFILENAME);
	_ConfigReadFile(config, getenv("HOME"), VRCONFIG_RCFILENAME);
	/* TODO: "./VRCONFIG_RCFILENAME" should only be read if "." != "~" */
	/*   This is now done in the NCSA extended CAVE library, get code */
	/*   from there.                                                  */
	if (1)
		_ConfigReadFile(config, ".", VRCONFIG_RCFILENAME);

	_ConfigReadEnvVariable(config, VRCONFIG_RC_ENVVAR);
	_ConfigReadStringList(config, argc, argv);
	_ConfigReadStringList(config, NULL, appargs);	/* CAVElib-style list of string args */

	/*****************************************************************************/
	/*** Create the correct vrConfigInfo structures based on the chosen system ***/
	/*****************************************************************************/
	_SystemConfig(config);
	_ProcsConfig(config);
	_WindowsConfig(config);
	_EyesConfig(config);
	_UsersConfig(config);
	_PropsConfig(config);
	_InputsConfig(config);

	/***********************************************************************************/
	/*** Print the configuration in re-readable format if "printconfig" option given ***/
	/***********************************************************************************/
	if (config->config_print != NULL) {
		FILE	*fp_config;

		fp_config = fopen(config->config_print, "w");
		if (fp_config == NULL)
			fp_config = stdout;

		vrDbgPrintf("FreeVR config: printing active configuration to file '%s'\n", (fp_config == NULL ? "<stdout>" : config->config_print));
		vrFprintf(fp_config, "###################\n");
		vrFprintf(fp_config, "## This config file generated from vrConfigInfo structure\n\n");

		vrFprintConfig(fp_config, config, file_format);

		if (fp_config != stdout)
			fclose(fp_config);
	}

	/****************************************************************/
	/*** Print pre-startup configuration information if requested ***/
	/****************************************************************/
#if 0
	/* TODO: have a configuration setting that determines at what debug level these are printed */
	if (vrDbgDo(DEFAULT_DBGLVL)) {
		vrFprintContext(stdout, context, verbose);
		vrFprintConfig(stdout, config, verbose);
	} else {
		vrFprintContext(stdout, context, brief);
		vrFprintConfig(stdout, config, brief);
	}
#else
	vrFprintContext(stdout, context, (config->system->settings.pre_context_print == def) ? config->defaults.pre_context_print : config->system->settings.pre_context_print);
	vrFprintConfig(stdout, config, (config->system->settings.pre_config_print == def) ? config->defaults.pre_config_print : config->system->settings.pre_config_print);
	vrFprintInput(stdout, context->input, (config->system->settings.pre_input_print == def) ? config->defaults.pre_input_print : config->system->settings.pre_input_print);
#endif

	/*********************************************************/
	/*** Indicate that the configuration stage is complete ***/
	/*********************************************************/
	config->configured = 1;
	vrTrace("vrConfigure", "done");

	return context;
}


/*********************************************/
void vrFprintConfig(FILE *file, vrConfigInfo *config, vrPrintStyle style)
{
	int		count,
			input,
			prop,
			user,
			eye,
			eyelist,
			window,
			proc;

	if (config == NULL) {
		vrErrPrintf("vrFprintConfig(): " RED_TEXT "memory argument is NULL!\n" NORM_TEXT);
		return;
	}

	if (config->object_type != VROBJECT_CONFIGINFO) {
		vrErrPrintf("vrFprintConfig(): " RED_TEXT "memory argument does not point to the vrConfigInfo structure!\n" NORM_TEXT);
		return;
	}

	switch (style) {
	case none:
		break;

	case brief:
		vrFprintf(file, "==================================================\n");
		vrFprintf(file, "FreeVR Version = " BOLD_TEXT "\"%s\"" NORM_TEXT "\n", config->context->version);
		vrFprintf(file, "%s\n", config->context->compile);
		vrFprintf(file, "Using System = " BOLD_TEXT "\"%s\"" NORM_TEXT " (init = %d)\n", config->system_name, config->system_init);
		vrFprintf(file, "debug level = %d\n", config->defaults.debug_level);
		vrFprintf(file, "debug exact = %d\n", config->defaults.debug_exact);
		vrFprintf(file, "visrenmode default = \"%s\" (%d)\n",
			vrVisrenModeName(config->defaults.visrenmode), config->defaults.visrenmode);
		vrFprintf(file, "processor lock default = \"%s\" (%d)\n", "TODO:", config->defaults.lock_proc);
		vrFprintf(file, "InputMap = \"%s\"\n", (config->input_map_name == NULL ? RED_TEXT "(null)" NORM_TEXT : config->input_map_name));

		vrFprintf(file, "%d input devs = [ ", config->num_input_devices);
		for (count = 0; count < config->num_input_devices; count++) {
			vrFprintf(file, "\"%s\" ", config->input_devices[count]->name);
		}
		vrFprintf(file, "] (init = %d)\n", config->inputs_init);

		vrFprintf(file, "%d props = [ ", config->num_props);
		for (count = 0; count < config->num_props; count++) {
			if (config->props[count] == NULL) {
				vrFprintf(file, " (nil) [this shouldn't happen] ");
			} else {
				vrFprintf(file, "\"%s\" ", config->props[count]->name);
			}
		}
		vrFprintf(file, "] (init = NYI)\n" /* , config->props_init */);

		vrFprintf(file, "%d users = [ ", config->num_users);
		for (count = 0; count < config->num_users; count++) {
			vrFprintf(file, "\"%s\" ", config->users[count]->name);
		}
		vrFprintf(file, "] (init = %d)\n", config->users_init);

		vrFprintf(file, "%d eyes = [ ", config->num_eyes);
		for (count = 0; count < config->num_eyes; count++) {
			vrFprintf(file, "\"%s\" ", config->eyes[count]->name);
		}
		vrFprintf(file, "]\n");

		vrFprintf(file, "%d eyelists = [ ", config->num_eyelists);
		for (count = 0; count < config->num_eyelists; count++) {
			vrFprintf(file, "\"%s\" ", config->eyelists[count]->name);
		}
		vrFprintf(file, "]\n");

		vrFprintf(file, "%d windows = [ ", config->num_windows);
		for (count = 0; count < config->num_windows; count++) {
			vrFprintf(file, "\"%s\" ", config->windows[count]->name);
		}
		vrFprintf(file, "] (init = NYI)\n" /*, config->windows_init */);

		vrFprintf(file, "%d processes = [ ", config->num_procs);
		for (count = 0; count < config->num_procs; count++) {
			vrFprintf(file, "\"%s\" (%d) ",
				config->procs[count]->name,
				config->procs[count]->pid);
		}
		vrFprintf(file, "]\n", config->procs_init);

		vrFprintf(file, "==================================================\n");
		break;

	case everything:
		/* here, this is the same as verbose -- it also means more info */
		/* during the read phase of configuration.                      */
		/* TODO: decide if it should be more.                           */

	case verbose:
		vrFprintf(file, "==================================================\n");
		vrFprintf(file, "Using System = " BOLD_TEXT "\"%s\"\n" NORM_TEXT, config->system_name);
		vrFprintf(file, "vrConfigInfo at %#p\n", config);
		vrFprintSystemSettings(file, &config->defaults, style);

#if 0
		vrFprintf(file, "General callbacks:\n"
				"\tVisrenInit = %#p\n\tVisrenFrame = %#p\n\tVisrenWorld = %#p\n"
				"\tVisrenSim = %#p\n\tVisrenExit = %#p\n",
				config->VisrenInit,
				config->VisrenFrame,
				config->VisrenWorld,
				config->VisrenSim,
				config->VisrenExit);
#endif

		vrFprintf(file, "InputMap = \"%s\"\n", (config->input_map_name == NULL ? RED_TEXT "(null)" NORM_TEXT : config->input_map_name));

		vrFprintf(file, "=== %d input_devices ===\n", config->num_input_devices);
		vrFprintf(file, "Inputs initialized = %d\n", config->inputs_init);
		for (input = 0; input < config->num_input_devices; input++) {
			vrFprintf(file, "input[%d] at %#p (%#p) = ", input, config->input_devices[input], &config->input_devices[input]);
			vrFprintInputDevice(file, config->input_devices[input], style);
		}

		vrFprintf(file, "=== %d props ===\n", config->num_props);
		vrFprintf(file, "Props initialized = NYI\n" /* , config->props_init */);
		for (prop = 0; prop < config->num_props; prop++) {
			vrFprintf(file, "prop[%d] at %#p (%#p) = ", prop, config->props[prop], &config->props[prop]);
			vrFprintPropInfo(file, config->props[prop], style);
		}

		vrFprintf(file, "=== %d users ===\n", config->num_users);
		vrFprintf(file, "Users initialized = %d\n", config->users_init);
		for (user = 0; user < config->num_users; user++) {
			vrFprintf(file, "user[%d] at %#p (%#p) = ", user, config->users[user], &config->users[user]);
			vrFprintUserInfo(file, config->users[user], style);
		}

		vrFprintf(file, "=== %d eyes ===\n", config->num_eyes);
		for (eye = 0; eye < config->num_eyes; eye++) {
			vrFprintf(file, "eye[%d] at %#p (%#p) = ", eye, config->eyes[eye], &config->eyes[eye]);
			vrFprintEyeInfo(file, config->eyes[eye], style);
		}

		vrFprintf(file, "=== %d eyelists ===\n", config->num_eyelists);
		for (eyelist = 0; eyelist < config->num_eyelists; eyelist++) {
			vrFprintf(file, "eyelist[%d] at %#p (%#p) = ", eyelist, config->eyelists[eyelist], &config->eyelists[eyelist]);
			vrFprintEyelistInfo(file, config->eyelists[eyelist], style);
		}

		vrFprintf(file, "=== %d windows ===\n", config->num_windows);
		vrFprintf(file, "Windows initialized = NYI\n" /* , config->windows_init */);
		for (window = 0; window < config->num_windows; window++) {
			vrFprintf(file, "window[%d] at %#p (%#p) = ", window, config->windows[window], &config->windows[window]);
			vrFprintWindowInfo(file, config->windows[window], style);
		}

		vrFprintf(file, "=== %d processes ===\n", config->num_procs);
		vrFprintf(file, "Processes initialized = %d\n", config->procs_init);
		for (proc = 0; proc < config->num_procs; proc++) {
			vrFprintf(file, "process[%d] at %#p (%#p) = ", proc, config->procs[proc], &config->procs[proc]);
			vrFprintProcessInfo(file, config->procs[proc], style);
		}

		vrFprintf(file, "=== system ===\n");
		vrFprintf(file, "System initialized = %d\n", config->system_init);
		vrFprintf(file, "system at %#p = ", config->system);
		vrFprintSystemInfo(file, config->system, style);

		vrFprintf(file, "==================================================\n");
		break;

	case file_format:
		/***************/
		/* Global Info */
		vrFprintf(file, "\n###################\n");
		vrFprintf(file,   "## Global defaults:\n");
		vrFprintSystemSettings(file, &config->defaults, style);
		vrFprintf(file, "\n");

		/***************/
		/* System Info */
		vrFprintf(file, "\n###################\n");
		vrFprintf(file,   "## System settings:\n");
		vrFprintSystemInfo(file, config->system, style);

		/****************/
		/* Process Info */
		vrFprintf(file, "\n####################\n");
		vrFprintf(file,   "## Process settings:\n");
		for (proc = 0; proc < config->num_procs; proc++) {
			vrFprintProcessInfo(file, config->procs[proc], style);
		}

		/***************/
		/* Window Info */
		vrFprintf(file, "\n###################\n");
		vrFprintf(file,   "## Window settings:\n");
		for (window = 0; window < config->num_windows; window++) {
			vrFprintWindowInfo(file, config->windows[window], style);
		}

		/*************/
		/* Eyes Info */
		vrFprintf(file, "\n#################\n");
		vrFprintf(file,   "## Eyes settings: (NOTE: commented out because not part of the configuration file.)\n");
		for (eye = 0; eye < config->num_eyes; eye++) {
			vrFprintEyeInfo(file, config->eyes[eye], style);
		}

		vrFprintf(file, "\n####################\n");
		vrFprintf(file,   "## Eyelist settings:\n");
		for (eyelist = 0; eyelist < config->num_eyelists; eyelist++) {
			vrFprintEyelistInfo(file, config->eyelists[eyelist], style);
		}

		/**************/
		/* Users Info */
		vrFprintf(file, "\n#################\n");
		vrFprintf(file,   "## User settings:\n");
		for (user = 0; user < config->num_users; user++) {
			vrFprintUserInfo(file, config->users[user], style);
		}

		/**************/
		/* Props Info */
		vrFprintf(file, "\n#################\n");
		vrFprintf(file,   "## Prop settings:\n");
		for (prop = 0; prop < config->num_props; prop++) {
			vrFprintPropInfo(file, config->props[prop], style);
		}

		/**********************/
		/* Input Devices Info */
		vrFprintf(file, "\n#########################\n");
		vrFprintf(file,   "## Input Device settings:\n");
		for (input = 0; input < config->num_input_devices; input++) {
			vrFprintInputDevice(file, config->input_devices[input], style);
		}

		/******************/
		/* Input Map Info */
		vrFprintf(file, "\n#################\n");
		vrFprintf(file,   "## Input Mapping:   TODO\n");
		/* TODO: */

		vrFprintf(file,   "\n\n");
		break;

	default:
	case one_line:	/* TODO: implement this */
	case machine:	/* TODO: implement this */
		vrFprintf(file, "TODO: put short version of config info here\n");
		break;
	}
}


/****************************************************************/
/* Some helper functions that make use of the config structure. */
/****************************************************************/

/************************************************************/
/* NOTE: we have a choice of whether to handle the built-in */
/*   variable name before or after checking the environment */
/*   variables.  It is nice to know that a variable always  */
/*   means the same thing -- which favors doing the built-in*/
/*   check first, however by checking the existence of the  */
/*   env-var first, we have to opportunity to override the  */
/*   built-in values.  For now we'll go with the former.    */
/* NOTE: this function also can handle special variable     */
/*   operations specified by a character that precedes the  */
/*   actual name of the variable.  Currently only the '?'   */
/*   operator has been implemented, which returns "0" or "1"*/
/*   depending on whether the variable exists or not.       */
char *vrEvaluateVariable(vrContextInfo *context, char *variable_name)
{
static	char		temp_string[1024];
	char		*return_string = NULL;
	char		*return_value;
	int		existence_check = 0;	/* by default, don't do an existence check */

	/**********************************************/
	/* Do some sanity checks on the variable name */
	if (variable_name == NULL) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "vrEvaluateVariable(): Null variable name\n");
	}

	if (variable_name[0] == '\0') {
		vrDbgPrintfN(ALWAYS_DBGLVL, "vrEvaluateVariable(): Empty variable name\n");
	}

	vrDbgPrintfN(VARIABLE_DBGLVL, "vrEvaluateVariable(): evaluating variable '%s'\n", variable_name);

	/******************************************/
	/* Check for a special operator character */
	if (variable_name[0] == '?') {
		/* doing a variable existence check */
		existence_check = 1;
		variable_name++;	/* skip the '?' */

		vrDbgPrintfN(VARIABLE_DBGLVL, "vrEvaluateVariable(): checking for variable existence\n");
	}

	/***************************************************/
	/* First check whether variable is a built-in name */
	if ((!strcmp(variable_name, "machine")) || (!strcmp(variable_name, "hostname"))) {
		/* return the hostname of the machine we're running on */
		gethostname(temp_string, 1024);
		return_string = vrShmemStrDup(temp_string);
	} else

	if (!strcmp(variable_name, "version")) {
		/* return the FreeVR version string */
		/* The version string is a special case, in that we only want   */
		/*   the actual version code -- not the words "FreeVR Version". */
		/*   However, we want the extra words in the string in order to */
		/*   quickly determine against which version an application was */
		/*   compiled.                                                  */

		char	*last_space;
		char	*last_tab;
		char	*last_return;
		char	*last_newline;
		char	*last_backspace;
		char	*begin_version;

		last_space = strrchr(context->version, ' ');
		last_tab = strrchr(context->version, '\t');
		last_return = strrchr(context->version, '\r');
		last_newline = strrchr(context->version, '\n');
		last_backspace = strrchr(context->version, '\b');

		begin_version = context->version;
		if (last_space > begin_version) begin_version = last_space+1;
		if (last_tab > begin_version) begin_version = last_tab+1;
		if (last_return > begin_version) begin_version = last_return+1;
		if (last_newline > begin_version) begin_version = last_newline+1;
		if (last_backspace > begin_version) begin_version = last_backspace+1;

		/* begin the version string with a 'v', so it won't be parsed as a number */
		temp_string[0] = 'v';
		strcpy(&temp_string[1], begin_version);

		return_string = temp_string;
	} else

	if (!strcmp(variable_name, "fullversion")) {
		/* return the FreeVR library architecture (and possibly abi format) */
		return_string = context->version;
	} else

	if (!strcmp(variable_name, "versionnum")) {
		/* return the value of the numeric form of the FreeVR version code */
		sprintf(temp_string, "%d", FREEVR_VERSION);
		return_string = temp_string;
	} else

	if (!strcmp(variable_name, "name")) {
		/* return the name of the FreeVR application -- as specified by the authors (or not) */
		return_string = context->name;
	} else

	if ((!strcmp(variable_name, "author")) || (!strcmp(variable_name, "authors"))) {
		/* return the author(s) of the FreeVR application -- as specified by the authors (or not) */
		return_string = context->authors;
	} else

	if ((!strcmp(variable_name, "appstatus"))) {
		/* return the programmer specified status of the FreeVR application */
		return_string = context->status_info;
	} else

	if (!strcmp(variable_name, "arch")) {
		/* return the FreeVR library architecture (and possibly abi format) */
		return_string = context->arch;
	} else

	if (!strcmp(variable_name, "binaryformat")) {
		/* alias for "arch" */
		return_string = context->arch;
	} else

	if (!strcmp(variable_name, "compile")) {
		/* return the FreeVR library compilation data */
		return_string = context->compile;
	} else

	if (!strcmp(variable_name, "target")) {
		/* return the FreeVR library Makefile target */
		return_string = context->compile_target;
	} else

	if (!strcmp(variable_name, "freevrhomedir")) {
		/* return the home directory of the FreeVR installation */
		return_string = context->homedir;
	} else

	if (!strcmp(variable_name, "memsize")) {
		/* return the size of the shared memory */
		sprintf(temp_string, "%d", (int)(context->shmem_size));
		return_string = temp_string;
	} else

	if (!strcmp(variable_name, "sysstatus")) {
		/* return the value of the system status */
		sprintf(temp_string, "%d", context->status);
		return_string = temp_string;
	} else

	if (!strcmp(variable_name, "startup_error")) {
		/* return the value of the system startup error */
		sprintf(temp_string, "%d", context->startup_error);
		return_string = temp_string;
	} else

	if (!strcmp(variable_name, "time_immemorial")) {
		/* return the value of the system time immemorial */
		sprintf(temp_string, "%.2lf", context->time_immemorial);
		return_string = temp_string;
	} else

	if ((!strcmp(variable_name, "sim_time")) || (!strcmp(variable_name, "time"))) {
		/* return the value of the system simulation time */
		sprintf(temp_string, "%.2lf", vrCurrentSimTime());
		return_string = temp_string;
	} else

	if (!strcmp(variable_name, "wall_time")) {
		/* return the value of the system wall time */
		sprintf(temp_string, "%.2lf", vrCurrentWallTime());
		return_string = temp_string;
	} else

	if (!strcmp(variable_name, "debuglevel")) {
		/* return the value of the global debuglevel variable */
		sprintf(temp_string, "%d", context->config->defaults.debug_level);
		return_string = temp_string;
	} else

	if (!strcmp(variable_name, "debugthistoo")) {
		/* return the value of the global debugthistoo variable */
		sprintf(temp_string, "%d", context->config->defaults.debug_exact);
		return_string = temp_string;
	} else

	if (!strcmp(variable_name, "system")) {
		/* return the name of the running system */
		return_string = context->config->system_name;
	} else

	if (!strcmp(variable_name, "def_visrenmode")) {
		/* return the name of the default visrenmode setting */
		return_string = vrVisrenModeName(context->config->defaults.visrenmode);
	} else

	if (!strcmp(variable_name, "win_style")) {
		/* return the name of the windowing interface (Win32 or GLX) */
#if defined(WIN_GLX)
		return_string = vrShmemStrDup("GLX");
#elif defined(WIN_WGL)
		return_string = vrShmemStrDup("Win32-GL");
#else
		return_string = vrShmemStrDup("Win-Unk");
#endif
	} else

	if (!strcmp(variable_name, "gfx_style")) {
		/* return the name of the windowing interface (Win32 or GLX) */
#if defined(GFX_PERFORMER)
		return_string = vrShmemStrDup("Performer");
#elif defined(GFX_OSG)
		return_string = vrShmemStrDup("OSG");
#else
		return_string = vrShmemStrDup("OpenGL");
#endif
	} else

	if (!strcmp(variable_name, "sem_style")) {
		/* return the name of the semaphore method of implementation */
#if defined(SEM_IRIX)
		return_string = vrShmemStrDup("IRIX");
#elif defined(SEM_POSIX)
		return_string = vrShmemStrDup("POSIX");
#elif defined(SEM_SYSVIPC)
		return_string = vrShmemStrDup("SYSVIPC");
#elif defined(SEM_WIN32)
		return_string = vrShmemStrDup("WIN32");
#elif defined(SEM_TCP)
		return_string = vrShmemStrDup("TCP");
#else
		return_string = vrShmemStrDup("Sem-Unk");
#endif
	} else

	if (!strcmp(variable_name, "shm_style")) {
		/* return the name of the shared memory implementation */
#if defined(SHM_SVR4MMAP)
		return_string = vrShmemStrDup("SVR4MMAP");
#elif defined(SHM_BSDANONMMAP)
		return_string = vrShmemStrDup("BSDANONMMAP");
#elif defined(SHM_SYSVIPC)
		return_string = vrShmemStrDup("SYSVIPC");
#elif defined(SHM_PF_ARENA)
		return_string = vrShmemStrDup("PF_ARENA");
#elif defined(SHM_NONE)
		return_string = vrShmemStrDup("NONE");
#else
		return_string = vrShmemStrDup("Shm-Unk");
#endif
	} else

	if (!strcmp(variable_name, "sock_style")) {
		/* return the name of the shared memory implementation */
#if defined(SOCK_W32)
		return_string = vrShmemStrDup("WIN32");
#else
		return_string = vrShmemStrDup("BSD");
#endif
	} else

	if (!strcmp(variable_name, "mp_style")) {
		/* return the name of the multi-processing implementation */
#if defined(MP_PTRHEADS)
		return_string = vrShmemStrDup("PTHREADS");
#elif defined(MP_PTHREADS2)
		return_string = vrShmemStrDup("PTHREADS2");
#else
		return_string = vrShmemStrDup("FORK");
#endif
	} else

	if (!strcmp(variable_name, "compile_options")) {
		/* return the all the compile options used during compilation */
		return_string = vrShmemStrDup("");
#if defined(WIN_GLX)
		return_string = vrShmemStrCat(return_string, " -DWIN_GLX");
#endif
#if defined(WIN_WGL)
		return_string = vrShmemStrCat(return_string, " -DWIN_WGL");
#endif
#if defined(GFX_PERFORMER)
		return_string = vrShmemStrCat(return_string, " -DGFX_PERFORMER");
#endif
#if defined(GFX_OSG)
		return_string = vrShmemStrCat(return_string, " -DGFX_OSG");
#endif
#if defined(SHM_DUMMY)
		return_string = vrShmemStrCat(return_string, " -DSHM_DUMMY");
#endif
#if defined(SHM_NONE)
		return_string = vrShmemStrCat(return_string, " -DSHM_NONE");
#endif
#if defined(SHM_SVR4MMAP)
		return_string = vrShmemStrCat(return_string, " -DSHM_SVR4MMAP");
#endif
#if defined(SHM_BSDANONMMAP)
		return_string = vrShmemStrCat(return_string, " -DSHM_BSDANONMMAP");
#endif
#if defined(SHM_SYSVIPC)
		return_string = vrShmemStrCat(return_string, " -DSHM_SYSVIPC");
#endif
#if defined(SHM_PF_ARENA)
		return_string = vrShmemStrCat(return_string, " -DSHM_PF_ARENA");
#endif
#if defined(SEM_IRIX)
		return_string = vrShmemStrCat(return_string, " -DSEM_IRIX");
#endif
#if defined(SEM_POSIX)
		return_string = vrShmemStrCat(return_string, " -DSEM_POSIX");
#endif
#if defined(SEM_SYSVIPC)
		return_string = vrShmemStrCat(return_string, " -DSEM_SYSVIPC");
#endif
#if defined(SEM_WIN32)
		return_string = vrShmemStrCat(return_string, " -DSEM_WIN32");
#endif
#if defined(SEM_TCP)
		return_string = vrShmemStrCat(return_string, " -DSEM_TCP");
#endif
#if defined(MP_PTHREADS)
		return_string = vrShmemStrCat(return_string, " -DMP_PTHREADS");
#endif
#if defined(MP_PTHREADS2)
		return_string = vrShmemStrCat(return_string, " -DMP_PTHREADS2");
#endif
#if defined(SOCK_W32)
		return_string = vrShmemStrCat(return_string, " -DSOCK_W32");
#endif
	} else

	/* See if the variable name matches any of the debug levels */
	if (vrDbgStringValue(variable_name) >= 0) {
		/* return the value of the debug level name */
		sprintf(temp_string, "%d", vrDbgStringValue(variable_name));
		return_string = temp_string;
	} else


	/**************************************************************/
	/* If not a built-in variable, then use environment variables */
	if (getenv(variable_name) != NULL) {
		return_string = getenv(variable_name);
		vrDbgPrintfN(VARIABLE_DBGLVL, "vrEvaluateVariable(): using environment variable '%s'\n", variable_name);
	} else

	/****************************************************/
	/* otherwise report that the variable doesn't exist */
	{
		vrDbgPrintfN(DEFAULT_DBGLVL, "vrEvaluateVariable(): No such variable: '%s'\n", variable_name);
	}

	/********************************/
	/* return the appropriate value */

	/* NOTE: I'm not sure we need to put the string into shared memory. */
	/*   These strings are used only once, and will never be deleted.   */
	/*   TODO: perhaps try without shared memory duplication.           */
	if (existence_check) {
		if (return_string == NULL) {
			return_value = vrShmemStrDup("0");
		} else {
			return_value = vrShmemStrDup("1");
		}
	} else {
		if (return_string == NULL) {
			return_value = NULL;
		} else {
			return_value = vrShmemStrDup(return_string);
		}
	}

	vrDbgPrintfN(VARIABLE_DBGLVL, "vrEvaluateVariable(): returning value '%s' for variable '%s'\n", return_value, variable_name);
	return (return_value);
}


/*****************************************************************/
/* TODO: consider naming this vrConfigSetNear() */
/* TODO: add the ability to set for specific user, window, or as global default */
/*	NOTE: the above could be written as one function by passing a pointer   */
/*	to whatever structure should be set, and then using the struct-type     */
/*      field to determine how to do the setting -- and in app-prog code, they  */
/*      will generally pass a vrContext handle, so it will set for the system   */
/*	in that case.                                                           */
void vrSetNear(vrContextInfo *context, double newnear)
{
	int		count;

	context->config->system->settings.near_clip = newnear;
#if 0 /* 06/16/2006 -- better to just change for the system and use the built-in inheritance */
	for (count = 0; count < context->config->num_windows; count++) {
		context->config->windows[count]->settings.near_clip = newnear;
	}
#endif
}


/*****************************************************************/
/* TODO: consider naming this vrConfigSetFar() */
/* TODO: add the ability to set for specific user, window, or as global default */
/*	NOTE: the above could be written as one function by passing a pointer   */
/*	to whatever structure should be set, and then using the struct-type     */
/*      field to determine how to do the setting -- and in app-prog code, they  */
/*      will generally pass a vrContext handle, so it will set for the system   */
/*	in that case.                                                           */
void vrSetFar(vrContextInfo *context, double newfar)
{
	int		count;

	context->config->system->settings.far_clip = newfar;
#if 0 /* 06/16/2006 -- better to just change for the system and use the built-in inheritance */
	for (count = 0; count < context->config->num_windows; count++) {
		context->config->windows[count]->settings.far_clip = newfar;
	}
#endif
}


/*********************************************/
/*********************************************/
#ifdef TEST_APP /* { */

main(int argc, char *argv[])
{
	vrConfigure(0, NULL, NULL);
}
#endif /* } TEST_APP */

