/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-trunk
 * Copyright (C) Johannes Schmid 2008 <jhs@gnome.org>
 * 
 * anjuta-trunk is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta-trunk is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "anjuta-bookmarks.h"
#include "anjuta-docman.h"
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#define BOOKMARKS_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANJUTA_TYPE_BOOKMARKS, AnjutaBookmarksPrivate))

#define ANJUTA_STOCK_BOOKMARK_TOGGLE          "anjuta-bookmark-toggle"

typedef struct _AnjutaBookmarksPrivate AnjutaBookmarksPrivate;

struct _AnjutaBookmarksPrivate 
{
	GtkWidget* window;
	GtkWidget* tree;
	GtkTreeModel* model;
	
	GtkWidget* button_add;
	GtkWidget* button_remove;	
	
	DocmanPlugin* docman;
};

enum
{
	COLUMN_TEXT = 0,
	COLUMN_FILE,
	COLUMN_LINE,
	COLUMN_HANDLE,
	N_COLUMNS
};

G_DEFINE_TYPE (AnjutaBookmarks, anjuta_bookmarks, G_TYPE_OBJECT);

static void
on_document_changed (AnjutaDocman *docman, IAnjutaDocument *doc,
					 AnjutaBookmarks *bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	gboolean status =  IANJUTA_IS_EDITOR(doc);
	gtk_widget_set_sensitive (GTK_WIDGET(priv->button_add), status);
}

static void
on_add_clicked (GtkWidget* button, AnjutaBookmarks* bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	IAnjutaDocument* doc =
		anjuta_docman_get_current_document (ANJUTA_DOCMAN(priv->docman->docman));
	g_return_if_fail (IANJUTA_IS_EDITOR(doc));
	IAnjutaEditor* editor = IANJUTA_EDITOR(doc);
	anjuta_bookmarks_add (bookmarks, editor, ianjuta_editor_get_lineno (editor, NULL));						 
}

static void
on_remove_clicked (GtkWidget* button, AnjutaBookmarks* bookmarks)
{					 
	anjuta_bookmarks_remove (bookmarks);
}

static void
on_row_activate (GtkTreeView* view, GtkTreePath* path,
				 GtkTreeViewColumn* column, AnjutaBookmarks* bookmarks)
{
	GtkTreeIter iter;
	GFile* file;
	gint line;
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	IAnjutaEditor* editor;
	gtk_tree_model_get_iter (priv->model, &iter, path);
	gtk_tree_model_get (priv->model, &iter,
						COLUMN_FILE, &file,
						COLUMN_LINE, &line,
						-1);
	editor = anjuta_docman_goto_file_line (ANJUTA_DOCMAN(priv->docman->docman), file, line);
	g_object_unref (file);
}

static void
on_document_added (AnjutaDocman* docman, IAnjutaDocument* doc,
				   AnjutaBookmarks* bookmarks)
{
	IAnjutaMarkable* markable;
	GtkTreeIter iter;
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	
	if (!IANJUTA_IS_MARKABLE(doc))
		return;
	
	markable = IANJUTA_MARKABLE(doc);
	if (!gtk_tree_model_get_iter_first (priv->model, &iter))
		return;
	do
	{
		GFile* file;
		GFile* editor_file = ianjuta_file_get_file (IANJUTA_FILE(doc), NULL);
		gint line;
		gtk_tree_model_get (priv->model, &iter,
							COLUMN_FILE, &file,
							COLUMN_LINE, &line,
							-1);
		if (g_file_equal (file, editor_file))
		{
			if (!ianjuta_markable_is_marker_set (markable,
												 line,
												 IANJUTA_MARKABLE_BOOKMARK,
												 NULL))
			{
				int handle = ianjuta_markable_mark (markable, line,
												   IANJUTA_MARKABLE_BOOKMARK,
												   NULL);
				gtk_list_store_set (GTK_LIST_STORE(priv->model),
									&iter,
									COLUMN_HANDLE,
									handle, -1);
			}
		}
		g_object_unref (editor_file);
		g_object_unref (file);		
	}
	while (gtk_tree_model_iter_next (priv->model, &iter));
	
	
}

static void
on_selection_changed (GtkTreeSelection* selection, AnjutaBookmarks* bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	gboolean status = gtk_tree_selection_get_selected (selection, NULL, NULL);
	gtk_widget_set_sensitive (priv->button_remove, status);
}

static void
anjuta_bookmarks_init (AnjutaBookmarks *bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	GtkCellRenderer* renderer;
	GtkTreeViewColumn* column;
	GtkWidget* scrolled_window;
	GtkWidget* button_box;
	GtkTreeSelection* selection;
	
	priv->window = gtk_vbox_new (FALSE, 5);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scrolled_window),
										 GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start (GTK_BOX (priv->window), 
						scrolled_window,
						TRUE, TRUE, 0);
	
	priv->model = GTK_TREE_MODEL(gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, 
													 G_TYPE_OBJECT, G_TYPE_INT, G_TYPE_INT));
	priv->tree = gtk_tree_view_new_with_model (priv->model);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(priv->tree), FALSE);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes ("Bookmark", renderer,
													   "text", COLUMN_TEXT, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(priv->tree), column);
	gtk_container_add (GTK_CONTAINER(scrolled_window),
					   priv->tree);
	
	g_signal_connect (G_OBJECT(priv->tree), "row-activated", G_CALLBACK(on_row_activate),
					  bookmarks);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->tree));
	g_signal_connect (G_OBJECT(selection), "changed", G_CALLBACK(on_selection_changed),
					  bookmarks);
	
	
	button_box = gtk_hbutton_box_new ();
	priv->button_add = gtk_button_new_from_stock (GTK_STOCK_ADD);
	priv->button_remove = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
	g_signal_connect (G_OBJECT(priv->button_add), "clicked", G_CALLBACK(on_add_clicked), bookmarks);
	g_signal_connect (G_OBJECT(priv->button_remove), "clicked", G_CALLBACK(on_remove_clicked), bookmarks);
	gtk_widget_set_sensitive (GTK_WIDGET(priv->button_add), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET(priv->button_remove), FALSE);
	gtk_box_pack_start_defaults (GTK_BOX(button_box), priv->button_add);
	gtk_box_pack_start_defaults (GTK_BOX(button_box), priv->button_remove);

	gtk_box_pack_start(GTK_BOX(priv->window), 
					   button_box,
					   FALSE, FALSE, 0);
	gtk_widget_show_all (priv->window);
}

static void
anjuta_bookmarks_finalize (GObject *object)
{
	AnjutaBookmarks* bookmarks = ANJUTA_BOOKMARKS (object);
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	anjuta_shell_remove_widget (ANJUTA_PLUGIN(priv->docman)->shell,
								priv->window,
								NULL);
	
	G_OBJECT_CLASS (anjuta_bookmarks_parent_class)->finalize (object);
}

static void
anjuta_bookmarks_class_init (AnjutaBookmarksClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = anjuta_bookmarks_finalize;
	
	g_type_class_add_private (klass, sizeof (AnjutaBookmarksPrivate));
}


AnjutaBookmarks*
anjuta_bookmarks_new (DocmanPlugin* docman)
{
	AnjutaBookmarks* bookmarks = ANJUTA_BOOKMARKS (g_object_new (ANJUTA_TYPE_BOOKMARKS, 
																 NULL));
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	priv->docman = docman;
	
	anjuta_shell_add_widget (ANJUTA_PLUGIN(docman)->shell,
							 priv->window,
							 "bookmarks",
							 _("Bookmarks"),
							 ANJUTA_STOCK_BOOKMARK_TOGGLE,
							 ANJUTA_SHELL_PLACEMENT_RIGHT,
							 NULL);
	
	g_signal_connect (G_OBJECT(docman->docman), "document-changed",
					  G_CALLBACK(on_document_changed), bookmarks);
	g_signal_connect (G_OBJECT(docman->docman), "document-added",
					  G_CALLBACK(on_document_added), bookmarks);	
	return bookmarks;
}

void
anjuta_bookmarks_add (AnjutaBookmarks* bookmarks, IAnjutaEditor* editor, gint line)
{
	g_return_if_fail (IANJUTA_IS_MARKABLE(editor));
	IAnjutaMarkable* markable = IANJUTA_MARKABLE(editor);
	GtkTreeIter iter;
	gint handle;
	gchar* text;
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	GFile* file;
	
	/* If there is already a marker -> do nothing */
	if (ianjuta_markable_is_marker_set (markable, line, IANJUTA_MARKABLE_BOOKMARK, NULL))
		return;
	
	handle = ianjuta_markable_mark (markable, line, IANJUTA_MARKABLE_BOOKMARK, NULL);
	
	gtk_list_store_append (GTK_LIST_STORE(priv->model), &iter);
	text = g_strdup_printf ("%s:%d",  ianjuta_document_get_filename(IANJUTA_DOCUMENT(editor), NULL),
							line);
	file = ianjuta_file_get_file(IANJUTA_FILE(editor), NULL);
	gtk_list_store_set (GTK_LIST_STORE(priv->model), &iter, 
						COLUMN_TEXT, text,
						COLUMN_FILE, file,
						COLUMN_LINE, line,
						COLUMN_HANDLE, handle,
						-1);
	g_free(text);
	g_object_unref (file);
}

void
anjuta_bookmarks_remove (AnjutaBookmarks* bookmarks)
{
	AnjutaBookmarksPrivate* priv = BOOKMARKS_GET_PRIVATE(bookmarks);
	GtkTreeIter iter;
	GtkTreeSelection* selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->tree));
	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		GFile* file;
		gint line;
		IAnjutaEditor* editor;
		gtk_tree_model_get (priv->model, &iter, 
							COLUMN_FILE, &file,
							COLUMN_LINE, &line,
							-1);
		editor = IANJUTA_EDITOR(anjuta_docman_get_document_for_file (ANJUTA_DOCMAN(priv->docman->docman),
																	 file));
		if (editor)
		{
			if (ianjuta_markable_is_marker_set (IANJUTA_MARKABLE(editor),
												line, IANJUTA_MARKABLE_BOOKMARK, NULL))
			{
				ianjuta_markable_unmark (IANJUTA_MARKABLE(editor), line,
										 IANJUTA_MARKABLE_BOOKMARK, NULL);
			}
		}
		g_object_unref (file);
		
		gtk_list_store_remove (GTK_LIST_STORE (priv->model), &iter);
	}
}

void
anjuta_bookmarks_session_save (AnjutaBookmarks* bookmarks, AnjutaSession* session)
{
	/* TODO: Add public function implementation here */
}

void
anjuta_bookmarks_session_load (AnjutaBookmarks* bookmarks, AnjutaSession* session)
{
	/* TODO: Add public function implementation here */
}