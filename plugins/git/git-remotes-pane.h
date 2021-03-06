/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GIT_REMOTES_PANE_H_
#define _GIT_REMOTES_PANE_H_

#include <glib-object.h>
#include "git-pane.h"

G_BEGIN_DECLS

#define GIT_TYPE_REMOTES_PANE             (git_remotes_pane_get_type ())
#define GIT_REMOTES_PANE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_REMOTES_PANE, GitRemotesPane))
#define GIT_REMOTES_PANE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_REMOTES_PANE, GitRemotesPaneClass))
#define GIT_IS_REMOTES_PANE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_REMOTES_PANE))
#define GIT_IS_REMOTES_PANE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_REMOTES_PANE))
#define GIT_REMOTES_PANE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_REMOTES_PANE, GitRemotesPaneClass))

typedef struct _GitRemotesPaneClass GitRemotesPaneClass;
typedef struct _GitRemotesPane GitRemotesPane;
typedef struct _GitRemotesPanePriv GitRemotesPanePriv;

struct _GitRemotesPaneClass
{
	GitPaneClass parent_class;
};

struct _GitRemotesPane
{
	GitPane parent_instance;

	GitRemotesPanePriv *priv;
};

GType git_remotes_pane_get_type (void) G_GNUC_CONST;
AnjutaDockPane *git_remotes_pane_new (Git *plugin);
gchar *git_remotes_pane_get_selected_remote (GitRemotesPane *self);

G_END_DECLS

#endif /* _GIT_REMOTES_PANE_H_ */
