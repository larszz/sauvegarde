/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *    clock.c
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
 * @file clock.c
 * This file contains the functions to measure time.
 * The whole programs should not fail even if these functions
 * does and returns NULL pointers.
 */

#include "libcdpfgl.h"


/**
 * Initializes a clock file to write output to
 */
void init_clock_t(char *filepath)
{
    if (filepath != NULL)
    {
        a_clock_out = g_file_new_for_path(filepath);
        print_debug("Set clock output to %s\n", filepath);
    } else
    {
        a_clock_out = NULL;
    }
}


/**
 * Creates a new clock_t structure filled accordingly
 * @returns a clock_t * structure with begin field set.
 */
a_clock_t *new_clock_t(void)
{
    a_clock_t *my_clock = NULL;

    my_clock = (a_clock_t *) g_malloc0(sizeof(a_clock_t));

    if (my_clock != NULL)
    {
        my_clock->end = NULL;
        my_clock->begin = g_date_time_new_now_local();
    }

    return my_clock;
}


/**
 * Ends the clock and prints the elapsed time and then frees everything
 * @param my_clock is a clock_t * structure with begin already filled
 * @param message is a message that we want to include into the displayed
 *        result in order to know what was measured.
 */
void end_clock(a_clock_t *my_clock, gchar *message)
{
    GTimeSpan difference = 0;

    if (my_clock != NULL && my_clock->begin != NULL)
    {
        my_clock->end = g_date_time_new_now_local();
        difference = g_date_time_difference(my_clock->end, my_clock->begin);

        g_date_time_unref(my_clock->begin);
        g_date_time_unref(my_clock->end);
        free_variable(my_clock);
        print_debug("Elapsed time (%s): %ld µs\n", message, difference);

        // write to file if specified
        if (a_clock_out != NULL)
        {
            GFileOutputStream *stream = NULL;
            GError *error = NULL;
            gsize count = 0;
            gsize written = 0;
            gchar *buffer = NULL;

            stream = g_file_append_to(a_clock_out, G_FILE_CREATE_NONE, NULL, &error);
            if (stream != NULL)
            {
                buffer = g_strdup_printf("%s, %ld\n", message, difference);
                count = strlen(buffer);
                written = g_output_stream_write((GOutputStream *) stream, buffer, count, NULL, &error);

                if (error != NULL)
                {
                    print_error(__FILE__, __LINE__, "Error: could not append clock timestamp to file!\n");
                }

                g_output_stream_close((GOutputStream *) stream, NULL, &error);
                g_free(buffer);
            }
        }

    }
}



