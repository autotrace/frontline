/* fl_prog.h --- Front Progress Widget (fl_prog)
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


#ifndef FL_PROG_H
#define FL_PROG_H 

BEGIN_GNOME_DECLS  

#define FRONTLINE_TYPE_PROGRESS                (frontline_progress_get_type ())
#define FRONTLINE_PROGRESS(obj)                (GTK_CHECK_CAST ((obj), FRONTLINE_TYPE_PROGRESS, FrontlineProgress))
#define FRONTLINE_PROGRESS_CLASS(klass)        (GTK_CHECK_CLASS_CAST ((klass), FRONTLINE_TYPE_PROGRESS, FrontlineProgressClass))
#define FRONTLINE_IS_PROGRESS(obj)             (GTK_CHECK_TYPE ((obj), FRONTLINE_TYPE_PROGRESS))
#define FRONTLINE_IS_PROGRESS_CLASS(klass)     (GTK_CHECK_CLASS_TYPE ((klass), FRONTLINE_TYPE_PROGRESS))

typedef struct _FrontlineProgress FrontlineProgress;
typedef struct _FrontlineProgressClass FrontlineProgressClass;
struct _FrontlineProgress 
{
  GtkHBox hbox;
  GtkObject * adj;
  GtkWidget * progress_bar;
  GtkWidget * stop_button;
};

struct _FrontlineProgressClass
{
  GtkHBoxClass parent_class;
  void (* stopped)   (FrontlineProgress  *fl_prog);
};

GtkType    frontline_progress_get_type (void);
GtkWidget* frontline_progress_new      (void);
void       frontline_progress_stopped  (FrontlineProgress * fl_prog);

END_GNOME_DECLS

#endif /* Not def: FL_PROG_H */
