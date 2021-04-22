/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *    clock.h
 *    This file is part of "Sauvegarde" project.
 *
 *    (C) Copyright 2014 - 2017 Olivier Delhomme
 *     e-mail : olivier.delhomme@free.fr
 *
 *    "Sauvegarde" is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    "Sauvegarde" is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with "Sauvegarde".  If not, see <http://www.gnu.org/licenses/>
 */
/**
 * @file clock.h
 *
 * This file contains all the definitions needed to measure time.
 */
#ifndef _CLOCK_H_
#define _CLOCK_H_

/** Write clock output to file for tests */
#define CLOCK_T_OUTPUT_TO_FILE (1)

/** Filepath for clock output */
#define CLOCK_T_PATH ("/var/tmp/cdpfgl/clocks/clockfile.txt")

/**
 * @struct a_clock_t
 * @brief Structure to store clock information in order to measure
 *        elapsed time.
 */
typedef struct
{
    GDateTime *begin;  /** begin is filled when initializing the structure */
    GDateTime *end;    /** end is filled when freeing the structure */
} a_clock_t;


/**
 * Stores the file to write the output to
 */
static GFile *a_clock_out;

/**
 * Initializes a clock file to write output to
 */
void init_clock_t(char *filepath);


/**
 * Creates a new a_clock_t structure filled accordingly
 * @returns a a_clock_t * structure with begin field set.
 */
a_clock_t *new_clock_t(void);


/**
 * Ends the clock and prints the elapsed time and then frees everything
 * @param my_clock is a a_clock_t * structure with begin already filled
 * @param message is a message that we want to include into the displayed
 *        result in order to know what was measured.
 */
extern void end_clock(a_clock_t *my_clock, gchar *message);


#endif /* #ifndef _CLOCK_H_ */
