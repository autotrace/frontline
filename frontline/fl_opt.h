/* fl_opt.h --- Frontline Option Widget (in short fl_opt) 
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


#ifndef FL_OPT_H
#define FL_OPT_H 

BEGIN_GNOME_DECLS  

#define FRONTLINE_TYPE_OPTION                (frontline_option_get_type ())
#define FRONTLINE_OPTION(obj)                (GTK_CHECK_CAST ((obj), FRONTLINE_TYPE_OPTION, FrontlineOption))
#define FRONTLINE_OPTION_CLASS(klass)        (GTK_CHECK_CLASS_CAST ((klass), FRONTLINE_TYPE_OPTION, FrontlineOptionClass))
#define FRONTLINE_IS_OPTION(obj)             (GTK_CHECK_TYPE ((obj), FRONTLINE_TYPE_OPTION))
#define FRONTLINE_IS_OPTION_CLASS(klass)     (GTK_CHECK_CLASS_TYPE ((klass), FRONTLINE_TYPE_OPTION))

typedef struct _FrontlineOption FrontlineOption;
typedef struct _FrontlineOptionClass FrontlineOptionClass;
typedef struct _FrontlineOptionPriv FrontlineOptionPriv;
struct _FrontlineOption 
{
  GtkVBox vbox;
  gint block_count;
  gboolean value_changed;
    
  GtkWidget * undo_button;
  GtkWidget * redo_button;
  GtkWidget * load_button;
  GtkWidget * save_button;
    
  GtkObject  * undo_seq;

  /* Don't access directly 
     Use frontline_option_set_value and
     frontline_option_get_value */
  at_fitting_opts_type * opts; 
  gboolean in_undo_action;

  GtkWidget * filesel;
    
  FrontlineOptionPriv * priv;
};

struct _FrontlineOptionClass
{
  GtkVBoxClass parent_class;
  void (* value_changed)	 (FrontlineOption * fl_opt);
};

GtkType    frontline_option_get_type (void);
GtkWidget* frontline_option_new      (void);
GtkWidget* frontline_option_new_with_value (at_fitting_opts_type * value);

/* FrontlineOption is the owner of returned value. 
   Don't free. */
at_fitting_opts_type * frontline_option_get_value (FrontlineOption *);
  
void frontline_option_set_value (FrontlineOption *, at_fitting_opts_type * value);
void frontline_option_value_changed (FrontlineOption *);
void frontline_option_value_changed_block (FrontlineOption *);
void frontline_option_value_changed_unblock (FrontlineOption *);

void frontline_option_clear_undo_sequence (FrontlineOption *);
  
END_GNOME_DECLS

#endif /* Not def: FL_OPT_H */
