/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *    options.c
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
 * @file server/options.c
 *
 *  This file contains all the functions to manage command line options for
 *  'cdpfglserver' program.
 */

#include "server.h"

static void print_selected_options(options_t *opt);
static void read_from_configuration_file(options_t *opt, gchar *filename);

/**
 * Frees the options structure if necessary.
 * @param opt : the malloc'ed options_t structure
 */
void free_options_t(options_t *opt)
{

    if (opt != NULL)
        {
            free_variable(opt);
        }

}


/**
 * Prints options as selected when invoking the program with -v option
 * @param opt the options_t * structure that contains all selected options
 *        from the command line and that will be used by the program.
 */
static void print_selected_options(options_t *opt)
{
    if (opt != NULL)
        {
            fprintf(stdout, _("\n%s options are:\n"), PROGRAM_NAME);

            print_string_option(_("Configuration file: %s\n"), opt->configfile);

            if (opt->port != 0)
                {
                    fprintf(stdout, _("Port number: %d\n"), opt->port);
                }
        }
}

/**
 * creates a buffer containing every selected options ...
 * @param opt the options_t * structure that contains all selected options
 *        from the command line and that will be used by the program.
 * @returns options as selected when invoking the program with -v option
 * into a newly allocated buffer that may be freed when no longer needed
 */
gchar *buffer_selected_option(options_t *opt)
{
    gchar *buf1 = NULL;
    gchar *buffer = NULL;

    if (opt != NULL)
        {
            if (opt->configfile != NULL)
                {
                    buffer = g_strdup_printf(_("\n%s options are:\nConfiguration file: %s\n"), PROGRAM_NAME, opt->configfile);
                }
            else
                {
                    buffer = g_strdup_printf(_("\n%s options are:\n"), PROGRAM_NAME);
                }

            if (opt->port != 0)
                {
                    buf1 = g_strdup_printf(_("%sPort number: %d\n"), buffer, opt->port);
                    free_variable(buffer);
                    buffer = buf1;
                }
        }

    return buffer;
}


/**
 * Reads from the configuration file "filename"
 * @param[in,out] opt : options_t * structure to store options read from the
 *                configuration file "filename"
 * @param filename : the filename of the configuration file to read from
 */
static void read_from_configuration_file(options_t *opt, gchar *filename)
{
    GKeyFile *keyfile = NULL;      /** Configuration file parser */
    GError *error = NULL;          /** Glib error handling       */
    srv_conf_t *srv_conf = NULL;

    if (filename != NULL)
        {
            if (opt->configfile != NULL)
                {
                    free_variable(opt->configfile);
                }

            opt->configfile = g_strdup(filename);

            print_debug(_("Reading configuration from file %s\n"), filename);

            keyfile = g_key_file_new();

            if (g_key_file_load_from_file(keyfile, filename, G_KEY_FILE_KEEP_COMMENTS, &error))
                {
                    srv_conf = read_from_group_server(keyfile, filename);
                    opt->port = srv_conf ->port;
                    opt->backend_meta = get_backend_number_from_label(srv_conf->backend_meta_label);
                    opt->backend_data = get_backend_number_from_label(srv_conf->backend_data_label);
                    read_debug_mode_from_file(keyfile, filename);
                }
            else if (error != NULL)
                {
                    print_error(__FILE__, __LINE__,  _("Failed to open %s configuration file: %s\n"), filename, error->message);
                    free_error(error);
                }

            g_key_file_free(keyfile);
        }
}


/**
 * This function parses command line options. It sets the options in this
 * order. It means that the value used for an option is the one set in the
 * lastest step.
 * 0) default values are set into the options_t * structure
 * 1) reads the default configuration file if any.
 * 2) reads the configuration file mentionned on the command line.
 * 3) sets the command line options (except for the list of directories,
 *    all other values are replaced by thoses in the command line)
 * @param argc : number of arguments given on the command line.
 * @param argv : an array of strings that contains command line arguments.
 * @returns options_t structure malloc'ed and filled upon choosen command
 *          line's option
 */
options_t *manage_command_line_options(int argc, char **argv)
{
    gboolean version = FALSE;       /** True if -v was selected on the command line                                        */
    gint cmdl_debug = -4;           /** debug mode as specified on the command line                                        */
    gchar *configfile = NULL;       /** Filename for the configuration file if any                                         */
    gint port = 0;                  /** Port number on which to listen                                                     */

    GOptionEntry entries[] =
    {
        { "version", 'v', 0, G_OPTION_ARG_NONE, &version, N_("Prints program version."), NULL},
        { "debug", 'd', 0,  G_OPTION_ARG_INT, &cmdl_debug, N_("Activates (1) or deactivates (0) debug mode."), N_("BOOLEAN")},
        { "configuration", 'c', 0, G_OPTION_ARG_STRING, &configfile, N_("Specify an alternative configuration file."), N_("FILENAME")},
        { "port", 'p', 0, G_OPTION_ARG_INT, &port, N_("Port NUMBER on which to listen."), N_("NUMBER")},
        { NULL }
    };

    GError *error = NULL;
    GOptionContext *context;
    options_t *opt = NULL;    /** Structure to manage program's options            */
    gchar *bugreport = NULL;  /** Bug Report message                               */
    gchar *summary = NULL;    /** Abstract for the program                         */
    gchar *defaultconfigfilename = NULL;

    bugreport = g_strconcat(_("Please report bugs to: "), PACKAGE_BUGREPORT, NULL);
    summary = g_strdup(_("This program is monitoring file changes in the filesystem and is hashing\nfiles with SHA256 algorithms from Glib."));
    context = g_option_context_new("");

    set_debug_mode(ENABLE_DEBUG);

    set_option_context_options(context, entries, TRUE, bugreport, summary);

    if (!g_option_context_parse(context, &argc, &argv, &error))
        {
            g_print(_("[%s, %d] Option parsing failed: %s\n"),  __FILE__, __LINE__, error->message);
            exit(EXIT_FAILURE);
        }

    /* 0) Setting default values */

    opt = (options_t *) g_malloc0(sizeof(options_t));

    opt->configfile = NULL;
    opt->port = SERVER_PORT;


    /* 1) Reading options from default configuration file */
    defaultconfigfilename = get_probable_etc_path(PROGRAM_NAME, "server.conf");
    read_from_configuration_file(opt,  defaultconfigfilename);
    free_variable(defaultconfigfilename);

    opt->version = version; /* only TRUE if -v or --version was invoked */


    /* 2) Reading the configuration from the configuration file specified
     *    on the command line
     */
    if (configfile != NULL)
        {
            read_from_configuration_file(opt, configfile);
        }

    /* 3) retrieving other options from the command line.
     */

    set_debug_mode_upon_cmdl(cmdl_debug);

    if (port > 1024 && port < 65535)
        {
            opt->port = port;
        }

    g_option_context_free(context);
    free_variable(bugreport);
    free_variable(summary);

    return opt;
}


/**
 * Decides what to do upon command lines options passed to the program
 * @param argc : number of arguments given on the command line.
 * @param argv : an array of strings that contains command line arguments.
 * @returns options_t structure malloc'ed and filled upon choosen command
 *          line's option (in manage_command_line_options function).
 */
options_t *do_what_is_needed_from_command_line_options(int argc, char **argv)
{
    options_t *opt = NULL;  /** Structure to manage options from the command line can be freed when no longer needed */

    opt = manage_command_line_options(argc, argv);

    if (opt != NULL)
        {
            if (opt->version == TRUE)
                {
                    print_program_version(PROGRAM_NAME, SERVER_DATE, SERVER_VERSION, SERVER_AUTHORS, SERVER_LICENSE);
                    print_libraries_versions(PROGRAM_NAME);
                    print_selected_options(opt);
                    exit(EXIT_SUCCESS);
                }
        }

    return opt;
}
