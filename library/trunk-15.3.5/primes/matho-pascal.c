/*
 * Calculate and display Pascal's triangle.
 *
 * Copyright (C) 2005 George Gesslein II.
 
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <termios.h>

#define	true	1
#define	false	0

#define	MAX_LINES	1000	/* max number of lines of Pascal's triangle allowed */

void	allocate_triangle(void);
void	calculate_triangle(void);
void	display_triangle(void);
int	center_buf(int line_number, int cell_size);
void	usage(void);

int		lines = 26;
int		gcell_size = 6;
long double	*array[MAX_LINES];
int		screen_columns = 80;
int		centered = true;
char		line_buf[1000];

char		*prog_name = "matho-pascal";

int
main(int argc, char *argv[])
{
	struct winsize	ws;

	ws.ws_col = 0;
	ws.ws_row = 0;
	ioctl(1, TIOCGWINSZ, &ws);
	if (ws.ws_col > 0) {
		screen_columns = ws.ws_col;
	}
	if (screen_columns >= sizeof(line_buf)) {
		screen_columns = sizeof(line_buf) - 1;
	}

	switch (argc) {
	case 0:
	case 1:
		break;
	case 2:
		centered = false;
		if (isdigit(argv[1][0])) {
			lines = atoi(argv[1]);
			break;
		}
	default:
		usage();
	}
	if (lines <= 0 || lines > MAX_LINES) {
		fprintf(stderr, "%s: Number of lines out of range (1..%d).\n", prog_name, MAX_LINES);
		exit(2);
	}
	allocate_triangle();
	calculate_triangle();
	display_triangle();
	exit(0);
}

void
allocate_triangle(void)
{
	int	i;

	for (i = 0; i < lines; i++) {
		array[i] = (long double *) malloc(sizeof(long double) * (i + 1));
		if (array[i] == NULL) {
			fprintf(stderr, "%s: Not enough memory.\n", prog_name);
			exit(2);
		}
	}
}

void
calculate_triangle(void)
{
	int	i, j;

	for (i = 0; i < lines; i++) {
		for (j = 0; j <= i; j++) {
			if (j == 0 || j == i) {
				array[i][j] = 1.0;	/* the line begins and ends with 1 */
			} else {
				array[i][j] = array[i-1][j-1] + array[i-1][j];
			}
		}
	}
}

void
display_triangle(void)
{
	int		i, j;
	int		len;

	if (centered && lines > 20) {
		len = center_buf(19, 8);
		if (len > 0 && len < screen_columns) {
			gcell_size = 8;	/* for very wide screens */
		}
	}
	for (i = 0; i < lines; i++) {
		if (centered) {
			len = center_buf(i, gcell_size);
			if (len <= 0 || len >= screen_columns) {
				return;	/* stop here because of wrap-around */
			}
			/* center on screen */
			for (j = (screen_columns - len) / 2; j > 0; j--) {
				printf(" ");
			}
			printf("%s", line_buf);
		} else {
			for (j = 0; j <= i; j++) {
				printf("%.19Lg ", array[i][j]);
			}
		}
		printf("\n");
	}
}

/*
 * Create a line of output in line_buf[] for centering mode.

 * Return length if successful, otherwise return 0.
 */
int
center_buf(int line_number, int cell_size)
{
	int	j, k;
	int	i1;
	int	len;
	char	buf2[100];

	assert(cell_size < sizeof(buf2));
	line_buf[0] = '\0';
	for (j = 0; j <= line_number; j++) {
		assert(strlen(line_buf) + cell_size < sizeof(line_buf));
		len = snprintf(buf2, sizeof(buf2), "%.19Lg", array[line_number][j]);
		if (len >= cell_size) {
			return(0);	/* cell_size too small */
		}
		/* center in the cell */
		for (k = i1 = (cell_size - len) / 2; k > 0; k--) {
			strcat(line_buf, " ");
		}
		strcat(line_buf, buf2);
		for (k = len + i1; k < cell_size; k++) {
			strcat(line_buf, " ");
		}
	}
	return(strlen(line_buf));
}

void
usage(void)
{
	printf("Usage: %s [number-of-lines]\n\n", prog_name);
	printf("Display up to %d lines of Pascal's triangle.\n", MAX_LINES);
	exit(2);
}
