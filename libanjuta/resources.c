/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * resources.h
 * Copyright (C) Naba Kumar  <naba@gnome.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:resources
 * @title: Program resources
 * @short_description: Application resource management
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/resources.h
 * 
 */

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <unistd.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>

GtkWidget *
anjuta_res_lookup_widget (GtkWidget * widget, const gchar * widget_name)
{
	GtkWidget *parent, *found_widget;

	for (;;)
	{
		if (GTK_IS_MENU (widget))
			parent =
				gtk_menu_get_attach_widget (GTK_MENU
							    (widget));
		else
			parent = gtk_widget_get_parent (widget);
		if (parent == NULL)
			break;
		widget = parent;
	}

	found_widget = (GtkWidget *) g_object_get_data (G_OBJECT (widget),
							  widget_name);
	if (!found_widget)
		g_warning (_("Widget not found: %s"), widget_name);
	return found_widget;
}

GtkWidget *
anjuta_res_get_image (const gchar * pixfile)
{
	GtkWidget *pixmap;
	gchar *pathname;
	
	if (!pixfile || !pixfile[0])
		return gtk_image_new ();

	pathname = anjuta_res_get_pixmap_file (pixfile);
	if (!pathname)
	{
		g_warning (_("Could not find application pixmap file: %s"),
			   pixfile);
		return gtk_image_new ();
	}
	pixmap = gtk_image_new_from_file (pathname);
	g_free (pathname);
	return pixmap;
}

GtkWidget *
anjuta_res_get_image_sized (const gchar * pixfile, gint width, gint height)
{
	GtkWidget *pixmap;
	GdkPixbuf *pixbuf;
	gchar *pathname;
	
	if (!pixfile || !pixfile[0])
		return gtk_image_new ();

	pathname = anjuta_res_get_pixmap_file (pixfile);
	if (!pathname)
	{
		g_warning (_("Could not find application pixmap file: %s"),
			   pixfile);
		return gtk_image_new ();
	}
	pixbuf = gdk_pixbuf_new_from_file_at_size (pathname, width, height, NULL);
	pixmap = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	g_free (pathname);
	return pixmap;
}

/* All the return strings MUST be freed */
gchar *
anjuta_res_get_pixmap_dir ()
{
	gchar* path;
	path = g_strdup (PACKAGE_PIXMAPS_DIR);
	if (g_file_test (path, G_FILE_TEST_IS_DIR))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_data_dir ()
{
	gchar* path;
	path = g_strdup (PACKAGE_DATA_DIR);
	if (g_file_test (path, G_FILE_TEST_IS_DIR))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_help_dir ()
{
	gchar* path;
	path = g_strdup (PACKAGE_HELP_DIR);
	if (g_file_test (path, G_FILE_TEST_IS_DIR))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_help_dir_locale (const gchar * locale)
{
	gchar* path;
	if (locale)
		path = g_strconcat (PACKAGE_HELP_DIR, "/", locale, NULL);
	else
		path = g_strdup (PACKAGE_HELP_DIR);
	if (g_file_test (path, G_FILE_TEST_IS_DIR))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_doc_dir ()
{
	gchar* path;
	path = g_strdup (PACKAGE_DOC_DIR);
	if (g_file_test (path, G_FILE_TEST_IS_DIR))
		return path;
	g_free (path);
	return NULL;
}

/* All the return strings MUST be freed */
gchar *
anjuta_res_get_pixmap_file (const gchar * pixfile)
{
	gchar* path;
	g_return_val_if_fail (pixfile != NULL, NULL);
	path = g_strconcat (PACKAGE_PIXMAPS_DIR, "/", pixfile, NULL);
	if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
		return path;
	g_warning ("Pixmap file not found: %s", path);
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_data_file (const gchar * datafile)
{
	gchar* path;
	g_return_val_if_fail (datafile != NULL, NULL);
	path = g_strconcat (PACKAGE_DATA_DIR, "/", datafile, NULL);
	if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_help_file (const gchar * helpfile)
{
	gchar* path;
	g_return_val_if_fail (helpfile != NULL, NULL);
	path = g_strconcat (PACKAGE_HELP_DIR, "/", helpfile, NULL);
	if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
		return path;
	g_free (path);
	return NULL;
}
	
gchar *
anjuta_res_get_help_file_locale (const gchar * helpfile, const gchar * locale)
{
	gchar* path;
	g_return_val_if_fail (helpfile != NULL, NULL);
	if (locale)
		path = g_strconcat (PACKAGE_HELP_DIR, "/", locale, "/",
				    helpfile, NULL);
	else
		path = g_strconcat (PACKAGE_HELP_DIR, "/", helpfile, NULL);
	if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
		return path;
	g_free (path);
	return NULL;
}

gchar *
anjuta_res_get_doc_file (const gchar * docfile)
{
	gchar* path;
	g_return_val_if_fail (docfile != NULL, NULL);
	path = g_strconcat (PACKAGE_DOC_DIR, "/", docfile, NULL);
	if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
		return path;
	g_free (path);
	return NULL;
}

#if 0
/* File type icons 16x16 */
GdkPixbuf *
anjuta_res_get_icon_for_file (PropsID props, const gchar *filename)
{
	gchar *value;
	GdkPixbuf *pixbuf;
	gchar *file;
	
	g_return_val_if_fail (filename != NULL, NULL);
	file = g_path_get_basename (filename);
	value = prop_get_new_expand (props, "icon.", file);
	if (value == NULL)
		value = g_strdup ("file_text.png");
	pixbuf = anjuta_res_get_pixbuf (value);
	g_free (value);
	g_free (file);
	return pixbuf;
}
#endif

void
anjuta_res_help_search (const gchar * word)
{
	if(word)
	{
		fprintf(stderr, "Word is %s\n", word);
		if(fork()==0)
		{
			execlp("devhelp", "devhelp", "-s", word, NULL);
			g_warning (_("Cannot execute command: \"%s\""), "devhelp");
			_exit(1);
		}
	}
	else
	{
		if(fork()==0)
		{
			execlp("devhelp", "devhelp", NULL);
			g_warning (_("Cannot execute command: \"%s\""), "devhelp");
			_exit(1);
		}
	}
}

void
anjuta_res_url_show (const gchar *url)
{
	gchar *open[3];

	if (!anjuta_util_prog_is_installed ("xdg-open", TRUE))
		return;
	
	open[0] = "xdg-open";
	open[1] = (gchar *)url;
	open[2] = NULL;
					
	g_spawn_async (NULL, open, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
}
