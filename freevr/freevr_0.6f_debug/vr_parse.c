/* ======================================================================
 *
 *  CCCCC          vr_parse.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: October 12, 1998
 * CC   CC         Last Modified: August 20, 2013
 *  CCCCC
 *
 * Code file for various parsing routines.  This is used for parsing
 * the configuration file, and also for parsing argument options, and
 * even commands sent via sockets.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vr_parse.h"
#include "vr_debug.h"
#include "vr_input.h"



/*********************************************************************/
/* boolean that indicates a token is a statement terminator */
int vrTokenTerminal(int tokentype)
{
	switch (tokentype) {
	case VRTOKEN_SEMICOLON:
	case VRTOKEN_CLOSECURL:
	case VRTOKEN_EOF:
		return 1;
	default:
		return 0;
	}
}


/*********************************************************************/
/* boolean that indicates a token is a value specifier */
int vrTokenValue(int tokentype)
{
	switch (tokentype) {
	case VRTOKEN_NUMBER:
	case VRTOKEN_STRING:
	case VRTOKEN_NIL:
		return 1;
	default:
		return 0;
	}
}


/************************************************************************/
/* boolean that indicates whether a token is a bi-operand math operator */
int vrBiMathOperatorToken(int tokentype)
{
	if ((tokentype > VRTOKEN_BIOP_BEGIN) && (tokentype < VRTOKEN_BIOP_END))
		return 1;
	else	return 0;
}


/*****************************************************************/
/* boolean that indicates a token is a uni-operand math operator */
/* NOTE: currently we don't handle any such operators, so always returns false */
int vrUniMathOperator(int tokentype)
{
	return 0;
}


/*********************************************************************/
/* convert certain strings to numbers */
int vrParseStringAsNumber(char *str)
{
	if (!strcasecmp(str, "default"))	return VRTOKEN_DEFAULT;
	else if (!strcasecmp(str, "no"))	return VRTOKEN_NO;
	else if (!strcasecmp(str, "yes"))	return VRTOKEN_YES;
	else if (!strcasecmp(str, "false"))	return VRTOKEN_FALSE;
	else if (!strcasecmp(str, "true"))	return VRTOKEN_TRUE;
	else if (!strcasecmp(str, "off"))	return VRTOKEN_OFF;
	else if (!strcasecmp(str, "on"))	return VRTOKEN_ON;

	else
		return VRTOKEN_UNKNOWN;
}


/*********************************************************************/
/* Determine the number of numeric characters that begin the given string. */
/*   A result of 0 means that the string does not begin with a number, so  */
/*   this function can be used to tell whether a string begins with a      */
/*   number, and if so, the length (not counting a closing '\0') that can  */
/*   then be used to copy the numeric string to another string.            */
static int vrNumericStringLength(char *string)
{
static	char	*numchars = "-.0123456789";
static	char	*hexnumchars = "-.0123456789ABCDEFabcdef";
static	char	*numradix = "boxd";
	int	numlen;

	if (strspn(string, numchars) > 0) {
		if ((string[0] == '0') && (strspn(&string[1], numradix) > 0)) {
			/* this number has a radix */
			/* NOTE: we know there must be at least two array elements left */
			/*   in string because the first one isn't '\0', and */
			/*   there must be one character that is '\0'.                  */
			if (string[1] == 'x')
				numlen = strspn(string+2, hexnumchars) + 2;
			else	numlen = strspn(string+2, numchars) + 2;
		} else if ((string[0] == '-') && (string[1] == '0') && (strspn(&string[2], numradix) > 0)) {
			if (string[2] == 'x')
				numlen = strspn(string+3, hexnumchars) + 3;
			else	numlen = strspn(string+3, numchars) + 3;
		} else {
			numlen = strspn(string, numchars);
		}

		return (numlen);
	} else {
		return (0);
	}
}


/*********************************************************************/
/* The next input is known to be a numeric string, so parse */
/*   that into a token.                                     */
static vrTokenInfo vrTokenNumber(char *tokstring, vrParseInfo *parse)
{
	vrTokenInfo	tok = { VRTOKEN_UNKNOWN, "<unk>" };

	tok.token = VRTOKEN_NUMBER;
	tok.string = strdup(tokstring);
	tok.linenum = parse->linenum;

	return (tok);
}


/*********************************************************************/
/* The next input is known to be an alphanumeric string, so parse */
/*   that into a token.                                           */
static vrTokenInfo vrTokenString(char *tokstring, vrParseInfo *parse)
{
	vrTokenInfo	tok = { VRTOKEN_UNKNOWN, "<unk>" };
	int		count;

	tok.token = VRTOKEN_STRING;
	tok.string = strdup(tokstring);
	tok.linenum = parse->linenum;

	for (count = 0; count < parse->nummaps; count++) {
		if (!strcasecmp(parse->tsmap_list[count].string, tokstring)) {
			tok.token = parse->tsmap_list[count].token;
			tok.string = strdup(tokstring);

			return (tok);
		}
	}

	return (tok);
}


/*********************************************************************/
/* The next input (character) is known to be a punctuation */
/*   character, so figure out what the token is.           */
static vrTokenInfo vrTokenPunc(char tokchar, vrParseInfo *parse)
{
	vrTokenInfo	tok = { VRTOKEN_UNKNOWN, "<unk>" };

	tok.linenum = parse->linenum;

	switch (tokchar) {
	case '{':
		tok.token = VRTOKEN_OPENCURL;
		tok.string = "{";
		break;
	case '}':
		tok.token = VRTOKEN_CLOSECURL;
		tok.string = "}";
		break;
	case ',':
		tok.token = VRTOKEN_COMMA;
		tok.string = ",";
		break;
	case ';':
		tok.token = VRTOKEN_SEMICOLON;
		tok.string = ";";
		break;
	case '"':
		tok.token = VRTOKEN_QUOTE;
		tok.string = "\"";
		break;
	case '(':
		tok.token = VRTOKEN_OPENPAREN;
		tok.string = "(";
		break;
	case ')':
		tok.token = VRTOKEN_CLOSEPAREN;
		tok.string = ")";
		break;
	}

	return (tok);
}


/*********************************************************************/
/* NOTE: this currently assumes all variables are environment variables */
/*   or special built in variables such as the architecture.            */
static vrTokenInfo vrTokenEvaluateVariable(vrTokenInfo var_token)
{
static	char		*whitespace = " \t\r\b";
	vrTokenInfo	value_token = { VRTOKEN_NIL, "<nil>" };	/* return the <nil> token if there's a problem */
	char		*variable_name = var_token.string;	/* the name of the variable */
	char		*var_string;				/* the string of the variable */
	int		toklen;

	/***************************************************/
	/* first set the line number of the returned token */
	value_token.linenum = var_token.linenum;			 /* set to line with the variable */

	/*******************************************/
	/* next check the validity of the variable */
	if (variable_name == NULL) {
		/* NOTE: this really shouldn't happen */
		vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): empty variable name string.\n", var_token.linenum);
		return value_token;
	}

	if (variable_name[0] == '\0') {
		vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): empty variable name.\n", var_token.linenum);
		return value_token;
	}

	/**************************************/
	/* now do a simple parse on the value */
	/*   Currently we only check for numeric or string values. */

	/* point to the string value of the variable */
	var_string = vrEvaluateVariable(vrContext, variable_name);	/* NOTE: using global vrContext */
	if (var_string == NULL) {
		vrDbgPrintfN(CONFIG_ERROR_DBGLVL, "Config Parse (line %d): " RED_TEXT "no such variable '$%s'\n" NORM_TEXT,
			var_token.linenum, var_token.string);
		return value_token;
	}

	var_string += strspn(var_string, whitespace);		/* skip whitespace */

	/* check whether the variable value is a number */
	if ((toklen = vrNumericStringLength(var_string)) > 0) {
		value_token.token = VRTOKEN_NUMBER;
		value_token.string = vrShmemAlloc(toklen+1);
		strncpy(value_token.string, var_string, toklen);
		value_token.string[toklen] = '\0';
	} else {
		/* NOTE: all non-numbers are strings, so we know token is a string */

		value_token.token = VRTOKEN_STRING;
		toklen = strlen(var_string);			/* copy the entire string to be the string value */
		value_token.string = vrShmemAlloc(toklen+1);
		strncpy(value_token.string, var_string, toklen);
		value_token.string[toklen] = '\0';
	}

	vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrTokenEvaluateVariable(), line %d: "
		BOLD_TEXT "variable $%s value is '%s', token type = %d\n" NORM_TEXT,
		value_token.linenum, variable_name, value_token.string, value_token.token);

	return (value_token);
}


/*********************************************************************/
/* The next input is known to be an alphanumeric string, so determine */
/*   whether the token is a function, and if so, evaluate it.         */
static vrTokenInfo vrFunctionString(char *tokstring, vrParseInfo *parse)
{
static	char		*whitespace = " \t\r\b";
	vrTokenInfo	tok = { VRTOKEN_NIL, "<nil>" };
	int		toklen;
	char		*(*function)(char *) = NULL;
	char		*string = NULL;
	char		*result = NULL;
	char		*tofree;
	vrTokenInfo	openparen_token;
	vrTokenInfo	arg_token;
	int		count;

	vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> " RED_TEXT "'%s' may be a function -- entering" NORM_TEXT "\n",
		__FILE__, __LINE__, "vrFunctionString()", tokstring);

	/***************************************/
	/* initialize the default token result */
#if 0 /* including these would work, except on the receiving end, it could be   */
	/* interpretted that the string token was a function, and just happened */
	/* to return a string that was the function's name.                     */
	tok.token = VRTOKEN_STRING;
	tok.string = strdup(tokstring);
#endif
	tok.linenum = parse->linenum;

	/**********************************/
	/* search for a matching function */
	for (count = 0; count < parse->numfuncs; count++) {
		if (!strcasecmp(parse->funcmap_list[count].string, tokstring)) {
			function = parse->funcmap_list[count].func;
		}
	}

	/***************************************************************/
	/* If no matching function is found, then return the NIL token */
	if (function == NULL) {
		vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> " RED_TEXT "'%s' is not a function -- exiting" NORM_TEXT "\n",
			__FILE__, __LINE__, "vrFunctionString()", tokstring);
		return (tok);
	}

	/****************************************************************************/
	/* Otherwise, determine the string between parentheses and pass to function */
	openparen_token = vrParseJustNextToken(parse);

	if (openparen_token.token != VRTOKEN_OPENPAREN) {
#if 0 /* one option ("1") will treat a function name not followed by an OP as though   */
	/* it is a function, and the arguments start right away.  Another option ("0") */
	/* will report a matched function name that isn't followed by an open-paren    */
	/* as simply not a function, but just that string.                             */
		vrErrPrintf("FreeVR config: Error (line %d): "
			RED_TEXT "Function name ('%s') not followed by open paren.\n" NORM_TEXT,
			openparen_token.linenum, tokstring);
		string = vrShmemStrDup(openparen_token.string);
#else
		vrParseReturnLastToken(openparen_token, parse);
		vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> " RED_TEXT "'%s' is apparently not a function -- exiting" NORM_TEXT "\n",
			__FILE__, __LINE__, "vrFunctionString()", tokstring);
		return (tok);
#endif
	}

	/* concatenate subsequent tokens into a string with the arguments */
	arg_token = vrParseNextToken(parse);
	while (arg_token.token != VRTOKEN_CLOSEPAREN  && !vrTokenTerminal(arg_token.token)) {

		if (string == NULL) {
			string = vrShmemStrDup(arg_token.string);
		} else {
			tofree = string;
#if 0
			string = vrShmemStrCat(string, " ");		/* add whitespace */
#endif
			string = vrShmemStrCat(string, arg_token.string);
			vrShmemFree(tofree);
		}

		arg_token = vrParseNextToken(parse);
	}

	/********************************************/
	/* evaluate the function with the arguments */
	result = (*function)(string);

	/* determine whether the result is a number */
	result += strspn(result, whitespace);			/* skip whitespace */
	if ((toklen = vrNumericStringLength(result)) > 0) {
		tok.token = VRTOKEN_NUMBER;
	} else {
		/* NOTE: all non-numbers are strings, so we know token is a string */

		tok.token = VRTOKEN_STRING;
		toklen = strlen(result);			/* copy the entire string to be the string value */
	}
	tok.string = vrShmemAlloc(toklen+1);
	strncpy(tok.string, result, toklen);
	tok.string[toklen] = '\0';

	vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> " RED_TEXT "function '%s' was evaluated to '%s' -- exiting" NORM_TEXT "\n",
		__FILE__, __LINE__, "vrFunctionString()", tokstring, tok.string);
	return (tok);
}


/*********************************************************************/
/* just do a regular vrParseNextToken(), and print the result, */
/*   and return the token as normal.                           */
vrTokenInfo vrParseNextTokenPrint(vrParseInfo *parse)
{
	vrTokenInfo	token;

	token = vrParseNextToken(parse);
	vrDbgPrintfN(PARSE_DBGLVL, "vrParseNextTokenPrint(): Parsed (line %d): token (%3d) #### '%s' ####\n",
		token.linenum, token.token, token.string);

	return token;
}


/************************************************************************/
/* This is just a wrapper function for vrParseNextToken() that simply   */
/*   disables the inline post-processing of tokens to determine whether */
/*   they can be further parsed.                                        */
/* Possible uses of this function is to get exactly the next string, or */
/*   get an open-paren token without doing the math simplification --   */
/*   which is desirable when trying to determine whether a string is    */
/*   a function name.                                                   */
vrTokenInfo vrParseJustNextToken(vrParseInfo *parse)
{
	vrTokenInfo	tok;

	parse->suppress_inline = 1;
	tok = vrParseNextToken(parse);
	parse->suppress_inline = 0;

	return (tok);
}


/*********************************************************************/
vrTokenInfo vrParseNextToken(vrParseInfo *parse)
{
static	char		*whitespace = " \t\r\b";
static	char		*punctuation = "(){},;\"";	/* NOTE: parens are here because they are parsed as single character tokens. */
static	char		*mathchars = "+*/=<>!|&";	/** TODO: work on parsing of "-" **/
static	char		*strchars = "_:abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static	char		*strnumchars = "_:abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.0123456789";	/* TODO: should '-' be in this list? */
	int		toklen;
	int		first_nonws;
	vrTokenInfo	eoftok = { VRTOKEN_EOF, "<eof>", -1 };
	vrTokenInfo	return_tok = { VRTOKEN_UNKNOWN, "<unk-' '>", -1 };
	vrTokenInfo	tok = { VRTOKEN_UNKNOWN, "<unk>", -1 };


	/***************************************************************/
	/** first check whether a token was already parsed and stored **/
	if (parse->preparsed != NULL) {
		vrTokenInfo	*freeme;

		/* if a token was already parsed and stored, use it */
		/* NOTE: this should only happen when a parsing process decided it */
		/*   shouldn't have read the next token, so it stores it here.     */
		tok = *(parse->preparsed);
		freeme = parse->preparsed;

		/* pop that token off the stack */
		parse->preparsed = parse->preparsed->next;
		vrShmemFree(freeme);
		vrDbgPrintfN(PARSE_DBGLVL, "vrParseNextToken: (line %d) using preparsed token **** (%3d) '%s' ****\n",
			tok.linenum, tok.token, tok.string);

		return_tok = tok;

#ifdef PARSE_CONCAT_STRINGS /* NOTE: not necessary when the string concatenation stuff has been disabled */
		/* NOTE: 5/12/03: I'm not sure that this is ever necessary, because the only */
		/*   way for this to get on the pre-parsed list is that some non-string      */
		/*   token was encountered -- I think!                                       */
		/* For now, I'll set it as a PARSE_DBGLVL message.                           */
		if (return_tok.token == VRTOKEN_STRING)
			vrDbgPrintfN(PARSE_DBGLVL, RED_TEXT "Hmmm, vrParseNextToken() pre-parsed token '%s' is a string!  We may need to check for concatenation.\n" NORM_TEXT, return_tok.string);
#endif
		return (return_tok);	/* TODO: not sure whether we can do this -- [04/04/12: well this seems to have been working the past 6 years!] */
	}

	/**********************************/
	/** otherwise, do the dirty work **/

	/* find the next non whitespace character */
	/*   NOTE: '\n' is treated differently than other whitespace in order to: */
	/*     1: advance the line number counter, and                            */
	/*     2: terminate comments.                                             */
	/*     (both of which are likely to occur when parsing strings).          */
	first_nonws = strspn(parse->next_input, whitespace);
	while ((parse->next_input[first_nonws] == '\n') ||
		(first_nonws == strlen(parse->next_input)) ||
		(parse->next_input[first_nonws] == '#')) {
	  if (parse->next_input[first_nonws] == '\n') {
		parse->linenum++;
		parse->next_input = &(parse->next_input[first_nonws+1]);
		first_nonws = strspn(parse->next_input, whitespace);
	  }

	/* advance the next_input pointer past comments (terminated by '\n') */
	  if (parse->next_input[first_nonws] == '#') {
		parse->next_input += first_nonws;
		parse->next_input += strcspn(parse->next_input, "\n");
		parse->next_input += strspn(parse->next_input, "\n");
		first_nonws = strspn(parse->next_input, whitespace);
	  }

	/* If the first non white space of the next stuff to parse is the end of */
	/*   the current input, then read more input -- if reading from a file.  */
	  if (first_nonws == strlen(parse->next_input)) {
		if (parse->fp != NULL) {
			/* Get the next non-comment, non-empty line.  If none left */
			/*   then return with EOF token.                           */
			do {
				parse->linenum++;
				eoftok.linenum = parse->linenum;
				if (fgets(parse->input, MAX_PARSE_INPUT, parse->fp) == NULL)
					return eoftok;
				first_nonws = strspn(parse->input, whitespace);
			} while (parse->input[first_nonws] == '#' || (first_nonws == strlen(parse->input)));

			/* print an error if the line was longer than we can handle */
			if (parse->input[MAX_PARSE_INPUT-1] != '\0') {
				vrFprintf(stderr, RED_TEXT "%s Parse: input line greater than maximum (%d) characters on line %d of file '%s'.\n" NORM_TEXT, parse->parse_name, MAX_PARSE_INPUT, parse->linenum, parse->filename);
				parse->input[MAX_PARSE_INPUT-1] = '\0';

			}

			/* not really nece, but makes it easier to print */
			parse->input[strlen(parse->input)-1] = '\0';

			/* start parsing at the beginning of the line */
			parse->next_input = parse->input;
		} else {
			eoftok.linenum = parse->linenum;
			return eoftok;
		}
	  }
	}

	tok.linenum = parse->linenum;
	eoftok.linenum = parse->linenum;

	parse->next_input += strspn(parse->next_input, whitespace);	/* skip whitespace */

	/***************************/
	/** parse the next string **/
	/***************************/
	if (parse->next_input[0] == '\0') {
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "hmmm, should I really get a null char as part of a token?\n" NORM_TEXT,
			parse->parse_name, tok.linenum);
		return (eoftok);

	/*********************/
	/** in-line comment **/
	} else if (parse->next_input[0] == '#') {
		tok.string = parse->next_input;
		parse->next_input += strlen(parse->next_input);
		tok.token = VRTOKEN_COMMENT;
		tok.string = "# in-line comment";

		/* recursively get the next token -- comments are white-space */
		return_tok = vrParseNextToken(parse);

	/*****************/
	/** punctuation **/
	} else if (strspn(parse->next_input, punctuation) > 0) {
		tok = vrTokenPunc(parse->next_input[0], parse);
		parse->next_input++;

		/* if we get a quote mark, then parse all the characters */
		/*   until the next quote as a string.                   */
		if (tok.token == VRTOKEN_QUOTE) {
			toklen = strcspn(parse->next_input, "\"");
			if (toklen == strlen(parse->next_input)) {
				/* no more quote marks in input is an error */
				tok.token = VRTOKEN_PARSEERROR;
				tok.string = "unmatched quote";
				return_tok = tok;
			} else {
				tok.token = VRTOKEN_STRING;
				tok.string = vrShmemAlloc(toklen+1);
				strncpy(tok.string, parse->next_input, toklen);
				tok.string[toklen] = '\0';
				parse->next_input += toklen + 1;

				return_tok = tok;
			}
		} else {
			return_tok = tok;
		}

	/*******************/
	/** math operator **/
	} else if (strspn(parse->next_input, mathchars) > 0) {
		toklen = strspn(parse->next_input, mathchars);
		tok.string = vrShmemAlloc(toklen+1);
		strncpy(tok.string, parse->next_input, toklen);
		tok.string[toklen] = '\0';
		parse->next_input += toklen;

		/* NOTE: the math operators are also part of the list of token strings, */
		/*   but it is important to parse them separately, so we don't end up   */
		/*   with long strings containing both alphanumeric and math characters.*/
		return_tok = vrTokenString(tok.string, parse);

	/** TODO: somehow we need to handle the special case of "-" which can **/
	/**   be part of a numeric expression or part of a math expression.   **/
	/**   For now, it will be limited to being part of a number.          **/
	/** NOTE: I was thinking that perhaps it might be possible to have a  **/
	/**   MINUS operator that coexists with the "-" as part of the number.**/
	/**   So, if we're expecting a math expression, then the "-" can be   **/
	/**   assumed to be that, or if we're expecting a number then it's a  **/
	/**   number -- hmmm, maybe the code isn't quite formulated that way. **/

	/************************/
	/** numeric expression **/
	} else if ((toklen = vrNumericStringLength(parse->next_input)) > 0) {
		tok.string = vrShmemAlloc(toklen+1);
		strncpy(tok.string, parse->next_input, toklen);
		tok.string[toklen] = '\0';
		parse->next_input += toklen;

		return_tok = vrTokenNumber(tok.string, parse);

	/*************************/
	/** alphanumeric string **/
	} else if (strspn(parse->next_input, strchars) > 0) {
		/* NOTE: the full string can have numbers in it, */
		/*   they just can't be first.                   */
		/*   NOTE however that we wouldn't get here if the first char was a number */
		toklen = strspn(parse->next_input, strnumchars);
		tok.string = vrShmemAlloc(toklen+1);
		strncpy(tok.string, parse->next_input, toklen);
		tok.string[toklen] = '\0';
		parse->next_input += toklen;

		return_tok = vrTokenString(tok.string, parse);

	/*******************/
	/** variable name **/
	} else if (parse->next_input[0] == '$') {
		parse->next_input++;			/* skip past the '$' */
		toklen = 0;

		if (parse->next_input[0] == '?')
			toklen = 1 + strspn(&parse->next_input[1], strnumchars);	/* include and skip past an initial '?' */
		else	toklen = strspn(parse->next_input, strnumchars);	/* NOTE: initial whitespace will give empty variable */

		tok.string = vrShmemAlloc(toklen+1);
		tok.token = VRTOKEN_VARIABLE;
		strncpy(tok.string, parse->next_input, toklen);
		tok.string[toklen] = '\0';
		parse->next_input += toklen;
		tok.linenum = parse->linenum;

		vrDbgPrintfN(VARIABLE_DBGLVL, "vrParseNextToken(): Parsed variable (line %d): token (%3d) #### '%s' ####, len = %d\n", parse->linenum, tok.token, tok.string, toklen);

		/* return not the value of the variable token, but rather the value it contains (ie. evaluates to) */
		return_tok = vrTokenEvaluateVariable(tok);
	}

	/***********************/
	/** unknown character **/

	/* If no token has been assigned, then the config file has a character that we don't */
	/*   recognize, so return as an unknown token, and advance the parsing counter.      */
	if (return_tok.token == VRTOKEN_UNKNOWN) {
		return_tok.linenum = parse->linenum;
#ifndef __linux /* TODO: for some unknown reason, Linux segfaults on this */
		return_tok.string[6] = parse->next_input[0];
#endif
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseNextToken: (line %d): "
			"Parsing '%s' as an Unknown token\n",
			return_tok.linenum, return_tok.string);

		parse->next_input++;
		return (return_tok);
	}


	/******************************************/
	/** skip inline re-evaluation of tokens? **/
	if (parse->suppress_inline)
		return (return_tok);


	/********************************/
	/** Math expression evaluation **/

	/* This is where math expressions can be evaluated as they're parsed.  If the */
	/*   current token is a number, a uni-operand math operator, or an open       */
	/*   parenthesis, then call vrParseMathExpression() otherwise consider this   */
	/*   to not be a math expression, and move on.                                */

	if (vrUniMathOperator(return_tok.token) ||
			(return_tok.token == VRTOKEN_OPENPAREN) ||
			(return_tok.token == VRTOKEN_NUMBER)
		) {
		vrTokenInfo	mathval_token;

		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseNextToken: (line %d): "
			"vrParseNextToken(): Parsed '%s' as a Math token\n",
			return_tok.linenum, return_tok.string);

		/* need to put initial token back so it will be seen by vrParseMathExpression() */
		vrParseReturnLastToken(return_tok, parse);

		mathval_token = vrParseMathExpression(parse);

		return_tok = mathval_token;
	}

	/*************************/
	/** Function evaluation **/

	/* If the token is a string, then we can compare it with the array of     */
	/*   available function names, and if we find a match, call the function. */
	if (return_tok.token == VRTOKEN_STRING) {
		vrTokenInfo	func_token = vrFunctionString(return_tok.string, parse);

		/* If the function was found and evaluated, then return */
		/*   the value without concatenation.                   */
		/* TODO: determine whether this is the behavior we want.*/
		/*   NOTE that the may still be concatenated to previous*/
		/*   strings.                                           */
		if (func_token.token != VRTOKEN_NIL) {
			vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseNextToken: (line %d): "
				"Parsed '%s' as a FunctionString token\n",
				return_tok.linenum, return_tok.string);

			return (func_token);
		}
	}

#ifdef PARSE_CONCAT_STRINGS /* NOTE: this doesn't work when separate strings are desired (eg. "setenv" implementation without '=') */
	/**************************/
	/** String concatenation **/

	/* If the token is a string, then check whether the next token is also a   */
	/*   string, and if so concatenate them together -- and do so recursively. */
	if (return_tok.token == VRTOKEN_STRING) {
		tok = vrParseNextToken(parse);

		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseNextToken: (line %d): "
			"'%s' is a string token, so looking ahead to token '%s' for possible concatenation...",
			return_tok.linenum, return_tok.string, tok.string);

		if (tok.token == VRTOKEN_STRING) {
			/* concatenate onto the return token (with a space separator) */
			return_tok.string = vrShmemStrCat(return_tok.string, tok.string);
			vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "yes, now '%s'.\n", return_tok.string);
		} else {
			/* store the next token as a pre-parsed token */
			vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "no.\n");
			vrParseReturnLastToken(tok, parse);
		}
	}
#endif

	return (return_tok);
}


/**************************************************************************/
/* vrParseReturnLastToken(): put a previously parsed token onto a token   */
/*   stack such that it has parsing priority, and will be the next item   */
/*   handled in the parsing sequence.  This is for cases where we need to */
/*   "look-ahead" in the parsing to see what action should be taken, and  */
/*   sometimes we won't use the upcoming token, so it will need to be     */
/*   addressed next.                                                      */
/* NOTE: care must be taken to avoid having the possibility of attempting */
/*   to store more than one pre-parsed token.  Currently, there are three */
/*   instances where this may be called, and they are (or are believed to */
/*   be) exclusive:                                                       */
/*   1) a math operation that may just be a numeric value, or may be      */
/*      followed by an operator.                                          */
/*   2) a string token that may be followed by another string, with the   */
/*      two being concatenated together.                                  */
/*   3) an open or close curly bracket is encountered as part of a search */
/*      for an end-of-statement indicator -- which would typically come   */
/*      about when skipping over an encountered error.  We'll still need  */
/*      to initiate or close the block associated with the bracket.       */
void vrParseReturnLastToken(vrTokenInfo token, vrParseInfo *parse)
{
	vrTokenInfo	*store_token = vrShmemAlloc0(sizeof(vrTokenInfo));

	*store_token = token;

	/* If there is already one (or more) pre-parsed tokens, */
	/*   add this one to the beginning of the list.         */
	if (parse->preparsed != NULL) {
		store_token->next = parse->preparsed;

#if 0
		vrErrPrintf("vrParseReturnLastToken: " RED_TEXT "Warning, attempting to store two pre-parsed tokens.\n" NORM_TEXT);
		vrErrPrintf("vrParseReturnLastToken: " RED_TEXT "Currently stored token is %d(%s):%d, need to also store %d(%s):%d.\n" NORM_TEXT,
			parse->preparsed.token, parse->preparsed.string, parse->preparsed.linenum,
			token.token, token.string, token.linenum);
#endif
	} else {
		store_token->next = NULL;
	}

	parse->preparsed = store_token;
	vrDbgPrintfN(PARSE_DBGLVL, "vrParseReturnLastToken: returning token **** (%3d) '%s' ****\n",
		token.token, token.string);
#if 0
vrPrintf(RED_TEXT "parse->preparsed is now %p\n", parse->preparsed);
#endif
}


/************************************************************************/
/* A math expression is:                                                */
/*      - <number>                                                      */
/* :-(  - <uni-operator> <number>                                       */
/*      - <number> <bi-operator> <number>                               */
/* :-(  - <number> <bi-operator> <expression>                           */
/* :-(  - "(" <number> <bi-operator> <expression> ")"                   */
/*	- <string> <equality|non-equality bi-operator> <string>         */
/*                                                                      */
/* Expressions currently handled by this function are:                  */
/*	- unary value                                                   */
/*	- equality comparison                                           */
/*	- inequality comparison                                         */
/*	- greater-than comparison                                       */
/*	- greater-than-or-equal comparison                              */
/*	- less-than comparison                                          */
/*	- less-than-or-equal comparison                                 */
/*	- addition                                                      */
/*	- multiplication                                                */
/*	- division                                                      */
/*	- bitwise-or                                                    */
/*	- bitwise-and                                                   */
/*	- logical-or                                                    */
/*	- logical-and                                                   */
/*                                                                      */
/* TODO: implement the <uni-operator> form.                             */
/* TODO: if an openparen is encountered, then make a recursive call.    */
/* NOTE: we need to not just parse the expression, but evaluate it too! */
/* NOTE: one problem we have is that if we get a number, and the next   */
/*   token isn't a bi-operator, then we need to put that token back!    */
/* TODO: there's a lot of redundancy in the comparison code -- probably */
/*   should write it with much less redundancy.                         */
/* NOTE: the logical comparison operations can operator on string or    */
/*   numeric values.  However, there are only limited places where this */
/*   function might be called with string values -- eg. currently the   */
/*   "if" operations.  Otherwise, the in-line numeric simplification    */
/*   parsing will not call this function for strings.                   */
/************************************************************************/
vrTokenInfo vrParseMathExpression(vrParseInfo *parse)
{
static	char		string[256];
	vrTokenInfo	value1_token;
	vrTokenInfo	value2_token;
	vrTokenInfo	operator_token;
	vrTokenInfo	close_token;
	vrTokenInfo	result_token = { VRTOKEN_NIL, "<nil>", -1 };
	int		openparen_linenum;

	vrTrace("vrParseMathExpression()", RED_TEXT "entering" NORM_TEXT);

	/*************************************/
	/* get first token of the expression */
	value1_token = vrParseNextToken(parse);

	/* if value begins with an open-paren, then evaluate the sub-expression */
	if (value1_token.token == VRTOKEN_OPENPAREN) {
		vrDbgPrintfN(PARSE_DBGLVL, "vrParseMathExpression(): " RED_TEXT "recursively calling vrParseMathExpression() for 1st argument\n" NORM_TEXT);
		openparen_linenum = value1_token.linenum;
		value1_token = vrParseMathExpression(parse);
		close_token = vrParseNextToken(parse);
		if (close_token.token != VRTOKEN_CLOSEPAREN) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): open paren is missing matching closing paren.\n", openparen_linenum);
			vrParseReturnLastToken(close_token, parse);
		}
		vrDbgPrintfN(PARSE_DBGLVL, "vrParseMathExpression(): " RED_TEXT "recursive call for 1st argument returned token string '%s'\n" NORM_TEXT, value1_token.string);
	}

	/******************************************/
	/* get possible operator, and 2nd operand */
	operator_token = vrParseNextToken(parse);

	/* if the operator is a bi-operator, then get the 2nd operand */
	if (vrBiMathOperatorToken(operator_token.token)) {
		value2_token = vrParseNextToken(parse);

		/* if value begins with an open-paren, then evaluate the sub-expression */
		if (value2_token.token == VRTOKEN_OPENPAREN) {
			vrDbgPrintfN(PARSE_DBGLVL, "vrParseMathExpression(): recursively calling vrParseMathExpression() for 2nd argument\n");
			openparen_linenum = value2_token.linenum;
			value2_token = vrParseMathExpression(parse);
			close_token = vrParseNextToken(parse);
			if (close_token.token != VRTOKEN_CLOSEPAREN) {
				vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): open paren is missing matching closing paren.\n", openparen_linenum);
				vrParseReturnLastToken(close_token, parse);
			}
			vrDbgPrintfN(PARSE_DBGLVL, "vrParseMathExpression(): " RED_TEXT "recursive call for 2nd argument returned token string '%s'\n" NORM_TEXT, value2_token.string);
		}
	} else {
		/* if the operator token wasn't a known math operator, then put it */
		/*   back into the parser as the next token, and return the first  */
		/*   value as our result (ie. a single value 'expression').        */
		vrParseReturnLastToken(operator_token, parse);
		vrDbgPrintfN(PARSE_DBGLVL, "vrParseMathExpression(<number>): 'operator' (line %d): returning token %d with value '%s'\n",
			operator_token.linenum, operator_token.token, operator_token.string);
		return (value1_token);
	}

	/* NOTE: if this were a single-value 'expression' then we should have just */
	/*   returned to the calling parse-routine, and should proceed no further. */

	/************************/
	/* now do the operation */
	vrDbgPrintfN(PARSE_DBGLVL, "vrParseMathExpression(): " RED_TEXT "performing operation: (%s %s %s)\n" NORM_TEXT,
		value1_token.string, operator_token.string, value2_token.string);

	switch (operator_token.token) {

	/********************/
	case VRTOKEN_EQUALITY:
		if (value1_token.token != VRTOKEN_NUMBER) {
			/* Do a string comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;

			/* TODO: consider whether we should ignore case */
			if (!strcmp(value1_token.string, value2_token.string))
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		} else {
			/* Do a numeric comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;
			if (atof(value1_token.string) == atof(value2_token.string))
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		}
		break;

	/**********************/
	case VRTOKEN_INEQUALITY:
		if (value1_token.token != VRTOKEN_NUMBER) {
			/* Do a string comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;

			/* TODO: consider whether we should ignore case */
			if (strcmp(value1_token.string, value2_token.string) != 0)
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		} else {
			/* Do a numeric comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;
			if (atof(value1_token.string) != atof(value2_token.string))
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		}
		break;

	/*******************/
	case VRTOKEN_GREATER:
		if (value1_token.token != VRTOKEN_NUMBER) {
			/* Do a string comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;

			/* TODO: consider whether we should ignore case */
			if (strcmp(value1_token.string, value2_token.string) > 0)
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		} else {
			/* Do a numeric comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;
			if (atof(value1_token.string) > atof(value2_token.string))
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		}
		break;

	/***********************/
	case VRTOKEN_GREATEROREQ:
		if (value1_token.token != VRTOKEN_NUMBER) {
			/* Do a string comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;

			/* TODO: consider whether we should ignore case */
			if (strcmp(value1_token.string, value2_token.string) >= 0)
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		} else {
			/* Do a numeric comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;
			if (atof(value1_token.string) >= atof(value2_token.string))
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		}
		break;

	/****************/
	case VRTOKEN_LESS:
		if (value1_token.token != VRTOKEN_NUMBER) {
			/* Do a string comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;

			/* TODO: consider whether we should ignore case */
			if (strcmp(value1_token.string, value2_token.string) < 0)
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		} else {
			/* Do a numeric comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;
			if (atof(value1_token.string) < atof(value2_token.string))
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		}
		break;

	/********************/
	case VRTOKEN_LESSOREQ:
		if (value1_token.token != VRTOKEN_NUMBER) {
			/* Do a string comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;

			/* TODO: consider whether we should ignore case */
			if (strcmp(value1_token.string, value2_token.string) <= 0)
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		} else {
			/* Do a numeric comparison */
			result_token.token = VRTOKEN_NUMBER;
			result_token.linenum = operator_token.linenum;
			if (atof(value1_token.string) <= atof(value2_token.string))
				result_token.string = strdup("1");
			else	result_token.string = strdup("0");
		}
		break;

	/****************/
	case VRTOKEN_PLUS:
		/* NOTE: it might be reasonable to "add" two strings, returning  */
		/*   a longer string, but we're not going to implement that now. */
		if (value1_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The addition operator (currently) expects the first operand to be a number -- got '%s'\n",
				value1_token.linenum, value1_token.string);
		}
		if (value2_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The addition operator (currently) expects the second operand to be a number -- got '%s'\n",
				value2_token.linenum, value2_token.string);
		}
		if ((value1_token.token == VRTOKEN_NUMBER) && (value2_token.token == VRTOKEN_NUMBER)) {
			snprintf(string, sizeof(string), "%f", (atof(value1_token.string) + atof(value2_token.string)));
			result_token.token = VRTOKEN_NUMBER;
			result_token.string = strdup(string);
			result_token.linenum = operator_token.linenum;
		}
		break;

	/*****************/
	case VRTOKEN_TIMES:
		if (value1_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The multiplication operator (currently) expects the first operand to be a number -- got '%s'\n",
				value1_token.linenum, value1_token.string);
		}
		if (value2_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The multiplication operator (currently) expects the second operand to be a number -- got '%s'\n",
				value2_token.linenum, value2_token.string);
		}
		if ((value1_token.token == VRTOKEN_NUMBER) && (value2_token.token == VRTOKEN_NUMBER)) {
			snprintf(string, sizeof(string), "%f", (atof(value1_token.string) * atof(value2_token.string)));
			result_token.token = VRTOKEN_NUMBER;
			result_token.string = strdup(string);
			result_token.linenum = operator_token.linenum;
		}
		break;

	/******************/
	case VRTOKEN_DIVIDE:
		if (value1_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The division operator (currently) expects the first operand to be a number -- got '%s'\n",
				value1_token.linenum, value1_token.string);
		}
		if (value2_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The division operator (currently) expects the second operand to be a number -- got '%s'\n",
				value2_token.linenum, value2_token.string);
		}
		if ((value1_token.token == VRTOKEN_NUMBER) && (value2_token.token == VRTOKEN_NUMBER)) {
			snprintf(string, sizeof(string), "%f", (atof(value1_token.string) / atof(value2_token.string)));
			result_token.token = VRTOKEN_NUMBER;
			result_token.string = strdup(string);
			result_token.linenum = operator_token.linenum;
		}
		break;

	/*******************/
	case VRTOKEN_BIT_AND:
		if (value1_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The bitwise-and operator (currently) expects the first operand to be a number -- got '%s'\n",
				value1_token.linenum, value1_token.string);
		}
		if (value2_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The bitwise-and operator (currently) expects the second operand to be a number -- got '%s'\n",
				value2_token.linenum, value2_token.string);
		}
		if ((value1_token.token == VRTOKEN_NUMBER) && (value2_token.token == VRTOKEN_NUMBER)) {
			snprintf(string, sizeof(string), "%ld", (vrAtoI(value1_token.string) & vrAtoI(value2_token.string)));
			result_token.token = VRTOKEN_NUMBER;
			result_token.string = strdup(string);
			result_token.linenum = operator_token.linenum;
		}
		break;

	/******************/
	case VRTOKEN_BIT_OR:
		if (value1_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The bitwise-or operator (currently) expects the first operand to be a number -- got '%s'\n",
				value1_token.linenum, value1_token.string);
		}
		if (value2_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The bitwise-or operator (currently) expects the second operand to be a number -- got '%s'\n",
				value2_token.linenum, value2_token.string);
		}
		if ((value1_token.token == VRTOKEN_NUMBER) && (value2_token.token == VRTOKEN_NUMBER)) {
			snprintf(string, sizeof(string), "%ld", (vrAtoI(value1_token.string) | vrAtoI(value2_token.string)));
			result_token.token = VRTOKEN_NUMBER;
			result_token.string = strdup(string);
			result_token.linenum = operator_token.linenum;
		}
		break;

	/***********************/
	case VRTOKEN_LOGICAL_AND:
		if (value1_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The logical-and operator (currently) expects the first operand to be a number -- got '%s'\n",
				value1_token.linenum, value1_token.string);
		}
		if (value2_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The logical-and operator (currently) expects the second operand to be a number -- got '%s'\n",
				value2_token.linenum, value2_token.string);
		}
		if ((value1_token.token == VRTOKEN_NUMBER) && (value2_token.token == VRTOKEN_NUMBER)) {
			snprintf(string, sizeof(string), "%d", (vrAtoI(value1_token.string) && vrAtoI(value2_token.string)));
			result_token.token = VRTOKEN_NUMBER;
			result_token.string = strdup(string);
			result_token.linenum = operator_token.linenum;
		}
		break;

	/**********************/
	case VRTOKEN_LOGICAL_OR:
		if (value1_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The logical-or operator (currently) expects the first operand to be a number -- got '%s'\n",
				value1_token.linenum, value1_token.string);
		}
		if (value2_token.token != VRTOKEN_NUMBER) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): The logical-or operator (currently) expects the second operand to be a number -- got '%s'\n",
				value2_token.linenum, value2_token.string);
		}
		if ((value1_token.token == VRTOKEN_NUMBER) && (value2_token.token == VRTOKEN_NUMBER)) {
			snprintf(string, sizeof(string), "%d", (vrAtoI(value1_token.string) || vrAtoI(value2_token.string)));
			result_token.token = VRTOKEN_NUMBER;
			result_token.string = strdup(string);
			result_token.linenum = operator_token.linenum;
		}
		break;

	/******/
	default:
		/* We shouldn't get here, because non-bi-operators should have */
		/*   been returned before the switch statement.                */
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrParseMathExpression(<number>): (line %d): "
			RED_TEXT "Library ERROR: Unknown bi-operator\n" NORM_TEXT,
			operator_token.linenum, operator_token.token, operator_token.string);
		return (value1_token);
	}

	vrDbgPrintfN(PARSE_DBGLVL, "vrParseMathExpression(<num> <op> <num>): (line %d): returning token %d with value '%s'\n",
		result_token.linenum, result_token.token, result_token.string);

	return (result_token);
}


/**********************************/
/*** Generic expression parsing ***/
/**********************************/


/*********************************************************************/
/* parses an expression to get a single string, and returns that string. */
/* Format: "<setting>" "=" string[;] */
vrTokenInfo vrParseSingleStringExpr(char **retvalue, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	assignment_tok;
	vrTokenInfo	value_token;
	vrTokenInfo	terminal_token;
#ifdef UNPICKY_COMPILER
	char		*value;
#endif

	vrTrace("vrParseSingleStringExpr()", RED_TEXT "entering" NORM_TEXT);

	/*****************************/
	/** operator ( "=" | "+=" ) **/
	assignment_tok = vrParseNextToken(parse);

	/***********/
	/** value **/
	value_token = vrParseNextToken(parse);
	if (value_token.token == VRTOKEN_NIL) {
		/* probably have a non-existent variable, so don't do the assignment */
		assignment_tok.token = VRTOKEN_NIL;
		assignment_tok.string = "<nil>";
	}

	/***********************/
	/** do the assignment **/
	switch(assignment_tok.token) {
	case VRTOKEN_SETEQUALS:
		*retvalue = NULL;
		break;

	case VRTOKEN_PLUSEQUALS:
		/* leave retvalue alone */
		break;

	case VRTOKEN_NIL:	/* TODO: if we ever create a VRTOKEN_NOP, change this to it */
		break;

	case VRTOKEN_TIMESEQUALS:
	default:
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid assignment operator (\"%s\") for %s.\n" NORM_TEXT,
			parse->parse_name, assignment_tok.linenum, assignment_tok.string, setting);

		terminal_token = vrParseToEOS(parse);
		vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
			terminal_token.linenum, terminal_token.string);

		vrTrace("vrParseSingleStringExpr()", RED_TEXT "exiting from assignment error" NORM_TEXT);
		return terminal_token;
	}

	if (value_token.token == VRTOKEN_STRING || value_token.token == VRTOKEN_NUMBER) {
		if (*retvalue == NULL)
			*retvalue = vrShmemStrDup(value_token.string);
		else	*retvalue = vrShmemStrCat(*retvalue, value_token.string);
	} else {
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid %s given -- value is not a string or number.\n" NORM_TEXT,
			parse->parse_name, value_token.linenum, setting);
	}

	if (assignment_tok.token == VRTOKEN_NIL) {
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseSingleStringExpr(): "
			BOLD_TEXT "leaving '%s' as value '%s'," NORM_TEXT
			" assignment token = '%s' (line %d), value token = '%s' (line %d)\n",
			setting, *retvalue,
			assignment_tok.string, assignment_tok.linenum,
			value_token.string, value_token.linenum);
	} else {
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseSingleStringExpr(): "
			BOLD_TEXT "setting '%s' to value '%s'," NORM_TEXT
			" assignment token = '%s' (line %d), value token = '%s' (line %d)\n",
			setting, *retvalue,
			assignment_tok.string, assignment_tok.linenum,
			value_token.string, value_token.linenum);
	}

	/***************************/			/* '{' to match next line */
	/** ( ";" | "}" | <eof> ) **/
	terminal_token = vrParseNextToken(parse);
	if (!vrTokenTerminal(terminal_token.token)) {
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "%s takes only one argument.  (I.e. expected a terminal token, but got '%s'(%d).)\n" NORM_TEXT,
			parse->parse_name, terminal_token.linenum, setting,
			terminal_token.string, terminal_token.token);
	}

	vrTrace("vrParseSingleStringExpr()", RED_TEXT "exiting" NORM_TEXT);
	return terminal_token;
}


/*********************************************************************/
/* parses an expression to get an enumerated string. */
/* Format: "<setting>" "=" string[;] */
vrTokenInfo vrParseSingleEnumerator(int *retvalue, vrParseEnumeratorString stringparser, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	assignment_tok;
	vrTokenInfo	value_token;
	vrTokenInfo	terminal_token;
	int		value;

	vrTrace("vrParseSingleEnumeratorExpr()", RED_TEXT "entering" NORM_TEXT);

	/**********************/
	/** operator ( "=" ) **/
	assignment_tok = vrParseNextToken(parse);

	/*********************************/
	/** enumeration value of string **/
	value_token = vrParseNextToken(parse);	/* first, get the value */
	switch(value_token.token) {
	case VRTOKEN_STRING:
		value = (*stringparser)(value_token.string);
		if (value == -2) {} /* TODO: print warning message  -- also make parsers return -2 */
		break;
	case VRTOKEN_NUMBER:
		value = vrAtoI(value_token.string);
		break;
	case VRTOKEN_NIL:
		/* probably have a non-existent variable, so don't do the assignment */
		assignment_tok.token = VRTOKEN_NIL;
		assignment_tok.string = "<nil>";
		break;
	default:
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid %s given.\n" NORM_TEXT,
			parse->parse_name, value_token.linenum, setting);
	}

	/***********************/
	/** do the assignment **/
	switch(assignment_tok.token) {
	case VRTOKEN_SETEQUALS:
		*retvalue = value;
		break;
	case VRTOKEN_NIL:	/* TODO: if we ever create a VRTOKEN_NOP, change this to it */
		break;
	case VRTOKEN_PLUSEQUALS:
	case VRTOKEN_TIMESEQUALS:
	default:
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid assignment operator (\"%s\") for %s.\n" NORM_TEXT,
			parse->parse_name, assignment_tok.linenum, assignment_tok.string, setting);

		if (!vrTokenTerminal(value_token.token)) {
			terminal_token = vrParseToEOS(parse);
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				terminal_token.linenum, terminal_token.string);
		} else {
			terminal_token = value_token;
		}
		vrTrace("vrParseSingleEnumerator()", RED_TEXT "exiting from assignment error" NORM_TEXT);
		return terminal_token;
	}

	if (assignment_tok.token == VRTOKEN_NIL) {
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseSingleEnumerator(): "
			BOLD_TEXT "leaving '%s' as value %d," NORM_TEXT
			" assignment token = '%s' (line %d), value token = '%s' (line %d)\n",
			setting, value,
			assignment_tok.string, assignment_tok.linenum,
			value_token.string, value_token.linenum);
	} else {
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseSingleEnumerator(): "
			BOLD_TEXT "setting '%s' to value %d," NORM_TEXT
			" assignment token = '%s' (line %d), value token = '%s' (line %d)\n",
			setting, value,
			assignment_tok.string, assignment_tok.linenum,
			value_token.string, value_token.linenum);
	}

	/***************************/	/* { -- to balance the one in the line below */
	/** ( ";" | "}" | <eof> ) **/
	terminal_token = vrParseNextToken(parse);
	if (!vrTokenTerminal(terminal_token.token))
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "%s takes only one argument.\n" NORM_TEXT,
			parse->parse_name, terminal_token.linenum, setting);


	vrTrace("vrParseSingleEnumeratorExpr()", RED_TEXT "exiting" NORM_TEXT);
	return terminal_token;
}


/*********************************************************************/
/* return an array of strings */
/* NOTE: unlike the numeric list-expression parsers, the string-list expression parser  */
/*   will set an unknown number of strings in the list based on the number given in the */
/*   config file.  Also, unlike all the single expression parsers, this function will   */
/*   go ahead and set values even when there are unavailable variables.  In these cases */
/*   there the parser skips over the non-existent variables as though they weren't in   */
/*   the list.                                                                          */
vrTokenInfo vrParseStringListExpr(char ***array, int *array_len, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	assignment_tok;
	vrTokenInfo	value_token;
	vrTokenInfo	terminal_token;
	int		count;
	int		num_strings = 0;
	char		**newarray = NULL;
	char		**oldarray;

	vrTrace("vrParseStringListExpr()", RED_TEXT "entering" NORM_TEXT);

	/********************/
	/** ( "=" | "+=" ) **/
	assignment_tok = vrParseNextToken(parse);
	if (assignment_tok.token != VRTOKEN_SETEQUALS && assignment_tok.token != VRTOKEN_PLUSEQUALS) {
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Expecting an assignment operator for System Procs.\n" NORM_TEXT,
			parse->parse_name, assignment_tok.linenum);

		terminal_token = vrParseToEOS(parse);
		vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
			terminal_token.linenum, terminal_token.string);

		vrTrace("vrParseStringListExpr()", RED_TEXT "exiting from assignment error" NORM_TEXT);
		return terminal_token;
	}

	if (assignment_tok.token == VRTOKEN_PLUSEQUALS) {
		/* this line causes the list to be appended to an existing list */
		/* otherwise, newarray remains null, and a new list is begun.   */
		newarray = *array;
		num_strings = *array_len;
	}

	/*******************************/
	/** get all the string tokens **/
	value_token = vrParseNextToken(parse);
	while (vrTokenValue(value_token.token)) {
		switch (value_token.token) {
		case VRTOKEN_STRING:	/* process all string tokens */
		case VRTOKEN_NUMBER:	/* process all numeric tokens (basically as though a string) */
			num_strings++;

			/**************************************/
			/** do the assignment for this token **/

			/* for the moment, the newarray is extended using  */
			/*   the brute-force method of just creating another   */
			/*   array that is one element longer, and copying all */
			/*   the old value into it, and adding the new value.  */
			oldarray = newarray;
			newarray = (char **)vrShmemAlloc0(num_strings * sizeof(char *));

			for (count = 0; count < num_strings-1; count++) {
				newarray[count] = oldarray[count];
				vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseStringListExpr(): copied '%s' (%d)\n", newarray[count], count);
			}
			newarray[num_strings-1] = vrShmemStrDup(value_token.string);
			vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseStringListExpr(): added '%s' (%d)\n", newarray[num_strings-1], num_strings-1);
			vrShmemFree(oldarray);
			break;

		case VRTOKEN_NIL:	/* just skip variables that weren't set, then get next token */
			break;

		default:		/* obviously this shouldn't happen -- see while-statement condition */
			break;
		}


		/************************/
		/** get the next token **/
		terminal_token = vrParseNextToken(parse);
		switch (terminal_token.token) {
		default:
			vrErrPrintf("%s Parse Error (line %d): "
				RED_TEXT "Expected a comma or semicolon, but got \"%s\".\n" NORM_TEXT,
				parse->parse_name, terminal_token.linenum, terminal_token.string);
		case VRTOKEN_SEMICOLON:
		case VRTOKEN_CLOSECURL:
		case VRTOKEN_EOF:
			value_token = terminal_token;	/* without this the while-loop won't terminate */
			break;
		case VRTOKEN_COMMA:
			value_token = vrParseNextToken(parse);
			break;
		}
	}
	*array_len = num_strings;
	*array = newarray;

	if (vrDbgDo(PARSE_DETAIL_DBGLVL)) {
		vrPrintf("vrParseStringListExpr(): " BOLD_TEXT "setting '%s' to list [", setting);
		for (count = 0; count < *array_len; count++) {
			vrPrintf( BOLD_TEXT "'%s' ", (*array)[count]);
		}
		vrPrintf(BOLD_TEXT "\b]\n" NORM_TEXT
			"\t-> assignment token = '%s' (line %d), last value token = '%s' (line %d)\n",
			assignment_tok.string, assignment_tok.linenum,
			value_token.string, value_token.linenum);
	}


	vrTrace("vrParseStringListExpr()", RED_TEXT "exiting" NORM_TEXT);
	return (terminal_token);
}


/*********************************************************************/
/* parses an expression to get a single integer, and returns that integer. */
/* Format: "<setting>" "=" number[;] */
vrTokenInfo vrParseSingleIntegerExpr(int *retvalue, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	assignment_tok;
	vrTokenInfo	value_token;
	vrTokenInfo	terminal_token;
	int		value;

	vrTrace("vrParseSingleIntegerExpr()", RED_TEXT "entering" NORM_TEXT);

	/************************************/
	/** operator ( "=" | "+=" | "*=" ) **/
	assignment_tok = vrParseNextToken(parse);

	/***********/
	/** value **/
	value_token = vrParseNextToken(parse);
	switch (value_token.token) {
	case VRTOKEN_NUMBER:
		/* if value_token is a number, then convert to the integer value */
		value = vrAtoI(value_token.string);
		break;
	case VRTOKEN_NIL:
		/* probably have a non-existent variable, so don't do the assignment */
		assignment_tok.token = VRTOKEN_NIL;
		assignment_tok.string = "<nil>";
		break;
	case VRTOKEN_STRING:
		/* if value_token is a string, then see whether it has a numeric value */
		value = vrParseStringAsNumber(value_token.string);
		if (value != VRTOKEN_UNKNOWN) {
			break;
		}
		/* otherwise, we fall through into the error message of the "default" case */
	default:
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid %s given.\n" NORM_TEXT,
			parse->parse_name, value_token.linenum, setting);
		if (!vrTokenTerminal(value_token.token)) {
			value_token = vrParseToEOS(parse);
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				value_token.linenum, value_token.string);
		}
	}

	/***********************/
	/** do the assignment **/
	switch(assignment_tok.token) {
	case VRTOKEN_SETEQUALS:
		*retvalue = value;
		break;
	case VRTOKEN_PLUSEQUALS:
		*retvalue += value;
		break;
	case VRTOKEN_TIMESEQUALS:
		*retvalue *= value;
		break;
	case VRTOKEN_NIL:	/* TODO: if we ever create a VRTOKEN_NOP, change this to it */
		break;
	default:
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid assignment operator (\"%s\") for %s.\n" NORM_TEXT,
			parse->parse_name, assignment_tok.linenum, assignment_tok.string, setting);

		if (!vrTokenTerminal(value_token.token)) {
			terminal_token = vrParseToEOS(parse);
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				terminal_token.linenum, terminal_token.string);
		} else
			terminal_token = value_token;
		vrTrace("vrParseSingleIntegerExpr()", RED_TEXT "exiting from assignment error" NORM_TEXT);
		return terminal_token;
	}

	if (assignment_tok.token == VRTOKEN_NIL) {
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseSingleIntegerExpr(): "
			BOLD_TEXT "leaving '%s' as value %d," NORM_TEXT
			" assignment token = '%s' (line %d), value token = '%s' (line %d)\n",
			setting, *retvalue,
			assignment_tok.string, assignment_tok.linenum,
			value_token.string, value_token.linenum);
	} else {
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseSingleIntegerExpr(): "
			BOLD_TEXT "setting '%s' to value %d," NORM_TEXT
			" assignment token = '%s' (line %d), value token = '%s' (line %d)\n",
			setting, *retvalue,
			assignment_tok.string, assignment_tok.linenum,
			value_token.string, value_token.linenum);
	}

	/***************************/	/* { -- to balance the one in the line below */
	/** ( ";" | "}" | <eof> ) **/
	terminal_token = vrParseNextToken(parse);
	if (!vrTokenTerminal(terminal_token.token))
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "%s takes only one argument.\n" NORM_TEXT,
			parse->parse_name, terminal_token.linenum, setting);

	vrTrace("vrParseSingleIntegerExpr()", RED_TEXT "exiting" NORM_TEXT);
	return terminal_token;
}


/*********************************************************************/
/* NOTE: the numeric list expression parsers generally require a specific number of */
/*   values.  A warning message is printed when the number of given numbers doesn't */
/*   match the expected list length -- however this is not a hard error.  On the    */
/*   other hand, any string that is parsed will be sent to find the string value,   */
/*   and any missing variable will be replaced with a zero value -- which doesn't   */
/*   follow the standard of the other expression parsers (except the other number   */
/*   lists.                                                                         */
vrTokenInfo vrParseIntegerListExpr(int *array, int maxlen, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	assignment_tok;
	vrTokenInfo	value_token;
	vrTokenInfo	terminal_token;
	int		value;
	int		count;

	vrTrace("vrParseIntegerListExpr()", RED_TEXT "entering" NORM_TEXT);

	/***************************/
	/** ( "=" | "+=" | "*=" ) **/
	assignment_tok = vrParseNextToken(parse);

	/*******************/
	/** number values **/
	value_token = vrParseNextToken(parse);
#if 0 /* TODO: I'm not sure this is necessary, and even it if is, the test should be for vrTokenValue(value_token.token) */
	if (value_token.token != VRTOKEN_NUMBER) {
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid %s given.\n" NORM_TEXT,
			parse->parse_name, value_token.linenum, setting);
	}
#endif
	for (count = 0; count < maxlen && vrTokenValue(value_token.token); count++) {
		switch (value_token.token) {
		case VRTOKEN_NUMBER:
			value = vrAtoI(value_token.string);
			break;
		case VRTOKEN_STRING:
			switch (vrParseStringAsNumber(value_token.string)) {
			case VRTOKEN_NO:
				value = 0;
				break;
			case VRTOKEN_YES:
				value = 1;
				break;
			default:
				vrErrPrintf("%s Parse Warning (line %d): "
					RED_TEXT "Non-numeric string '%s' given as number.\n" NORM_TEXT,
					parse->parse_name, value_token.linenum, value_token.string);
				value_token.token = VRTOKEN_NIL;	/* set to <nil> so no assignment will be done */
				break;
			}
			break;
		case VRTOKEN_NIL:
			break;
		}

		/***********************/
		/** do the assignment **/
		/* NOTE: assignments are not made when the value is the <nil> token, so old values remain */
		switch(assignment_tok.token) {
		case VRTOKEN_SETEQUALS:
			if (value_token.token != VRTOKEN_NIL)
				array[count] = value;
			break;
		case VRTOKEN_PLUSEQUALS:
			if (value_token.token != VRTOKEN_NIL)
				array[count] += value;
			break;
		case VRTOKEN_TIMESEQUALS:
			if (value_token.token != VRTOKEN_NIL)
				array[count] *= value;
			break;
		default:
			vrErrPrintf("%s Parse Error (line %d): "
				RED_TEXT "Invalid assignment operator (\"%s\") for %s.\n" NORM_TEXT,
				parse->parse_name, assignment_tok.linenum, assignment_tok.string, setting);

#if 0 /* the condition of this if-statement can never be true (05/25/2006: "true"? or false?  Seems like the latter makes more sense. */
			if (!vrTokenTerminal(value_token.token)) {
#endif
				terminal_token = vrParseToEOS(parse);
				vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
					terminal_token.linenum, terminal_token.string);
#if 0 /* the condition of this if-statement can never be true */
			} else {
				terminal_token = value_token;
			}
#endif
			vrTrace("vrParseIntegerListExpr()", RED_TEXT "exiting from assignment error" NORM_TEXT);
			return terminal_token;
		}

		/*********************************/	/* { -- to balance the one in the line below */
		/** ( "," | ";" | "}" | <eof> ) **/
		terminal_token = vrParseNextToken(parse);
		switch (terminal_token.token) {
		case VRTOKEN_COMMA:
			value_token = vrParseNextToken(parse);
			break;
		case VRTOKEN_SEMICOLON:
		case VRTOKEN_CLOSECURL:
		case VRTOKEN_EOF:
			value_token = terminal_token;
			continue;
		default:
			vrErrPrintf("%s Parse Error (line %d): "
				RED_TEXT "%s: Expected a comma or statement terminator.\n" NORM_TEXT,
				parse->parse_name, terminal_token.linenum, setting);
			value_token = terminal_token;
		}
	}

	if (count < maxlen)
		vrErrPrintf("%s Parse Warning (line %d): "
			RED_TEXT "%s: Less than the full list of %d numbers given.\n" NORM_TEXT,
			parse->parse_name, value_token.linenum, setting, maxlen);

	if (vrDbgDo(PARSE_DETAIL_DBGLVL)) {
		vrPrintf("vrParseIntegerListExpr(): " BOLD_TEXT "setting '%s' to list [", setting);
		for (count = 0; count < maxlen; count++) {
			vrPrintf( BOLD_TEXT "%d ", array[count]);
		}
		vrPrintf(BOLD_TEXT "\b]" NORM_TEXT
			" assignment token = '%s' (line %d), last value token = '%s' (line %d)\n",
			assignment_tok.string, assignment_tok.linenum,
			value_token.string, value_token.linenum);
	}

	vrTrace("vrParseIntegerListExpr()", RED_TEXT "exiting" NORM_TEXT);
	return terminal_token;
}


/*********************************************************************/
/* parses an expression to get a single float, and returns that float. */
/* Format: "<setting>" "=" number[;] */
vrTokenInfo vrParseSingleFloatExpr(float *retvalue, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	assignment_tok;
	vrTokenInfo	value_token;
	vrTokenInfo	terminal_token;
	float		value;

	vrTrace("vrParseSingleFloatExpr()", RED_TEXT "entering" NORM_TEXT);

	/************************************/
	/** operator ( "=" | "+=" | "*=" ) **/
	assignment_tok = vrParseNextToken(parse);

	/***********/
	/** value **/
	value_token = vrParseNextToken(parse);
	switch (value_token.token) {
	case VRTOKEN_NUMBER:
		/* if value_token is a number, then convert to the float value */
		value = atof(value_token.string);
		break;
	case VRTOKEN_NIL:
		/* probably have a non-existent variable, so don't do the assignment */
		assignment_tok.token = VRTOKEN_NIL;
		assignment_tok.string = "<nil>";
		break;
	case VRTOKEN_STRING:
		value = vrParseStringAsNumber(value_token.string);
		if (value != VRTOKEN_UNKNOWN) {
			break;
		}
		/* otherwise, we fall through into the error message of the "default" case */
	default:
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid %s given.\n" NORM_TEXT,
			parse->parse_name, value_token.linenum, setting);

		/* at this point we're in a messed-up statement, so if we're not at a terminator, read read of statement */
		if (!vrTokenTerminal(value_token.token)) {
			value_token = vrParseToEOS(parse);
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				value_token.linenum, value_token.string);
		}
	}

	/***********************/
	/** do the assignment **/
	switch(assignment_tok.token) {
	case VRTOKEN_SETEQUALS:
		*retvalue = value;
		break;
	case VRTOKEN_PLUSEQUALS:
		*retvalue += value;
		break;
	case VRTOKEN_TIMESEQUALS:
		*retvalue *= value;
		break;
	case VRTOKEN_NIL:	/* TODO: if we ever create a VRTOKEN_NOP, change this to it */
		break;
	default:
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid assignment operator (\"%s\") for %s.\n" NORM_TEXT,
			parse->parse_name, assignment_tok.linenum, assignment_tok.string, setting);

		if (!vrTokenTerminal(value_token.token)) {
			terminal_token = vrParseToEOS(parse);
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				terminal_token.linenum, terminal_token.string);
		} else
			terminal_token = value_token;
		vrTrace("vrParseSingleFloatExpr()", RED_TEXT "exiting from assignment error" NORM_TEXT);
		return terminal_token;
	}

	if (assignment_tok.token == VRTOKEN_NIL) {
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseSingleFloatExpr(): "
			BOLD_TEXT "leaving '%s' as value %.4f," NORM_TEXT
			" assignment token = '%s' (line %d), value token = '%s' (line %d)\n",
			setting, *retvalue,
			assignment_tok.string, assignment_tok.linenum,
			value_token.string, value_token.linenum);
	} else {
		vrDbgPrintfN(PARSE_DETAIL_DBGLVL, "vrParseSingleFloatExpr(): "
			BOLD_TEXT "setting '%s' to value %.4f," NORM_TEXT
			" assignment token = '%s' (line %d), value token = '%s' (line %d)\n",
			setting, *retvalue,
			assignment_tok.string, assignment_tok.linenum,
			value_token.string, value_token.linenum);
	}

	/***************************/	/* { -- to balance the one in the line below */
	/** ( ";" | "}" | <eof> ) **/
	terminal_token = vrParseNextToken(parse);
	if (!vrTokenTerminal(terminal_token.token))
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "%s takes only one argument.\n" NORM_TEXT,
			parse->parse_name, terminal_token.linenum, setting);

	vrTrace("vrParseSingleFloatExpr()", RED_TEXT "exiting" NORM_TEXT);
	return terminal_token;
}


/*********************************************************************/
vrTokenInfo vrParseFloatListExpr(float *array, int maxlen, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	assignment_tok;

#if 1 /* 12/17/02 -- test the necessity of the duplicate function by setting to 0 */
	/* TODO: [1/4/2] why do we do this?  It seems this isn't necessary. */

	return vrParseFloatListExprAssign(array, maxlen, &assignment_tok, setting, parse);
}


/*********************************************************************/
/* NOTE: the numeric list expression parsers generally require a specific number of */
/*   values.  A warning message is printed when the number of given numbers doesn't */
/*   match the expected list length -- however this is not a hard error.  On the    */
/*   other hand, any string that is parsed will be sent to find the string value,   */
/*   and any missing variable will be replaced with a zero value -- which doesn't   */
/*   follow the standard of the other expression parsers (except the other number   */
/*   lists.                                                                         */
vrTokenInfo vrParseFloatListExprAssign(float *array, int maxlen, vrTokenInfo *assignment_tok, char *setting, vrParseInfo *parse)
{
#endif
	vrTokenInfo	value_token;
	vrTokenInfo	terminal_token;
	float		value;
	int		count;

	vrTrace("vrParseFloatListExprAssign()", RED_TEXT "entering" NORM_TEXT);

	/***************************/
	/** ( "=" | "+=" | "*=" ) **/
	*assignment_tok = vrParseNextToken(parse);

	/************/
	/** values **/
	value_token = vrParseNextToken(parse);
#if 0 /* TODO: I'm not sure this is necessary, and even it if is, the test should be for vrTokenValue(value_token.token) */
	if (token.token != VRTOKEN_NUMBER) {
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid %s given.\n" NORM_TEXT,
			parse->parse_name, token.linenum, setting);
	}
#endif
	for (count = 0; count < maxlen && vrTokenValue(value_token.token); count++) {
		switch (value_token.token) {
		case VRTOKEN_NUMBER:
			value = atof(value_token.string);
			break;
		case VRTOKEN_STRING:
			switch (vrParseStringAsNumber(value_token.string)) {
			case VRTOKEN_NO:
				value = 0.0;
				break;
			case VRTOKEN_YES:
				value = 1.0;
				break;
			default:
				vrErrPrintf("%s Parse Warning (line %d): "
					RED_TEXT "Non-numeric string '%s' given as number.\n" NORM_TEXT,
					parse->parse_name, value_token.linenum, value_token.string);
				value_token.token = VRTOKEN_NIL;	/* set to <nil> so no assignment will be done */
				break;
			}
			break;
		case VRTOKEN_NIL:
			break;
		}

		/***********************/
		/** do the assignment **/
		/* NOTE: assignments are not made when the value is the <nil> token, so old values remain */
		switch(assignment_tok->token) {
		case VRTOKEN_SETEQUALS:
			if (value_token.token != VRTOKEN_NIL)
				array[count] = value;
			break;
		case VRTOKEN_PLUSEQUALS:
			if (value_token.token != VRTOKEN_NIL)
				array[count] += value;
			break;
		case VRTOKEN_TIMESEQUALS:
			if (value_token.token != VRTOKEN_NIL)
				array[count] *= value;
			break;
		default:
			vrErrPrintf("%s Parse Error (line %d): "
				RED_TEXT "Invalid assignment operator (\"%s\") for %s.\n" NORM_TEXT,
				parse->parse_name, assignment_tok->linenum, assignment_tok->string, setting);

			terminal_token = vrParseToEOS(parse);
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				terminal_token.linenum, terminal_token.string);

			vrTrace("vrParseFloatListExprAssign()", RED_TEXT "exiting from assignment error" NORM_TEXT);
			return terminal_token;
		}

		/*********************************/	/* { -- to balance the one in the line below */
		/** ( "," | ";" | "}" | <eof> ) **/
		terminal_token = vrParseNextToken(parse);
		switch (terminal_token.token) {
		case VRTOKEN_COMMA:
			value_token = vrParseNextToken(parse);
			break;
		case VRTOKEN_SEMICOLON:
		case VRTOKEN_CLOSECURL:
		case VRTOKEN_EOF:
			value_token = terminal_token;
			continue;
		default:
			vrErrPrintf("%s Parse Error (line %d): "
				RED_TEXT "%s: Expected a comma or statement terminator.\n" NORM_TEXT,
				parse->parse_name, terminal_token.linenum, setting);
			value_token = terminal_token;
		}
	}

	if (count < maxlen)
		vrErrPrintf("%s Parse Warning (line %d): "
			RED_TEXT "%s: Less than the full list of %d numbers given.\n" NORM_TEXT,
			parse->parse_name, value_token.linenum, setting, maxlen);

	if (vrDbgDo(PARSE_DETAIL_DBGLVL)) {
		vrPrintf("vrParseFloatListExprAssign(): " BOLD_TEXT "setting '%s' to list [", setting);
		for (count = 0; count < maxlen; count++) {
			vrPrintf( BOLD_TEXT "%.4f ", array[count]);
		}
		vrPrintf(BOLD_TEXT "\b]" NORM_TEXT
			" assignment token = '%s' (line %d), last value token = '%s' (line %d)\n",
			assignment_tok->string, assignment_tok->linenum,
			value_token.string, value_token.linenum);
	}

	vrTrace("vrParseFloatListExprAssign()", RED_TEXT "exiting" NORM_TEXT);
	return terminal_token;
}


/*********************************************************************/
/* TODO: [1/4/2] why this this function take 'assignment_tok' as an argument, and vrParseFloatListExpr() doesn't??? */
/*   Answer: because this (these?) are called by the Matrix parsing functions below */
/* This function parses the assignment token, and all the (maxlen) numbers that go   */
/*   with a particular type of operation.  However, the operation itself is not      */
/*   performed here.  Thus, many different types of operations can use this function */
/*   to get the basic inputs, and then perform the appropriate operation.            */
vrTokenInfo vrParseDoubleListExprAssign(double *array, int maxlen, vrTokenInfo *assignment_tok, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	value_token;
	vrTokenInfo	terminal_token;
	double		value;
	int		count;

	vrTrace("vrParseDoubleListExpr()", RED_TEXT "entering" NORM_TEXT);

	/***************************/
	/** ( "=" | "+=" | "*=" ) **/
	*assignment_tok = vrParseNextToken(parse);

	/************/
	/** values **/
	value_token = vrParseNextToken(parse);
#if 0 /* TODO: I'm not sure this is necessary, and even it if is, the test should be for vrTokenValue(value_token.token) */
	if (token.token != VRTOKEN_NUMBER) {
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid %s given.\n" NORM_TEXT,
			parse->parse_name, token.linenum, setting);
	}
#endif
	for (count = 0; count < maxlen && vrTokenValue(value_token.token); count++) {
		switch (value_token.token) {
		case VRTOKEN_NUMBER:
			value = atof(value_token.string);
			break;
		case VRTOKEN_STRING:
			switch (vrParseStringAsNumber(value_token.string)) {
			case VRTOKEN_NO:
				value = 0.0;
				break;
			case VRTOKEN_YES:
				value = 1.0;
				break;
			default:
				vrErrPrintf("%s Parse Warning (line %d): "
					RED_TEXT "Non-numeric string '%s' given as number.\n" NORM_TEXT,
					parse->parse_name, value_token.linenum, value_token.string);
				value_token.token = VRTOKEN_NIL;	/* set to <nil> so no assignment will be done */
				break;
			}
			break;
		case VRTOKEN_NIL:
			break;
		}

		/***********************/
		/** do the assignment **/
		/* NOTE: assignments are not made when the value is the <nil> token, so old values remain */
		/* TODO: consider setting the array[count] value to 0.0 for VRTOKEN_NIL cases -- I think  */
		/*   that might make more sense here because they values will then be used to operate on  */
		/*   some other matrix or something.                                                      */
		switch(assignment_tok->token) {
		case VRTOKEN_SETEQUALS:
			if (value_token.token != VRTOKEN_NIL)
				array[count] = value;
			break;
		case VRTOKEN_PLUSEQUALS:
			if (value_token.token != VRTOKEN_NIL)
				array[count] += value;
			break;
		case VRTOKEN_TIMESEQUALS:
			if (value_token.token != VRTOKEN_NIL)
				array[count] *= value;
			break;
		default:
			vrErrPrintf("%s Parse Error (line %d): "
				RED_TEXT "Invalid assignment operator (\"%s\") for %s.\n" NORM_TEXT,
				parse->parse_name, assignment_tok->linenum, assignment_tok->string, setting);

			terminal_token = vrParseToEOS(parse);
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				terminal_token.linenum, terminal_token.string);

			vrTrace("vrParseDoubleListExprAssign()", RED_TEXT "exiting from assignment error" NORM_TEXT);
			return terminal_token;
		}

		/*********************************/	/* { -- to balance the one in the line below */
		/** ( "," | ";" | "}" | <eof> ) **/
		terminal_token = vrParseNextToken(parse);
		switch (terminal_token.token) {
		case VRTOKEN_COMMA:
			value_token = vrParseNextToken(parse);
			break;
		case VRTOKEN_SEMICOLON:
		case VRTOKEN_CLOSECURL:
		case VRTOKEN_EOF:
			value_token = terminal_token;
			continue;
		default:
			vrErrPrintf("%s Parse Error (line %d): "
				RED_TEXT "%s: Unexpected end of list.\n" NORM_TEXT,
				parse->parse_name, terminal_token.linenum, setting);
			value_token = terminal_token;
		}
	}

	if (count < maxlen)
		vrErrPrintf("%s Parse Warning (line %d): "
			RED_TEXT "%s: Less than the full list of %d numbers given.\n" NORM_TEXT,
			parse->parse_name, value_token.linenum, setting, maxlen);

	if (vrDbgDo(PARSE_DETAIL_DBGLVL)) {
		vrPrintf("vrParseDoubleListExprAssign(): " BOLD_TEXT "setting '%s' to list [", setting);
		for (count = 0; count < maxlen; count++) {
			vrPrintf( BOLD_TEXT "%.4lf ", array[count]);
		}
		vrPrintf(BOLD_TEXT "\b]" NORM_TEXT
			" assignment token = '%s' (line %d), last value token = '%s' (line %d)\n",
			assignment_tok->string, assignment_tok->linenum,
			value_token.string, value_token.linenum);
	}

	vrTrace("vrParseDoubleListExpr()", RED_TEXT "exiting" NORM_TEXT);

	return terminal_token;
}


/*********************************************************************/
vrTokenInfo vrParseMatrixFromCoords(vrMatrix **xform, double *coords_ll, double *coords_lr, double *coords_ul, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	term_token;		/* the terminal token returned by vrParseDoubleListExprAssign() */
	double		coords[9];		/* temporary space for the list of numbers to operate with */
	vrMatrix	tmp_matrix;		/* temporary matrix for doing the operation */
	vrTokenInfo	assignment_tok;		/* type of assignment used */

	/********************/
	/** get the values **/
	term_token = vrParseDoubleListExprAssign(coords, 9, &assignment_tok, setting, parse);
	vrMatrixSetFromCoords(&tmp_matrix, coords);
	coords_ll[VR_X] = coords[0];
	coords_ll[VR_Y] = coords[1];
	coords_ll[VR_Z] = coords[2];
	coords_lr[VR_X] = coords[3];
	coords_lr[VR_Y] = coords[4];
	coords_lr[VR_Z] = coords[5];
	coords_ul[VR_X] = coords[6];
	coords_ul[VR_Y] = coords[7];
	coords_ul[VR_Z] = coords[8];

	/***********************/
	/** do the assignment **/
	switch (assignment_tok.token) {
	case VRTOKEN_SETEQUALS:
		/* TODO: get rid of leak when vrObjectCopy fixed  [2/2/00 -- not so sure anymore] */
		*xform = vrMatrixCreate();
		vrMatrixCopy(*xform, &tmp_matrix);
		break;
	case VRTOKEN_TIMESEQUALS:
		/* TODO: get rid of leak when vrObjectCopy fixed  [2/2/00 -- not so sure anymore] */
		*xform = vrMatrixCopy(vrMatrixCreate(), *xform);
		vrMatrixPreMult(*xform, &tmp_matrix);
		break;
	case VRTOKEN_PLUSEQUALS:
	default:
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid assignment operator (\"%s\") for %s.\n" NORM_TEXT,
			parse->parse_name, assignment_tok.linenum, assignment_tok.string, setting);

		if (!vrTokenTerminal(term_token.token)) {
			term_token = vrParseToEOS(parse);
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				term_token.linenum, term_token.string);
		}
		vrTrace("vrParseMatrixFromCoords()", RED_TEXT "exiting from assignment error" NORM_TEXT);
		return term_token;
	}

	if (vrDbgDo(PARSE_DETAIL_DBGLVL)) {
		vrPrintf("vrParseMatrixFromCoords(): " BOLD_TEXT "setting '%s' with assignment '%s' (line %d):\n",
			setting, assignment_tok.string, assignment_tok.linenum);
		vrPrintMatrix("xform", *xform);
	}


	return term_token;
}


/*********************************************************************/
/* TODO: no need to use call by ref on xform once the vrObjectCopy() bug is fixed. */
/*  [2/2/00] Now I'm not so sure.  I tried it the other way, and it didn't work.   */
vrTokenInfo vrParseMatrixTransform(vrMatrix **xform, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	term_token;		/* the terminal token returned by vrParseDoubleListExprAssign() */
	vrMatrix	tmp_matrix;		/* temporary matrix for doing the operation */
	vrTokenInfo	assignment_tok;		/* type of assignment used */

	/********************/
	/** get the values **/

	/* tmp_matrix values must be set 1.0 for the possible use of the *= assignment */
	tmp_matrix.v[ 0] = tmp_matrix.v[ 1] = tmp_matrix.v[ 2] = tmp_matrix.v[ 3] = 1.0;
	tmp_matrix.v[ 4] = tmp_matrix.v[ 5] = tmp_matrix.v[ 6] = tmp_matrix.v[ 7] = 1.0;
	tmp_matrix.v[ 8] = tmp_matrix.v[ 9] = tmp_matrix.v[10] = tmp_matrix.v[11] = 1.0;
	tmp_matrix.v[12] = tmp_matrix.v[13] = tmp_matrix.v[14] = tmp_matrix.v[15] = 1.0;

	term_token = vrParseDoubleListExprAssign(tmp_matrix.v, 16, &assignment_tok, setting, parse);

	/***********************/
	/** do the assignment **/
	switch (assignment_tok.token) {
	case VRTOKEN_SETEQUALS:
		*xform = vrMatrixCreate();	/* TODO: get rid of leak when vrObjectCopy fixed  [2/2/00 -- not so sure anymore] */
		vrMatrixCopy(*xform, &tmp_matrix);
		break;
	case VRTOKEN_TIMESEQUALS:
		/* TODO: get rid of leak when vrObjectCopy fixed  [2/2/00 -- not so sure anymore] */
		*xform = vrMatrixCopy(vrMatrixCreate(), *xform);
		vrMatrixPreMult(*xform, &tmp_matrix);
		break;
	case VRTOKEN_PLUSEQUALS:
	default:
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid assignment operator (\"%s\") for %s.\n" NORM_TEXT,
			parse->parse_name, assignment_tok.linenum, assignment_tok.string, setting);

		if (!vrTokenTerminal(term_token.token)) {
			term_token = vrParseToEOS(parse);
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				term_token.linenum, term_token.string);
		}
		vrTrace("vrParseMatrixTransform()", RED_TEXT "exiting from assignment error" NORM_TEXT);
		return term_token;
	}

	if (vrDbgDo(PARSE_DETAIL_DBGLVL)) {
		vrPrintf("vrParseMatrixTransform(): " BOLD_TEXT "setting '%s' with assignment '%s' (line %d):\n",
			setting, assignment_tok.string, assignment_tok.linenum);
		vrPrintMatrix("xform", *xform);
	}

	return term_token;
}


/*********************************************************************/
/* TODO: no need to use call by ref on xform once the vrObjectCopy() bug is fixed. */
/*  [2/2/00] Now I'm not so sure.  I tried it the other way, and it didn't work.   */
vrTokenInfo vrParseMatrixTranslate(vrMatrix **xform, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	term_token;		/* the terminal token returned by vrParseDoubleListExprAssign() */
	vrMatrix	tmp_matrix;		/* temporary matrix for doing the operation */
	vrTokenInfo	assignment_tok;		/* type of assignment used */

	/********************/
	/** get the values **/

	/* tmp_matrix values must be set 1.0 for the possible use of the *= assignment */
	tmp_matrix.v[0] = tmp_matrix.v[1] = tmp_matrix.v[2] = 1.0;

	term_token = vrParseDoubleListExprAssign(tmp_matrix.v, 3, &assignment_tok, setting, parse);

	/***********************/
	/** do the assignment **/
	switch (assignment_tok.token) {
	case VRTOKEN_SETEQUALS:
		*xform = vrMatrixSetTranslationAd(vrMatrixCreate(), tmp_matrix.v);
		break;
	case VRTOKEN_TIMESEQUALS:
		/* TODO: get rid of leak when vrObjectCopy fixed  [2/2/00 -- not so sure anymore] */
		*xform = vrMatrixCopy(vrMatrixCreate(), *xform);
		vrMatrixPreMult(*xform, vrMatrixSetTranslationAd(vrMatrixCreate(), tmp_matrix.v));
		break;
	case VRTOKEN_PLUSEQUALS:
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid assignment operator (\"%s\") for %s.\n" NORM_TEXT,
			parse->parse_name, assignment_tok.linenum, assignment_tok.string, setting);

		if (!vrTokenTerminal(term_token.token)) {
			term_token = vrParseToEOS(parse);
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				term_token.linenum, term_token.string);
		}
		vrTrace("vrParseMatrixTranslate()", RED_TEXT "exiting from assignment error" NORM_TEXT);
		return term_token;

	default:
		/* TODO: report grievous error  -- this should never happen */
		break;
	}

	if (vrDbgDo(PARSE_DETAIL_DBGLVL)) {
		vrPrintf("vrParseMatrixTranslate(): " BOLD_TEXT "setting '%s' with assignment '%s' (line %d):\n",
			setting, assignment_tok.string, assignment_tok.linenum);
		vrPrintMatrix("xform", *xform);
	}

	return term_token;
}


/*********************************************************************/
/* TODO: no need to use call by ref on xform once the vrObjectCopy() bug is fixed. */
/*  [2/2/00] Now I'm not so sure.  I tried it the other way, and it didn't work.   */
vrTokenInfo vrParseMatrixRotate(vrMatrix **xform, char *setting, vrParseInfo *parse)
{
	vrTokenInfo	term_token;		/* the terminal token returned by vrParseDoubleListExprAssign() */
	double		rot_values[4];		/* temporary space for the list of numbers to operate with */
	vrTokenInfo	assignment_tok;		/* type of assignment used */

	/********************/
	/** get the values **/

	/* rot_values values must be set 1.0 for the possible use of the *= assignment */
	rot_values[0] = rot_values[1] = rot_values[2] = rot_values[3] = 1.0;

	term_token = vrParseDoubleListExprAssign(rot_values, 4, &assignment_tok, setting, parse);

	/***********************/
	/** do the assignment **/
	switch (assignment_tok.token) {
	case VRTOKEN_SETEQUALS:
		*xform = vrMatrixSetRotationAd(vrMatrixCreate(), rot_values);
		break;
	case VRTOKEN_TIMESEQUALS:
		/* TODO: get rid of leak when vrObjectCopy fixed  [2/2/00 -- not so sure anymore] */
		*xform = vrMatrixCopy(vrMatrixCreate(), *xform);
#if 0 /* 2/2/01 -- trying postmult */
		vrMatrixPreMult(*xform, vrMatrixSetRotationAd(vrMatrixCreate(), rot_values));
#else
		vrMatrixPostMult(*xform, vrMatrixSetRotationAd(vrMatrixCreate(), rot_values));
#endif
		break;
	case VRTOKEN_PLUSEQUALS:
		vrErrPrintf("%s Parse Error (line %d): "
			RED_TEXT "Invalid assignment operator (\"%s\") for %s.\n" NORM_TEXT,
			parse->parse_name, assignment_tok.linenum, assignment_tok.string, setting);

		if (!vrTokenTerminal(term_token.token)) {
			term_token = vrParseToEOS(parse);
			vrDbgPrintfN(ALWAYS_DBGLVL, "Config Parse (line %d): skipping '%s'\n",
				term_token.linenum, term_token.string);
		}
		vrTrace("vrParseMatrixRotate()", RED_TEXT "exiting from assignment error" NORM_TEXT);
		return term_token;

	default:
		/* TODO: report grievous error  -- this should never happen */
		break;
	}

	if (vrDbgDo(PARSE_DETAIL_DBGLVL)) {
		vrPrintf("vrParseMatrixRotate(): " BOLD_TEXT "setting '%s' with assignment '%s' (line %d):\n",
			setting, assignment_tok.string, assignment_tok.linenum);
		vrPrintMatrix("xform", *xform);
	}

	return term_token;
}


/*******************************************************************/
/* return all tokens through the next statement termination token  */
/*   the token type will be that of the termination token itself,  */
/*   but the string value will contain all the intervening tokens. */
/* NOTE: in the case of encountering an open or close curly-brace  */
/*   while skipping the rest of the statement, we will still need  */
/*   to properly parse brace token as it does more than merely     */
/*   terminate statements, it is also used to group objects, so it */
/*   itself can't just be skipped.                                 */
/*   This was added 04/04/12 -- so if other bugs appeared as of    */
/*   this date, this could be something to investigate.            */
vrTokenInfo vrParseToEOS(vrParseInfo *parse)
{
	char		*string = NULL;
	vrTokenInfo	token;
	int		paren_level = 0;
	char		*tofree;

	vrTrace("vrParseToEOS()", RED_TEXT "entering" NORM_TEXT);

	do {
		token = vrParseNextToken(parse);
		if (token.token == VRTOKEN_OPENCURL)
			paren_level++;
		if (token.token == VRTOKEN_CLOSECURL)
			paren_level--;

		if (string == NULL) {
			string = vrShmemStrDup(token.string);
		} else {
			tofree = string;
			string = vrShmemStrCat(string, " ");		/* add whitespace */
			string = vrShmemStrCat(string, token.string);
			vrShmemFree(tofree);
		}
	} while ((paren_level > 0) || (!vrTokenTerminal(token.token)));

	token.string = string;
	if (token.token == VRTOKEN_CLOSECURL || token.token == VRTOKEN_OPENCURL) {
		vrDbgPrintfN(PARSE_DBGLVL, "vrParseToEOS(): "
			BOLD_TEXT "returning token '%s' to the parsing stack -- it terminates the end-of-statement, but still must be properly parsed.\n" NORM_TEXT, string);
		vrParseReturnLastToken(token, parse);
	}

	vrTrace("vrParseToEOS()", RED_TEXT "exiting" NORM_TEXT);
	return token;
}


#if 0
/*********************************************************************/
/* TODO: I was thinking of having a parse-to-end-of-line function,   */
/*   but it would be dangerous, since the system is basically not    */
/*   line-based, and we could end up skipping a semicolon or curly   */
/*   brace.  So for now, I won't implement this.                     */
vrTokenInfo vrParseToEOL(vrParseInfo *parse)
{
}
#endif



/* =================================================================== */
/* This section has a different style of parsing than is used for      */
/*   handling arguments passed to input devices, processes, windows,   */
/*   etc. in the config file.  These are typically much simpler forms  */
/*   that just include an argument, some form of assignment, and a     */
/*   choice of assignees.                                              */

/* TODO: [6/3/03] specifically for vrArgParseString, trailing whitespace should be trimmed */
/* TODO: for all the vrArgParse...() functions, need to do the following: */
/*	- ignore case when searching for the arg option                   */
/*	- make sure the found arg immediately follows a separator (or     */
/*		the beginning of the line.)                               */
/*	- print a warning or use the last occurrence if more than one     */
/*		instance of an argument exists.                           */

/* NOTE: at one point both commas and semicolons were considered    */
/*   acceptable separators.  But commas were seldom used in the     */
/*   config file, yet were sometimes erroneously used as separating,*/
/*   within-argument tokens.  So, I decided to adopt the latter.    */
#undef COMMA_SEPARATOR	/* TODO: delete all the comma separator stuff */

/*********************************************************************/
int vrArgParseString(char *args, char *argname, char **arglvalue)
{
	char	*arg_str;	/* pointer to the argument string */
#ifdef UNPICKY_COMPILER
	char	*end_str;	/* pointer to the end of the args string */
#endif
	char	*begin_str;	/* pointer to the beginning of the argument value */
	char	*equals_str;	/* pointer to the "next" '=' after the argument */
	char	*sep_str;	/* pointer to the "next" separator after the argument */
	char	sep_char;	/* temp storage for the separator character */
	int	arg_set = 0;	/* boolean of whether the argument was set */

	/* Handle the case of a NULL argument string */
	if (args == NULL)
		return (0);

#ifdef UNPICKY_COMPILER
	/* search for the end of the argument string */
	end_str = strchr(args, '\0');
#endif

	/*******************************************************/
	/** Argument format: "<argname>" "=" <argvalue> [";"] **/
	/*******************************************************/
	if ((arg_str = strstr(args, argname)) != NULL) {	/* TODO: need to make this ignore case */
		/* find the next '=' character */
		equals_str = strchr(arg_str, '=');

		/* find the beginning and ending (separator) boundaries */
		begin_str = equals_str + strspn(equals_str, "= \t");
#ifdef COMMA_SEPARATOR /* also removed the space which didn't seem correct */
		sep_str = begin_str + strcspn(begin_str, " \t,;");
#else
		sep_str = begin_str + strcspn(begin_str, "\t;");
#endif

		/* Error if no '=', or no '=' prior to the next separator */
		if ((begin_str == NULL) || (begin_str > sep_str)) {
			vrErrPrintf("vrArgParseString(): "
				RED_TEXT "Missing assignment character after '%s' argument.\n" NORM_TEXT, argname);
		} else {
			/* temporarily terminate the string at the next separator */
			sep_char = sep_str[0];
			sep_str[0] = '\0';

			if (strlen(begin_str) == 0) {
				vrErrPrintf("vrArgParseString(): "
					RED_TEXT "No filename following '%s' argument.\n" NORM_TEXT, argname);
			} else {
				*arglvalue = vrShmemStrDup(begin_str);
				arg_set = 1;
				vrDbgPrintfN(PARSE_DBGLVL, "vrArgParseString(): parsed %s = \"%s\"\n", argname, *arglvalue);
			}

			/* restore the string's separator for other arguments */
			sep_str[0] = sep_char;
		}
	}

	return (arg_set);
}


/*********************************************************************/
int vrArgParseFloat(char *args, char *argname, float *arglvalue)
{
	char	*arg_str;	/* pointer to the argument string */
	char	*end_str;	/* pointer to the end of the args string */
	char	*equals_str;	/* pointer to the "next" '=' after the argument */
#ifdef COMMA_SEPARATOR
	char	*comma_str;	/* pointer to the "next" ',' after the argument */
	char	*semi_str;	/* pointer to the "next" ';' after the argument */
#endif
	char	*sep_str;	/* pointer to the "next" separator after the argument */
	int	arg_set = 0;	/* boolean of whether the argument was set */

	/* Handle the case of a NULL argument string */
	if (args == NULL)
		return (0);

	/* search for the end of the argument string */
	end_str = strchr(args, '\0');

	/***********************************************************/
	/** Argument format: "<argname>" "=" number [(";" | ",")] **/
	/***********************************************************/
	if ((arg_str = strstr(args, argname)) != NULL) {	/* TODO: need to make this ignore case */
		/* find the next '=' character */
		equals_str = strchr(arg_str, '=');

		/* find the next separator */
#ifdef COMMA_SEPARATOR
		comma_str = strchr(arg_str, ',');
		semi_str = strchr(arg_str, ';');
		if (semi_str == NULL) semi_str = end_str;
		if (comma_str == NULL) comma_str = end_str;
		sep_str = (comma_str < semi_str ? comma_str : semi_str);
#else
		sep_str = strchr(arg_str, ';');
		if (sep_str == NULL) sep_str = end_str;
#endif

		/* Error if no '=', or no '=' prior to the next separator */
		if ((equals_str == NULL) || (equals_str > sep_str)) {
			vrErrPrintf("vrArgParseFloat(): "
				RED_TEXT "Missing assignment character after '%s' argument.\n" NORM_TEXT, argname);
		} else {
			*arglvalue = atof(equals_str+1);
			arg_set = 1;
			vrDbgPrintfN(PARSE_DBGLVL, "vrArgParseFloat(): parsed %s = %f\n", argname, *arglvalue);
		}
	}

	return (arg_set);
}


/*********************************************************************/
int vrArgParseFloatList(char *args, char *argname, float *arglvalue, int maxlen)
{
static	char	*whitespace = " \t\r\b\n";
static	char	*whiteandsep = " \t\r\b\n,;";
#ifndef COMMA_SEPARATOR
static	char	*whiteandcomma = " \t\r\b\n,";
#endif
	char	*arg_str;	/* pointer to the argument string */
	char	*end_str;	/* pointer to the end of the args string */
	char	*equals_str;	/* pointer to the "next" '=' after the argument */
#ifdef COMMA_SEPARATOR
	char	*comma_str;	/* pointer to the "next" ',' after the argument */
	char	*semi_str;	/* pointer to the "next" ';' after the argument */
#endif
	char	*sep_str;	/* pointer to the "next" separator after the argument */
	char	*num_str;	/* pointer to the "next" number in the argument */
	int	arg_set = 0;	/* boolean of whether the argument was set */
	int	count;		/* for putting the next number in the proper location */

	/* Handle the case of a NULL argument string */
	if (args == NULL)
		return (0);

	/* search for the end of the argument string */
	end_str = strchr(args, '\0');

	/*******************************************************************/
	/** Argument format: "<argname>" "=" number [ "," number ]* [";"] **/
	/*******************************************************************/
	if ((arg_str = strstr(args, argname)) != NULL) {	/* TODO: need to make this ignore case */
		/* find the next '=' character */
		equals_str = strchr(arg_str, '=');

		/* find the next separator */
#ifdef COMMA_SEPARATOR
		comma_str = strchr(arg_str, ',');
		semi_str = strchr(arg_str, ';');
		if (semi_str == NULL) semi_str = end_str;
		if (comma_str == NULL) comma_str = end_str;
		sep_str = (comma_str < semi_str ? comma_str : semi_str);
#else
		sep_str = strchr(arg_str, ';');
		if (sep_str == NULL) sep_str = end_str;
#endif

		/* Error if no '=', or no '=' prior to the next separator */
		if ((equals_str == NULL) || (equals_str > sep_str)) {
			vrErrPrintf("vrArgParseFloat(): "
				RED_TEXT "Missing assignment character after '%s' argument.\n" NORM_TEXT, argname);
		} else {
			vrDbgPrintfN(PARSE_DBGLVL, "vrArgParseFloatList(): parsed %s = ", argname);
			num_str = equals_str+1;					/* skip the equals */
			num_str += strspn(num_str, whitespace);			/* skip whitespace */
			count = 0;
			do {
				arglvalue[count] = atof(num_str);

				/* these two lines basically skip the number and trailing whitespace */
				num_str += strcspn(num_str, whiteandsep);	/* skip non-whitespace (and separators) */
				num_str += strspn(num_str, whiteandcomma);	/* skip whitespace (and commas) */
				vrDbgPrintfN(PARSE_DBGLVL, "%f (next = '%s') ", arglvalue[count], num_str);

				count++;
			} while (count < maxlen && strcspn(num_str, ";") > 0);
			vrDbgPrintfN(PARSE_DBGLVL, "\n");
			arg_set = 1;
		}
	}

	return (arg_set);
}


/*********************************************************************/
int vrArgParseInteger(char *args, char *argname, int *arglvalue)
{
	char	*arg_str;	/* pointer to the argument string */
	char	*end_str;	/* pointer to the end of the args string */
	char	*equals_str;	/* pointer to the "next" '=' after the argument */
#ifdef COMMA_SEPARATOR
	char	*comma_str;	/* pointer to the "next" ',' after the argument */
	char	*semi_str;	/* pointer to the "next" ';' after the argument */
#endif
	char	*sep_str;	/* pointer to the "next" separator after the argument */
	int	arg_set = 0;	/* boolean of whether the argument was set (return value) */

	/* Handle the case of a NULL argument string */
	if (args == NULL)
		return (0);

	/* search for the end of the argument string */
	end_str = strchr(args, '\0');

	/***********************************************************/
	/** Argument format: "<argname>" "=" number [(";" | ",")] **/
	/***********************************************************/
	if ((arg_str = strstr(args, argname)) != NULL) {	/* TODO: need to make this ignore case */
		/* find the next '=' character */
		equals_str = strchr(arg_str, '=');

		/* find the next separator */
#ifdef COMMA_SEPARATOR
		comma_str = strchr(arg_str, ',');
		semi_str = strchr(arg_str, ';');
		if (semi_str == NULL) semi_str = end_str;
		if (comma_str == NULL) comma_str = end_str;
		sep_str = (comma_str < semi_str ? comma_str : semi_str);
#else
		sep_str = strchr(arg_str, ';');
		if (sep_str == NULL) sep_str = end_str;
#endif

		/* Error if no '=', or no '=' prior to the next separator */
		if ((equals_str == NULL) || (equals_str > sep_str)) {
			vrErrPrintf("vrArgParseInteger(): "
				RED_TEXT "Missing assignment character after '%s' argument.\n" NORM_TEXT, argname);
		} else {
			*arglvalue = vrAtoI(equals_str+1);
			arg_set = 1;
			vrDbgPrintfN(PARSE_DBGLVL, "vrArgParseInteger(): parsed %s = %d\n", argname, *arglvalue);
		}
	}

	return (arg_set);
}


/*********************************************************************/
int vrArgParseBool(char *args, char *argname, int *arglvalue)
{
	char	*arg_str;	/* pointer to the argument string */
	char	*begin_str;	/* pointer to the beginning of the argument value */
	char	*equals_str;	/* pointer to the "next" '=' after the argument */
	char	*sep_str;	/* pointer to the "next" separator after the argument */
	char	sep_char;	/* temp storage for the separator character */
	int	arg_set = 0;	/* boolean of whether the argument was set */

	/* Handle the case of a NULL argument string */
	if (args == NULL)
		return (0);

	/****************************************************************************************************/
	/** Argument format: "<argname>" "=" { "on"|"off"|"yes"|"no"|"true"|"false"|number } [(";" | ",")] **/
	/****************************************************************************************************/
	if ((arg_str = strstr(args, argname)) != NULL) {	/* TODO: need to make this ignore case */
		/* find the next '=' character */
		equals_str = strchr(arg_str, '=');

		/* find the beginning and ending (separator) boundaries */
		begin_str = equals_str + strspn(equals_str, "= \t");
#ifdef COMMA_SEPARATOR
		sep_str = begin_str + strcspn(begin_str, " \t,;");
#else
		sep_str = begin_str + strcspn(begin_str, " \t;");
#endif

		/* Error if no '=', or no '=' prior to the next separator */
		if ((begin_str == NULL) || (begin_str > sep_str)) {
			vrErrPrintf("vrArgParseBool(): "
				RED_TEXT "Missing assignment character after '%s' argument.\n" NORM_TEXT, argname);
		} else {
			/* temporarily terminate the string at the next separator */
			sep_char = sep_str[0];
			sep_str[0] = '\0';

			if (strlen(begin_str) == 0) {
				vrErrPrintf("vrArgParseBool(): "
					RED_TEXT "No value following '%s' argument.\n" NORM_TEXT, argname);
			} else {
				if (!strcasecmp(begin_str, "on")) {
					*arglvalue = 1;
				} else if (!strcasecmp(begin_str, "yes")) {
					*arglvalue = 1;
				} else if (!strcasecmp(begin_str, "true")) {
					*arglvalue = 1;
				} else if (!strcasecmp(begin_str, "off")) {
					*arglvalue= 0;
				} else if (!strcasecmp(begin_str, "no")) {
					*arglvalue= 0;
				} else if (!strcasecmp(begin_str, "false")) {
					*arglvalue= 0;
				} else {
					*arglvalue = !(!vrAtoI(begin_str));
				}
				arg_set = 1;
				vrDbgPrintfN(PARSE_DBGLVL, "vrArgParseBool(): parsed %s = %d\n", argname, *arglvalue);
			}

			/* restore the string's separator for other arguments */
			sep_str[0] = sep_char;
		}
	}

	return (arg_set);
}


/*********************************************************************/
int vrArgParseChoiceInteger(char *args, char *argname, int *arglvalue, char *choices[], int values[])
{
	char	*arg_str;	/* pointer to the argument string */
	char	*begin_str;	/* pointer to the beginning of the argument value */
	char	*equals_str;	/* pointer to the "next" '=' after the argument */
	char	*sep_str;	/* pointer to the "next" separator after the argument */
	char	sep_char;	/* temp storage for the separator character */
	int	arg_set = 0;	/* boolean of whether the argument was set */

	char	**choice;	/* current choice to test for */
	int	*value;		/* associated value of current choice */

	/* Handle the case of a NULL argument string */
	if (args == NULL)
		return (0);

	/****************************************************************************************/
	/** Argument format: "<argname>" "=" { "<choice1>" | "<choice2>" | ... } [(";" | ",")] **/
	/****************************************************************************************/
	if ((arg_str = strstr(args, argname)) != NULL) {	/* TODO: need to make this ignore case */
		/* find the next '=' character */
		equals_str = strchr(arg_str, '=');

		/* find the beginning and ending (separator) boundaries */
		begin_str = equals_str + strspn(equals_str, "= \t");
#ifdef COMMA_SEPARATOR
		sep_str = begin_str + strcspn(begin_str, " \t,;");
#else
		sep_str = begin_str + strcspn(begin_str, " \t;");
#endif

		/* Error if no '=', or no '=' prior to the next separator */
		if ((begin_str == NULL) || (begin_str > sep_str)) {
			vrErrPrintf("vrArgParseChoiceInteger(): "
				RED_TEXT "Missing assignment character after '%s' argument.\n" NORM_TEXT, argname);
		} else {
			/* temporarily terminate the string at the next separator */
			sep_char = sep_str[0];
			sep_str[0] = '\0';

			if (strlen(begin_str) == 0) {
				vrErrPrintf("vrArgParseChoiceInteger(): "
					RED_TEXT "No value following '%s' argument.\n" NORM_TEXT, argname);
			} else {
				for (choice = choices, value = values; *choice != NULL; choice++, value++) {
vrPrintf("checking if '%s' matches with '%s' giving a value of %d\n", begin_str, *choice, *value);
					if (!strcasecmp(begin_str, *choice)) {
						*arglvalue = *value;
						arg_set = 1;
						vrDbgPrintfN(PARSE_DBGLVL, "ArgParseChoiceInteger(): parsed %s = %d ('%s')\n", argname, *arglvalue, *choice);
						break;
					}
				}
			}

			/* restore the string's separator for other arguments */
			sep_str[0] = sep_char;
		}
	}

	return (arg_set);
}



/* ==================================================================== */
/* This section has parsing routines that are specialized to particular */
/*   tasks that may or may not be common to several places in the code. */


/************************************************************/
/*  For parsing things of the form:                           */
/*       2switch(0) -- example of a static 2switch            */
/*       valuator(0.5) -- example of a static valuator        */
/*       2switch(button[1]) -- for a Magellan or similar unit */
vrInputDesc *vrParseInputDescription(char *description)
{
	vrInputDesc	*indesc;
	char		*desc;
	char		*open_paren;
	char		*close_paren;

	indesc = (vrInputDesc *)vrShmemAlloc(sizeof(vrInputDesc));
	indesc->type = VRINPUT_UNKNOWN;
	indesc->args = vrShmemStrDup("");
	desc = vrShmemStrDup(description);

	/* find the first '(' character */
	open_paren = strchr(desc, '(');
	if (open_paren == NULL) {
		vrErrPrintf(RED_TEXT "vrParseInputDescription(): Invalid description format '%s'\n" NORM_TEXT, desc);
		return indesc;
	}

	*open_paren = '\0';
	if (!strcasecmp(desc, "2switch")) {
		indesc->type = VRINPUT_BINARY;
	} else if (!strcasecmp(desc, "switch2")) {
		indesc->type = VRINPUT_BINARY;
	} else if (!strcasecmp(desc, "Nswitch")) {
		indesc->type = VRINPUT_NWAY;
	} else if (!strcasecmp(desc, "switchN")) {
		indesc->type = VRINPUT_NWAY;
	} else if (!strcasecmp(desc, "valuator")) {
		indesc->type = VRINPUT_VALUATOR;
	} else if (!strcasecmp(desc, "6sensor")) {
		indesc->type = VRINPUT_6SENSOR;
	} else if (!strcasecmp(desc, "sensor6")) {
		indesc->type = VRINPUT_6SENSOR;
	} else if (!strcasecmp(desc, "Nsensor")) {
		indesc->type = VRINPUT_NSENSOR;
	} else if (!strcasecmp(desc, "sensorN")) {
		indesc->type = VRINPUT_NSENSOR;
	} else if (!strcasecmp(desc, "control")) {
		indesc->type = VRINPUT_CONTROL;
	} else {
		vrErrPrintf(RED_TEXT "vrParseInputDescription: unknown input type '%s'\n", desc);
		indesc->type = VRINPUT_UNKNOWN;
	}

	/* now find the characters up to the next ')' character */
	close_paren = strchr(open_paren+1, ')');
	*close_paren = '\0';
	indesc->args = vrShmemStrDup(open_paren+1);

	return indesc;
}


/***********************************************************/
/* For parsing things of the form: <device>:type[instance] */
/*   In particular this was designed for Xevents, but may  */
/*   work well in other circumstances.                     */
/*                                                         */
/* NOTE: [1/31/00] We have a choice of whether a string with */
/*   neither a ':' or '[' & ']' characters is interpreted as */
/*   either a type, with empty device & instance values, or  */
/*   as an instance with empty device & type values -- or    */
/*   even as device without the other two, but since many    */
/*   input declarations don't even use the device field,     */
/*   that doesn't make a lot of sense.  Previously, it had   */
/*   been the type that was the default, but today I'm going */
/*   to try using the instance field, allowing "" to be a    */
/*   possible type.                                          */
vrInputDTI *vrParseInputDTI(char *string)
{
	vrInputDTI	*dti;		/* pointer to a new DTI structure */
	char		*tmp_string;	/* duplicate copy of string used for manipulation */
	char		*colon;		/* locate of the first colon (':') in the string */
	char		*open_square;	/* locate of the first open-square ('[') in the string */
	char		*close_square;	/* locate of the first close-square (']') in the string */

	dti = (vrInputDTI *)vrShmemAlloc(sizeof(vrInputDTI));
	dti->device = vrShmemStrDup("");
	dti->type = vrShmemStrDup("");
	dti->instance = vrShmemStrDup("");
	tmp_string = vrShmemStrDup(string);

	/*******************/
	/*** dti->device ***/

	/* find the first ':' character */
	colon = strchr(tmp_string, ':');
	if (colon != NULL) {
		/* for some input parsing, the device field is not used */
		*colon = '\0';
		dti->device = vrShmemStrDup(tmp_string);
		*colon = ':';
	} else {
		colon = tmp_string-1;
	}

	/*****************/
	/*** dti->type ***/

	/* find the '[' character */
	open_square = strchr(tmp_string, '[');
	if (open_square == NULL ) {
#if 0 /* See 1/31/00 NOTE above */
		/* for some input parsing, the instance field is not used (when there's no '[' */
		dti->type = vrShmemStrDup(colon+1);
#else
		/* for some input parsing, the type field is not used (when there's no '[') */
		dti->instance = vrShmemStrDup(colon+1);
#endif
		return dti;
	}

	/* If the open-square happens before the colon, then the colon is really */
	/*   part of the instance name -- so make adjustments (added 08/19/13).  */
	if (open_square < colon) {
#if 0
		vrErrPrintf(RED_TEXT "vrParseInputDTI(): Invalid description format '%s'\n" NORM_TEXT, string);
		return dti;
#else
		/* the colon therefore is part of the instance name */
		dti->device = vrShmemStrDup("");	/* which means there is no sub-device */
#endif
		*open_square = '\0';
		dti->type = vrShmemStrDup(tmp_string);
		*open_square = '[';
	} else {
		*open_square = '\0';
		dti->type = vrShmemStrDup(colon+1);
		*open_square = '[';
	}

	/*********************/
	/*** dti->instance ***/

	/* find the ']' character */
	close_square = strchr(tmp_string, ']');
	if (close_square == NULL || close_square < open_square) {
		vrErrPrintf(RED_TEXT "vrParseInputDTI(): Invalid description format '%s'\n" NORM_TEXT, string);
		return dti;
	}
	*close_square = '\0';
	dti->instance = vrShmemStrDup(open_square+1);	/* the 'instance' is between the square-brackets */
	*close_square = '[';


	return dti;
}


/* =================================================================== */

#ifdef TEST_APP

/*********************************************************************/
static int vrParseFile(char *path, char *file)
{
	FILE		*fp_configfile;
	char		*fullfilename;
	vrTokenInfo	token;
	vrParseInfo	config_parse;

	fullfilename = (char *)vrShmemAlloc(strlen(path)+strlen(file)+3);
	sprintf(fullfilename, "%s/%s", path, file);

	/* TODO: at this point we really need to pipe the file      */
	/*   through the m4 macro processor, and then parse the     */
	/*   tokens.  If m4 can't be found then attempt to parse    */
	/*   the file anyway.  If there is a parsing error, then    */
	/*   print a warning message that indicates the problem     */
	/*   may stem from the fact that that m4 couldn't be found. */

	fp_configfile = fopen(fullfilename, "r");
	if (fp_configfile == NULL) {
		fprintf(stderr, "vrReadConfigFile(): " RED_TEXT "could not open file '%s'\n" NORM_TEXT, fullfilename);
		return 0;
	}
	printf("FreeVR: reading configuration file '%s'.\n", fullfilename);
	config_parse.linenum = 0;
	config_parse.parse_name = vrShmemStrDup("Config");
	config_parse.filename = vrShmemStrDup(fullfilename);
	config_parse.fp = fp_configfile;
	memset(config_parse.input, '\0', 256);
	config_parse.next_input = "";

/* NOTE: _ConfigTSMap is local to vr_config.c -- perhaps #include <vr_config.c> will work here. */
	config_parse.tsmap_list = _ConfigTSMap;
	config_parse.nummaps = (sizeof (_ConfigTSMap) / sizeof (vrTokenInfo));
	do {
		token = vrParseNextToken(config_parse);
		printf("%s Parse: (line %d) got token **** (%3d) '%s' ****\n", config_parse->parse_name, config_parse.linenum, token.token, token.string);
	} while (token.token != VRTOKEN_EOF);

	return 1;
}


/*********************************************************************/
main (int argc, char *argv[])
{
	vrParseFile(".", "freevr.rc");
}
#endif


