/* at_opts_io.c --- 
 *
 * Copyright (C) 2002 Masatake YAMATO
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */ 

/* TODO: Error report/check */

#include "config.h"
#include "private.h"
#include "frontline.h"
#include <popt.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>

static void at_fitting_opts_color_write    (at_color_type * color, const char * symbol, FILE * fp);
static void at_fitting_opts_unsigned_write (unsigned val,          const char * symbol, FILE * fp);
static void at_fitting_opts_real_write     (at_real val,              const char * symbol, FILE * fp);
static void at_fitting_opts_bool_write     (at_bool val,              const char * symbol, FILE * fp);

#define CMD_NAME "autotrace"
#define LAST_VAR "$@"

int
at_fitting_opts_save (at_fitting_opts_type * opts, FILE * fp)
{
  g_return_val_if_fail (opts, -1);
  g_return_val_if_fail (fp, -1);

  fprintf(fp, "# %s\n", at_version(TRUE));
  fprintf(fp, "%s ", CMD_NAME);

#define member_write(member_symbol, type) G_STMT_START{			\
  gchar * copy_symbol = g_strdup(g_string(member_symbol));		\
  fl_str_replace_underscore(copy_symbol, '-');				\
  at_fitting_opts_##type##_write(opts->member_symbol, copy_symbol, fp);	\
  g_free(copy_symbol);							\
  }G_STMT_END

  /* at_fitting_opts_color_write(opts->background_color, "background-color", fp); */
  member_write(background_color, color);
  member_write(color_count, unsigned);
  member_write(corner_always_threshold, real);
  member_write(corner_surround, unsigned);
  member_write(corner_threshold, real);
  member_write(error_threshold, real);
  member_write(filter_iterations, unsigned);
  member_write(line_reversion_threshold, real);
  member_write(line_threshold, real);
  member_write(remove_adjacent_corners, bool);
  member_write(tangent_surround, unsigned);
  member_write(despeckle_tightness, real);
  member_write(centerline, bool);
  member_write(preserve_width, bool);
  member_write(width_factor, real);
#undef member_write
  
  fprintf(fp, "%s\n", LAST_VAR);
  return 0;
}

static void
at_fitting_opts_color_write    (at_color_type * value, const char * symbol, FILE * fp)
{
  if (value)
    fprintf(fp, "--%s=%.2X%.2X%.2X ", symbol, value->r, value->g, value->b);
}

static void
at_fitting_opts_unsigned_write (unsigned val, const char * symbol, FILE * fp)
{
  fprintf(fp, "--%s=%u ", symbol, val);
}

static void
at_fitting_opts_real_write (at_real val, const char * symbol, FILE * fp)
{
  fprintf(fp, "--%s=%f ", symbol, val);
}

static void
at_fitting_opts_bool_write (at_bool val, const char * symbol, FILE * fp)
{
  if (val)
    fprintf(fp, "--%s ", symbol);
}

at_fitting_opts_type *
at_fitting_opts_new_from_file(FILE * fp)
{
  const int buf_len = 512;
  char buf[buf_len];
  char * status;
  int len;

  int argc;
  char ** argv;
  at_fitting_opts_type * opts;
  int parse_result;

  g_return_val_if_fail (fp, NULL);
  
  /* 
   * TODO: Check version
   */
  status = fgets(buf, buf_len, fp);
  if (status == NULL)
    return NULL;

  /*
   * Read command line
   */
  status = fgets(buf, buf_len, fp);
  if (status == NULL)
    return NULL;
  /* Remove last newline */
  len = strlen(buf);
  if (buf[len -1] == '\n')
    buf[len -1] = '\0';

  parse_result = poptParseArgvString(buf, &argc, &argv);
  if (parse_result < 0)
    {
      /* TODO gnome_error_dialog_parented(poptStrerror(parse_result)); */
      g_warning ("%s", poptStrerror(parse_result));
      return NULL;
    }

  if ((strcmp(argv[0], CMD_NAME)) 
      || (strcmp(argv[argc - 1], LAST_VAR)))
    {
      free(argv);
      return NULL;
    }

  opts = at_fitting_opts_new_from_argv(argc, argv);
  free(argv);
  
  return opts;
}

/* TODO: Warning via GUI */
at_fitting_opts_type *
at_fitting_opts_new_from_argv(int argc, const char ** argv)

{
  struct poptOption table [at_fitting_opts_popt_table_length];
  poptContext optCon;
  int rc;
  at_fitting_opts_type * opts = at_fitting_opts_new();
  
  at_fitting_opts_popt_table_init(table, opts);
  optCon = poptGetContext(CMD_NAME, 
			  argc, argv, 
			  table, 0);  
  while ((rc = poptGetNextOpt(optCon)) > 0)
    /* Do nothing */;
  if (rc != -1)
    {
      gchar * msg = g_strdup_printf("%s: %s", poptBadOption(optCon, 0), poptStrerror(rc));
      /* TODO gnome_error_dialog(msg); */
      g_warning(msg);
      g_free(msg);
      at_fitting_opts_free(opts);
      opts = NULL;
    }

  poptFreeContext(optCon);
  return opts;
}

static void at_opts_table_add(struct poptOption * entry, char * longName, int  argInfo, void * datum);
static void at_opts_table_end(struct poptOption * entry);
static void opts_callback(poptContext con, enum poptCallbackReason reason, const struct poptOption * opt, char * arg, void * data);
static at_color_type * at_opts_parse_color (char * color_string);

void
at_fitting_opts_popt_table_init(struct poptOption * table, at_fitting_opts_type * opts)
{
  struct poptOption * table_cb, * table_background_color;
  
#define TABLE_ADD(name, type, slot) \
  at_opts_table_add(table++, g_string(name), POPT_ARG_##type, &opts->slot)

  at_opts_table_add(table++, NULL,    POPT_ARG_CALLBACK, opts_callback);
  table_cb = table - 1;
  TABLE_ADD(background-color,         STRING,   background_color);
  table_background_color       = table - 1;

  /* Next statement makes the callback can access background-color
     data slot. */
  table_cb->descrip = (void *)table_background_color;
  
  TABLE_ADD(color-count,              INT,      color_count);
  TABLE_ADD(corner-always-threshold,  FLOAT,    corner_always_threshold);
  TABLE_ADD(corner-surround,          INT,      corner_surround);
  TABLE_ADD(corner-threshold,         FLOAT,    corner_threshold);
  TABLE_ADD(error-threshold,          FLOAT,    error_threshold);
  TABLE_ADD(filter-iterations,        INT,      filter_iterations);
  TABLE_ADD(line-reversion-threshold, FLOAT,    line_reversion_threshold);
  TABLE_ADD(line-threshold,           FLOAT,    line_threshold);
  TABLE_ADD(remove-adjacent-corners,  NONE,     remove_adjacent_corners);
  TABLE_ADD(tangent-surround,         INT,      tangent_surround);
  TABLE_ADD(despeckle-level,          INT,      despeckle_level);
  TABLE_ADD(despeckle-tightness,      FLOAT,    despeckle_tightness);
  TABLE_ADD(centerline,               NONE,     centerline);
  TABLE_ADD(preserve_width,           NONE,     preserve_width);
  TABLE_ADD(width_factor,             FLOAT,    width_factor);
#undef TABLE_ADD
  at_opts_table_end(table++);
}

static void
at_opts_table_add(struct poptOption * entry, 
		  char * longName,
		  int  argInfo,
		  void * datum)
{
  switch(argInfo)
    {
    case POPT_ARG_CALLBACK:
      entry->longName  = NULL;
      entry->shortName = '\0';
      entry->argInfo = (argInfo | POPT_CBFLAG_POST);
      entry->arg = datum;
      entry->val = 0;
      entry->descrip = NULL;	
      entry->argDescrip = NULL;
      break;
    case POPT_ARG_NONE:
      *(at_bool *)datum = 0;
      /* Fall through */
    case POPT_ARG_INT:
    case POPT_ARG_FLOAT:
    case POPT_ARG_STRING:
      entry->longName  = longName;
      entry->shortName = '\0';
      entry->argInfo = (argInfo);
      entry->arg = datum;
      entry->val = 0;
      entry->descrip = "\n";
      entry->argDescrip = NULL;
      break;
    default:
      g_assert_not_reached();
    }
}

static void
at_opts_table_end(struct poptOption * entry)
{
  entry->longName = NULL;
  entry->shortName = '\0';
  entry->argInfo = 0;
  entry->arg = 0;
  entry->val = 0;
  entry->descrip = NULL;
  entry->argDescrip = NULL;
}

static void 
opts_callback(poptContext con, enum poptCallbackReason reason,
	      const struct poptOption * opt,
	      char * arg, void * data) 
{
  const struct poptOption * background_color_opt = data;
  if (reason == POPT_CALLBACK_REASON_POST
      && *(char **)background_color_opt->arg)
    *(at_color_type **)background_color_opt->arg = at_opts_parse_color(*(char **)background_color_opt->arg);
}

static at_color_type *
at_opts_parse_color (char * color_string)
{
  char r_string[5] = {'0', 'X'};
  char g_string[5] = {'0', 'X'};
  char b_string[5] = {'0', 'X'};
  
  unsigned int r, g, b;
  int i = 0;

  r_string[2] = color_string[i++];
  r_string[3] = color_string[i++];
  r_string[4] = '\0';
  g_string[2] = color_string[i++];
  g_string[3] = color_string[i++];
  g_string[4] = '\0';
  b_string[2] = color_string[i++];
  b_string[3] = color_string[i++];
  b_string[4] = '\0';
  
  if (sscanf(r_string, "%X", &r)    != 1
      || sscanf(g_string, "%X", &g) != 1
      || sscanf(b_string, "%X", &b) != 1)
    return NULL;
  g_assert(r < 256 && g < 256 && b < 256);

  return at_color_new((unsigned char)r, 
		      (unsigned char)g, 
		      (unsigned char)b);
}
