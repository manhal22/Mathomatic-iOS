/*
 * Standard routines for Mathomatic.
 *
 * Copyright (C) 1987-2010 George Gesslein II.
 
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public 
    License as published by the Free Software Foundation; either 
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of 
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
 
    You should have received a copy of the GNU Lesser General Public 
    License along with this library; if not, write to the Free Software 
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 
 */

#include "includes.h"
#if	UNIX || CYGWIN
#include <sys/ioctl.h>
#include <termios.h>
#endif

/*
 * Standard function to report an error to the user.
 */
void
error(str)
const char	*str;
{
	error_str = str;	/* must be a constant string, temporary strings don't work */
#if	!SILENT
	set_color(2);		/* set color to red */
	printf("%s\n", str);
	default_color();	/* restore to default color */
#endif
}

/*
 * Standard function to report a warning to the user.
 */
void
warning(str)
const char	*str;
{
	warning_str = str;	/* must be a constant string, temporary strings don't work */
#if	!SILENT
	if (debug_level >= 0) {
		set_color(1);	/* set color to yellow */
		printf("Warning: %s\n", str);
		default_color();
	}
#endif
}

/*
 * This is called when the maximum expression size has been exceeded.
 *
 * There is no return.
 */
void
error_huge()
{
	longjmp(jmp_save, 14);
}

/*
 * This is called when a bug test result is positive.
 *
 * There is no return.
 */
void
error_bug(str)
const char	*str;	/* constant string to display */
{
	error(str);		/* Display the passed error message. */
#if	SILENT
	printf("%s\n", str);
#endif
	printf(_("Please report this bug to the maintainers,\n"));
	printf(_("along with the entry sequence that caused it.\n"));
	printf(_("Type \"help bugs\" for info on how to report bugs found in this program.\n"));
	longjmp(jmp_save, 13);	/* Abort the current operation with the critical error number 13. */
}

/*
 * Check if a floating point math function flagged an error.
 *
 * There is no return if an error message is displayed.
 */
void
check_err()
{
	switch (errno) {
	case EDOM:
		errno = 0;
		if (domain_check) {
			domain_check = false;
		} else {
			error(_("Domain error in constant."));
			longjmp(jmp_save, 2);
		}
		break;
	case ERANGE:
		errno = 0;
		error(_("Overflow error in constant."));
		longjmp(jmp_save, 2);
		break;
	}
}

/*
 * Get the screen (window) width and height from the operating system.
 *
 * Sets the global integers screen_columns and screen_rows.
 */
void
get_screen_size()
{
#if	UNIX || CYGWIN
	struct winsize	ws;

	ws.ws_col = 0;
	ws.ws_row = 0;
	if (ioctl(1, TIOCGWINSZ, &ws) >= 0) {
		if (ws.ws_col > 0) {
			screen_columns = ws.ws_col;
		}
		if (ws.ws_row > 0) {
			screen_rows = ws.ws_row;
		}
	}
#else
	screen_columns = STANDARD_SCREEN_COLUMNS;
	screen_rows = STANDARD_SCREEN_ROWS;
#endif
}

/*
 * Allocate the needed global expression storage arrays.
 * Each is static and can hold n_tokens elements.
 * n_tokens must not change until Mathomatic terminates.
 *
 * init_mem() is called only once upon Mathomatic startup
 * before using the symbolic math engine,
 * and can be undone by calling free_mem() below.
 *
 * Returns true if successful, otherwise Mathomatic cannot be used.
 */
int
init_mem()
{
	if ((scratch = (token_type *) malloc(((n_tokens * 3) / 2) * sizeof(token_type))) == NULL
	    || (tes = (token_type *) malloc(n_tokens * sizeof(token_type))) == NULL
	    || (tlhs = (token_type *) malloc(n_tokens * sizeof(token_type))) == NULL
	    || (trhs = (token_type *) malloc(n_tokens * sizeof(token_type))) == NULL) {
		return false;
	}
	if (alloc_next_espace() < 0) {	/* make sure there is at least 1 equation space */
		return false;
	}
	clear_all();
	return true;
}

#if	LIBRARY
/*
 * Free the global expression storage arrays.
 * After calling this, memory usage is reset and Mathomatic becomes unusable,
 * so do not call unless finished with using the Mathomatic code.
 *
 * This routine is usually not needed, because when a program exits,
 * all the memory it allocated is released by the operating system.
 * Inclusion of this routine was requested by Tam Hanna for Symbian OS.
 */
void
free_mem()
{
	int i;

	clear_all();

	free(scratch);
	free(tes);
	free(tlhs);
	free(trhs);

	for (i = 0; i < N_EQUATIONS; i++) {
		if (lhs[i]) {
			free(lhs[i]);
			lhs[i] = NULL;
		}
		if (rhs[i]) {
			free(rhs[i]);
			rhs[i] = NULL;
		}
	}
	n_equations = 0;
}
#endif

/*
 * Initialize some important global variables to their defaults.
 * This is called on startup and by process() to reset the global flags to the default state.
 */
void
init_gvars()
{
	domain_check = false;
	high_prec = false;
	partial_flag = true;
	symb_flag = false;
	sign_cmp_flag = false;
	approximate_roots = false;
	repeat_flag = false;

	/* initialize the universal and often used constant "0" expression */
	zero_token.level = 1;
	zero_token.kind = CONSTANT;
	zero_token.token.constant = 0.0;

	/* initialize the universal and often used constant "1" expression */
	one_token.level = 1;
	one_token.kind = CONSTANT;
	one_token.token.constant = 1.0;
}

/*
 * Clean up when processing is unexpectedly interrupted or terminated.
 */
void
clean_up()
{
	int	i;

	init_gvars();		/* reset the global variables to the default */
	if (gfp != default_out) {	/* reset the output file to default */
#if	!SECURE
		if (gfp != stdout && gfp != stderr)
			fclose(gfp);
#endif
		gfp = default_out;
	}
	for (i = 0; i < n_equations; i++) {
		if (n_lhs[i] <= 0) {
			n_lhs[i] = 0;
			n_rhs[i] = 0;
		}
	}
}

/*
 * Register all sign variables in all equation spaces
 * so that the next sign variables returned by next_sign() will be unique.
 */
void
set_sign_array()
{
	int	i, j;

	CLEAR_ARRAY(sign_array);
	for (i = 0; i < n_equations; i++) {
		if (n_lhs[i] > 0) {
			for (j = 0; j < n_lhs[i]; j += 2) {
				if (lhs[i][j].kind == VARIABLE && (lhs[i][j].token.variable & VAR_MASK) == SIGN) {
					sign_array[(lhs[i][j].token.variable >> VAR_SHIFT) & SUBSCRIPT_MASK] = true;
				}
			}
			for (j = 0; j < n_rhs[i]; j += 2) {
				if (rhs[i][j].kind == VARIABLE && (rhs[i][j].token.variable & VAR_MASK) == SIGN) {
					sign_array[(rhs[i][j].token.variable >> VAR_SHIFT) & SUBSCRIPT_MASK] = true;
				}
			}
		}
	}
}

/*
 * Return next unused sign variable in "*vp".
 * Mark it used.
 */
int
next_sign(vp)
long	*vp;
{
	int	i;

	for (i = 0;; i++) {
		if (i >= ARR_CNT(sign_array)) {
			/* out of unique sign variables */
			*vp = SIGN;
			return false;
		}
		if (!sign_array[i]) {
			*vp = SIGN + (((long) i) << VAR_SHIFT);
			sign_array[i] = true;
			break;
		}
	}
	return true;
}

/*
 * Erase all equation spaces and initialize the global variables.
 * Similar to a restart.
 */
void
clear_all()
{
	int	i;

/* select first equation space */
	cur_equation = 0;
/* erase all equation spaces by setting their length to zero */
	CLEAR_ARRAY(n_lhs);
	CLEAR_ARRAY(n_rhs);
/* forget all variables names */
	for (i = 0; var_names[i]; i++) {
		free(var_names[i]);
		var_names[i] = NULL;
	}
/* reset everything to a known state */
	CLEAR_ARRAY(sign_array);
	init_gvars();
}

/*
 * Return true if the specified equation space is available,
 * zeroing and allocating if necessary.
 */
int
alloc_espace(i)
int	i;	/* equation space number */
{
	if (i < 0 || i >= N_EQUATIONS)
		return false;
	n_lhs[i] = 0;
	n_rhs[i] = 0;
	if (lhs[i] && rhs[i])
		return true;	/* already allocated */
	if (lhs[i] || rhs[i])
		return false;	/* something is wrong */
	lhs[i] = (token_type *) malloc(n_tokens * sizeof(token_type));
	if (lhs[i] == NULL)
		return false;
	rhs[i] = (token_type *) malloc(n_tokens * sizeof(token_type));
	if (rhs[i] == NULL) {
		free(lhs[i]);
		lhs[i] = NULL;
		return false;
	}
	return true;
}

/*
 * Allocate all equation spaces up to and including an equation space number,
 * making sure the specified equation number is valid and usable.
 *
 * Returns true if successful.
 */
int
alloc_to_espace(en)
int	en;	/* equation space number */
{
	if (en < 0 || en >= N_EQUATIONS)
		return false;
	for (;;) {
		if (en < n_equations)
			return true;
		if (n_equations >= N_EQUATIONS)
			return false;
		if (!alloc_espace(n_equations)) {
			warning(_("Memory is exhausted."));
			return false;
		}
		n_equations++;
	}
}

/*
 * Allocate or reuse an empty equation space.
 *
 * Returns empty equation space number ready for use or -1 on error.
 */
int
alloc_next_espace()
{
	int	i, n;

	for (n = cur_equation, i = 0;; n = (n + 1) % N_EQUATIONS, i++) {
		if (i >= N_EQUATIONS)
			return -1;
		if (n >= n_equations) {
			n = n_equations;
			if (!alloc_espace(n)) {
				warning(_("Memory is exhausted."));
				for (n = 0; n < n_equations; n++) {
					if (n_lhs[n] == 0) {
						n_rhs[n] = 0;
						return n;
					}
				}
				return -1;
			}
			n_equations++;
			return n;
		}
		if (n_lhs[n] == 0)
			break;
	}
	n_rhs[n] = 0;
	return n;
}

/*
 * Return the number of the next empty equation space, otherwise don't return.
 */
int
next_espace()
{
	int	i;

	i = alloc_next_espace();
	if (i < 0) {
		error(_("Out of free equation spaces."));
#if	!SILENT
		printf(_("Use the clear command on unnecessary equations and try again.\n"));
#endif
		longjmp(jmp_save, 3);	/* do not return */
	}
	return i;
}

/*
 * Copy equation space "src" to equation space "dest".
 * "dest" is overwritten.
 */
void
copy_espace(src, dest)
int	src, dest;	/* equation space numbers */
{
	if (src == dest) {
/*		error("Internal error: copy_espace() source and destination the same."); */
		return;
	}
	blt(lhs[dest], lhs[src], n_lhs[src] * sizeof(token_type));
	n_lhs[dest] = n_lhs[src];
	blt(rhs[dest], rhs[src], n_rhs[src] * sizeof(token_type));
	n_rhs[dest] = n_rhs[src];
}

/*
 * Return true if equation space "i" is solved for a normal variable.
 */
int
solved_equation(i)
int	i;
{
	if (n_rhs[i] <= 0)
		return false;
	if (n_lhs[i] != 1 || lhs[i][0].kind != VARIABLE || (lhs[i][0].token.variable & VAR_MASK) <= SIGN)
		return false;
	if (found_var(rhs[i], n_rhs[i], lhs[i][0].token.variable))
		return false;
	return true;
}

/*
 * Return the number of times variable "v" is found in an expression.
 */
int
found_var(p1, n, v)
token_type	*p1;	/* expression pointer */
int		n;	/* expression length */
long		v;	/* standard Mathomatic variable */
{
	int	j;
	int	count = 0;

	if (v) {
		for (j = 0; j < n; j++) {
			if (p1[j].kind == VARIABLE && p1[j].token.variable == v) {
				count++;
			}
		}
	}
	return count;
}

/*
 * Return true if variable "v" exists in equation space "i".
 */
int
var_in_equation(i, v)
int	i;	/* equation space number */
long	v;	/* standard Mathomatic variable */
{
	if (n_lhs[i] <= 0)
		return false;
	if (found_var(lhs[i], n_lhs[i], v))
		return true;
	if (n_rhs[i] <= 0)
		return false;
	if (found_var(rhs[i], n_rhs[i], v))
		return true;
	return false;
}

/*
 * Substitute every instance of "v" in "equation" with "expression".
 */
void
subst_var_with_exp(equation, np, expression, len, v)
token_type	*equation;	/* equation side pointer */
int		*np;		/* pointer to equation side length */
token_type	*expression;	/* expression pointer */
int		len;		/* expression length */
long		v;		/* variable to substitute with expression */
{
	int	j, k;
	int	level;

	if (v == 0 || len <= 0)
		return;
	for (j = *np - 1; j >= 0; j--) {
		if (equation[j].kind == VARIABLE && equation[j].token.variable == v) {
			level = equation[j].level;
			if (*np + len - 1 > n_tokens) {
				error_huge();
			}
			blt(&equation[j+len], &equation[j+1], (*np - (j + 1)) * sizeof(token_type));
			*np += len - 1;
			blt(&equation[j], expression, len * sizeof(token_type));
			for (k = j; k < j + len; k++)
				equation[k].level += level;
		}
	}
}

/*
 * Return the minimum level encountered in a Mathomatic "expression".
 */
int
min_level(expression, n)
token_type	*expression;	/* expression pointer */
int		n;		/* expression length */
{
	int		min1;
	token_type	*p1, *ep;

	if (n <= 1) {
		if (n <= 0) {
			return 1;
		} else {
			return expression[0].level;
		}
	}
	min1 = expression[1].level;
	ep = &expression[n];
	for (p1 = &expression[3]; p1 < ep; p1 += 2) {
		if (p1->level < min1)
			min1 = p1->level;
	}
	return min1;
}

/*
 * Get default equation number from a command parameter string.
 * The equation number must be the only parameter.
 * If no equation number is specified, default to the current equation.
 *
 * Return -1 on error.
 */
int
get_default_en(cp)
char	*cp;
{
	int	i;

	if (*cp == '\0') {
		i = cur_equation;
	} else {
		i = decstrtol(cp, &cp) - 1;
		if (extra_characters(cp))
			return -1;
	}
	if (not_defined(i)) {
		return -1;
	}
	return i;
}

/*
 * Get an expression from the user.
 * The prompt must be previously copied into the global prompt_str[].
 *
 * Return true if successful.
 */
int
get_expr(equation, np)
token_type	*equation;	/* where the parsed expression is stored (equation side) */
int		*np;		/* pointer to the returned parsed expression length */
{
	char	buf[DEFAULT_N_TOKENS];
	char	*cp;

#if	LIBRARY
	snprintf(buf, sizeof(buf), "#%+d", pull_number);
	pull_number++;
	cp = parse_expr(equation, np, buf);
	return(cp && *np > 0);
#else
	for (;;) {
		if ((cp = get_string(buf, sizeof(buf))) == NULL) {
			return false;
		}
		cp = parse_expr(equation, np, cp);
		if (cp) {
			break;
		}
	}
	return(*np > 0);
#endif
}

/*
 * Prompt for a variable name from the user.
 *
 * Return true if successful.
 */
int
prompt_var(vp)
long	*vp;	/* pointer to the returned variable */
{
	char	buf[MAX_CMD_LEN];
	char	*cp;

	for (;;) {
		my_strlcpy(prompt_str, _("Enter variable: "), sizeof(prompt_str));
		if ((cp = get_string(buf, sizeof(buf))) == NULL) {
			return false;
		}
		if (*cp == '\0') {
			return false;
		}
		cp = parse_var2(vp, cp);
		if (cp == NULL || extra_characters(cp)) {
			continue;
		}
		return true;
	}
}

/*
 * Return true and display a message if equation "i" is undefined.
 */
int
not_defined(i)
int	i;	/* equation space number */
{
	if (i < 0 || i >= n_equations) {
		error(_("Invalid equation number."));
		return true;
	}
	if (n_lhs[i] <= 0) {
		error(_("Equation space is empty."));
		return true;
	}
	return false;
}

/*
 * Verify that a current equation or expression is loaded.
 *
 * Return true and display a message if current equation space is empty.
 */
int
current_not_defined()
{
	int	i;

	i = cur_equation;
	if (i < 0 || i >= n_equations || n_lhs[i] <= 0) {
		error(_("No current equation or expression."));
		return true;
	}
	return false;
}

/*
 * Common routine to output the prompt in prompt_str[] and get a line of input from stdin.
 * All Mathomatic input comes from this routine, except for file reading.
 * If there is no more input (EOF), exit this program with no error.
 *
 * Returns "string" if successful or NULL on error.
 */
char *
get_string(string, n)
char	*string;	/* storage for input string */
int	n;		/* maximum size of "string" in bytes */
{
#if	LIBRARY
	error(_("Missing command line argument."));
	return NULL;
#else
	int	i;
#if	READLINE
	char	*cp;
#endif

	if (quiet_mode) {
		prompt_str[0] = '\0';	/* don't display a prompt */
	}
	input_column = strlen(prompt_str);
#if	READLINE
	if (readline_enabled) {
		cp = readline(prompt_str);
		if (cp == NULL) {
			if (!quiet_mode)
				printf(_("\nEnd of input.\n"));
			exit_program(0);
		}
		my_strlcpy(string, cp, n);
		if (skip_space(cp)[0] && (last_history_string == NULL || strcmp(last_history_string, cp))) {
			add_history(cp);
			last_history_string = cp;
		} else {
			free(cp);
		}
	} else {
		printf("%s", prompt_str);
		if (fgets(string, n, stdin) == NULL) {
			if (!quiet_mode)
				printf(_("\nEnd of input.\n"));
			exit_program(0);
		}
	}
#else
	printf("%s", prompt_str);
	if (fgets(string, n, stdin) == NULL) {
		if (!quiet_mode)
			printf(_("\nEnd of input.\n"));
		exit_program(0);
	}
#endif
	/* Fix an fgets() peculiarity: */
	i = strlen(string) - 1;
	if (i >= 0 && string[i] == '\n') {
		string[i] = '\0';
	}
	if ((gfp != stdout && gfp != stderr) || (echo_input && !quiet_mode)) {
		/* Input that is prompted for is now included in the redirected output
		   of a command to a file, making redirection like logging. */
		fprintf(gfp, "%s%s\n", prompt_str, string);
	}
	set_error_level(string);
	abort_flag = false;
	return string;
#endif
}

/*
 * Display the prompt in prompt_str[] and wait for "y" or "n" followed by Enter.
 *
 * Return true if "y".
 */
int
get_yes_no()
{
	char	*cp;
	char	buf[MAX_CMD_LEN];

	for (;;) {
		if ((cp = get_string(buf, sizeof(buf))) == NULL) {
			return false;
		}
		str_tolower(cp);
		switch (*cp) {
		case 'n':
			return false;
		case 'y':
			return true;
		}
	}
}

/*
 * Display the result of a command,
 * or store the pointer to the text of the listed expression
 * into result_str if compiled for the library.
 *
 * Return true if successful.
 */
int
return_result(en)
int	en;	/* equation space number the result is in */
{
	if (n_lhs[en] <= 0) {
		return false;
	}
#if	LIBRARY
	make_fractions_and_group(en);
	if (factor_int_flag) {
		factor_int_sub(en);
	}
	free_result_str();
	result_str = list_equation(en, false);
	result_en = en;
	if (gfp == stdout) {
		return(result_str != NULL);
	}
#endif
	return(list_sub(en) != 0);
}

/*
 * Free any malloc()ed result_str, so there won't be a memory leak
 * in the symbolic math library.
 */
void
free_result_str()
{
	if (result_str) {
		free(result_str);
		result_str = NULL;
	}
	result_en = -1;
}

/*
 * Return true if the first word in the passed string is "all".
 */
int
is_all(cp)
char	*cp;
{
	return(strcmp_tospace(cp, "all") == 0);
}

/*
 * Process an equation number range given in text string "*cpp".
 * Skip past all spaces and update "*cpp" to point to the next argument if successful.
 * If no equation number or range is given, assume the current equation is wanted.
 *
 * Return true if successful,
 * with the starting equation number in "*ip"
 * and ending equation number in "*jp".
 */
int
get_range(cpp, ip, jp)
char	**cpp;
int	*ip, *jp;
{
	int	i;
	char	*cp;

	cp = skip_space(*cpp);
	if (is_all(cp)) {
		cp = skip_param(cp);
		*ip = 0;
		*jp = n_equations - 1;
		while (*jp > 0 && n_lhs[*jp] == 0)
			(*jp)--;
	} else {
		if (isdigit(*cp)) {
			*ip = strtol(cp, &cp, 10) - 1;
		} else {
			*ip = cur_equation;
		}
		if (*ip < 0 || *ip >= n_equations) {
			error(_("Invalid equation number."));
			return false;
		}
		if (*cp != '-') {
			if (not_defined(*ip)) {
				return false;
			}
			*jp = *ip;
			*cpp = skip_space(cp);
			return true;
		}
		(cp)++;
		if (isdigit(*cp)) {
			*jp = strtol(cp, &cp, 10) - 1;
		} else {
			*jp = cur_equation;
		}
		if (*jp < 0 || *jp >= n_equations) {
			error(_("Invalid equation number."));
			return false;
		}
		if (*jp < *ip) {
			i = *ip;
			*ip = *jp;
			*jp = i;
		}
	}
	cp = skip_space(cp);
	for (i = *ip; i <= *jp; i++) {
		if (n_lhs[i] > 0) {
			*cpp = cp;
			return true;
		}
	}
	error(_("No equations defined in specified range."));
	return false;
}

/*
 * This function is provided to make sure there is nothing else on a command line.
 *
 * Returns true if any non-space characters are encountered before the end of the string
 * and an error message is printed.
 * Otherwise just returns false indicating everything is OK.
 */
int
extra_characters(cp)
char	*cp;	/* command line string */
{
	if (cp) {
		cp = skip_space(cp);
		if (*cp) {
			printf(_("\nError: \"%s\" not required on command line.\n"), cp);
			error(_("Extra characters or unrecognized argument."));
			return true;
		}
	}
	return false;
}

/*
 * get_range() if it is the last possible option on the command line,
 * otherwise display an error message and return false.
 */
int
get_range_eol(cpp, ip, jp)
char	**cpp;
int	*ip, *jp;
{
	if (!get_range(cpp, ip, jp)) {
		return false;
	}
	if (extra_characters(*cpp)) {
		return false;
	}
	return true;
}

/*
 * Skip over space characters.
 */
char *
skip_space(cp)
char	*cp;	/* character pointer */
{
	if (cp) {
		while (*cp && isspace(*cp))
			cp++;
	}
	return cp;
}

/*
 * Enhanced decimal strtol().
 * Skips trailing spaces.
 */
long
decstrtol(cp, cpp)
char	*cp, **cpp;
{
	long	l;

	l = strtol(cp, cpp, 10);
	if (cpp && cp != *cpp) {
		*cpp = skip_space(*cpp);
	}
	return l;
}

/*
 * Return true if passed character is a Mathomatic command parameter delimiter.
 */
int
isdelimiter(ch)
int	ch;
{
	return(isspace(ch) || ch == ',' || ch == '=');
}

/*
 * Skip over the current parameter in a Mathomatic command line string.
 * Parameters are usually separated with spaces.
 *
 * Returns a string (character pointer) to the next parameter.
 */
char *
skip_param(cp)
char	*cp;
{
	if (cp) {
		while (*cp && (!isascii(*cp) || !isdelimiter(*cp))) {
			cp++;
		}
		cp = skip_space(cp);
		if (isdelimiter(*cp)) {
			cp = skip_space(cp + 1);
		}
	}
	return(cp);
}

/*
 * Compare strings up to the end or the first space or parameter delimiter.
 * Must be an exact match.
 * Compare is alphabetic case insensitive.
 *
 * Returns zero on exact match, otherwise non-zero if strings are different.
 */
int
strcmp_tospace(cp1, cp2)
char	*cp1, *cp2;
{
	char	*cp1a, *cp2a;

	for (cp1a = cp1; *cp1a && !isdelimiter(*cp1a); cp1a++)
		;
	for (cp2a = cp2; *cp2a && !isdelimiter(*cp2a); cp2a++)
		;
	return strncasecmp(cp1, cp2, max(cp1a - cp1, cp2a - cp2));
}

/*
 * Return the number of "level" additive type operators.
 */
int
level_plus_count(p1, n1, level)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
int		level;	/* parentheses level number to check */
{
	int	i;
	int	count = 0;

	for (i = 1; i < n1; i += 2) {
		if (p1[i].level == level) {
			switch (p1[i].token.operatr) {
			case PLUS:
			case MINUS:
				count++;
			}
		}
	}
	return count;
}

/*
 * Return the number of level 1 additive type operators.
 */
int
level1_plus_count(p1, n1)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
{
	return level_plus_count(p1, n1, min_level(p1, n1));
}

/*
 * Return the count of variables in an expression.
 */
int
var_count(p1, n1)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
{
	int	i;
	int	count = 0;

	for (i = 0; i < n1; i += 2) {
		if (p1[i].kind == VARIABLE) {
			count++;
		}
	}
	return count;
}

/*
 * Set "*vp" if single variable expression.
 *
 * Return true if expression contains no variables.
 */
int
no_vars(source, n, vp)
token_type	*source;	/* expression pointer */
int		n;		/* expression length */
long		*vp;		/* variable pointer */
{
	int	j;
	int	found = false;

	if (*vp) {
		return(var_count(source, n) == 0);
	}
	for (j = 0; j < n; j += 2) {
		if (source[j].kind == VARIABLE) {
			if ((source[j].token.variable & VAR_MASK) <= SIGN)
				continue;
			if (*vp) {
				if (*vp != source[j].token.variable) {
					*vp = 0;
					break;
				}
			} else {
				found = true;
				*vp = source[j].token.variable;
			}
		}
	}
	return(!found);
}

/*
 * Return true if expression contains infinity or NaN (Not a Number).
 */
int
exp_contains_infinity(p1, n1)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
{
	int	i;

	for (i = 0; i < n1; i++) {
		if (p1[i].kind == CONSTANT && !isfinite(p1[i].token.constant)) {
			return true;
		}
	}
	return false;
}

/*
 * Return true if expression contains NaN (Not a Number).
 */
int
exp_contains_nan(p1, n1)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
{
	int	i;

	for (i = 0; i < n1; i++) {
		if (p1[i].kind == CONSTANT && isnan(p1[i].token.constant)) {
			return true;
		}
	}
	return false;
}

/*
 * Return true if expression is numeric (not symbolic).
 * Pseudo-variables e, pi, i, and sign are considered numeric.
 */
int
exp_is_numeric(p1, n1)
token_type	*p1;	/* expression pointer */
int		n1;	/* expression length */
{
	int	i;

	for (i = 0; i < n1; i++) {
		if (p1[i].kind == VARIABLE && (p1[i].token.variable & VAR_MASK) > SIGN) {
			return false;		/* not numerical (contains a variable) */
		}
	}
	return true;
}

/*
 * Check for division by zero.
 *
 * Display a warning and return true if passed double is 0.
 */
int
check_divide_by_zero(denominator)
double	denominator;
{
	if (denominator == 0) {
		warning(_("Division by zero."));
		return true;
	}
	return false;
}
