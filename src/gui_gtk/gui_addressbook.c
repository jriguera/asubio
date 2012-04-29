
/*
 *
 * FILE: gui_addressbook.c
 *
 * Copyright: (c) Junio 2003, Jose Riguera <jriguera@gmail.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ***************************************************************************/

#include "gui_addressbook.h"




static gboolean destroy_addressbook_window ();
static GtkTreeIter *search_item_user (GtkTreeView *treeview, gchar *search, gboolean column_name, gboolean column_user, gboolean column_description, gboolean column_photo, gboolean ignore_case, gboolean search_from_sel);
static void create_dialog_search (GtkTreeView *treeview);
static void dialog_search ();
static void menu_item_toolbar (gpointer data, guint action, GtkWidget *widget);
static gboolean save_items (gchar *filename);
static void dialog_about ();
static void destroy_window ();
static void button_open ();
static void button_new ();
static void button_save_as ();
static void button_save ();
static void new_items ();
static gboolean create_items (gchar *filename);
static void cell_edited_name (GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, gpointer data);
static void cell_edited_user (GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, gpointer data);
static void cell_edited_check (GtkCellRendererText *cell, const gchar *path_string, gpointer data);
static void cell_edited_description (GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, GtkTreeView *treeview);
static void add_item_user (GtkWidget *button, gpointer data);
static gboolean remove_item_user (GtkWidget *widget, GtkTreeView *treeview);
static gboolean cb_function ();
static void add_columns (GtkTreeView *treeview);
static void selection_item_user_changed (GtkTreeSelection *selection, GtkTreeView  *treeview);
static void create_file_selection (GtkWidget *widget, GtkTreeView *treeview);




static GtkWidget *toolbar;
static GtkWidget *window;
static GtkWidget *statusbar;
static GtkListStore *model = NULL;
static GtkWidget *treeview;
static ItemUser *user_default = NULL;
static gint user_counter = 0;
static gboolean modified = FALSE;
static gchar *filename = NULL;
static CB_gui_addbook callback = NULL;


static GtkItemFactoryEntry menu_items[] =
{
    { "/_Fichero",                  NULL,         0,                          0,     "<Branch>",    NULL           },
    { "/_Fichero/sep1",             NULL,         0,                          0,     "<Tearoff>",   NULL           },
    { "/_Fichero/_Nuevo",           "<control>N", button_new,                 0,     "<StockItem>", GTK_STOCK_NEW  },
    { "/_Fichero/_Abrir",           "<control>O", button_open,                0,     "<StockItem>", GTK_STOCK_OPEN },
    { "/_Fichero/_Guardar",         "<control>S", button_save,                0,     "<StockItem>", GTK_STOCK_SAVE },
    { "/_Fichero/Guardar _Como...", NULL,         button_save_as,             0,     "<StockItem>", GTK_STOCK_SAVE },
    { "/_Fichero/sep2",             NULL,         0,                          0,     "<Separator>", NULL           },
    { "/_Fichero/_Salir",           "<control>Q", destroy_window,             0,     "<StockItem>", GTK_STOCK_QUIT },

    { "/_Editar",                   NULL,         0,                    0,           "<Branch>",    NULL           },
    { "/_Editar/sep3",              NULL,         0,                    0,           "<Tearoff>",   NULL           },
    { "/_Editar/_Deshacer",         NULL,         0,                    0,           "<StockItem>", GTK_STOCK_UNDO },
    { "/_Editar/_Rehacer",          NULL,         0,                    0,           "<StockItem>", GTK_STOCK_REDO },
    { "/_Editar/sep4",              NULL,         0,                    0,           "<Separator>", NULL           },
    { "/_Editar/_Buscar",           "<control>F", dialog_search,        0,           "<StockItem>", GTK_STOCK_FIND },

    { "/_Preferencias",                   NULL,         0,                  0,       "<Branch>",                     NULL},
    { "/_Preferencias/sep4",              NULL,         0,                  0,       "<Tearoff>",                    NULL},
    { "/_Preferencias/_Toolbar",          NULL,         0,                  0,       "<Branch>",                     NULL},
    { "/_Preferencias/_Toolbar/Normal",   NULL,         menu_item_toolbar,  0,       "<RadioItem>",                  NULL},
    { "/_Preferencias/_Toolbar/Minima",   NULL,         menu_item_toolbar,  0,       "/Preferencias/Toolbar/Normal", NULL},
    { "/_Preferencias/_Toolbar/Oculta",   NULL,         menu_item_toolbar,  0,       "/Preferencias/Toolbar/Normal", NULL},

    { "/_Ayuda",                          NULL,         0,                   0,      "<LastBranch>", NULL                },
    { "/_Ayuda/_Acerca de ...",           NULL,         dialog_about,        0, "<StockItem>",  GTK_STOCK_HELP      },
};




// Functions



static gboolean destroy_addressbook_window ()
{
    GtkWidget *dialog;
    gint result;


    if (modified)
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, "Los datos han sido modificados, Continuar sin guardar ?");
	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if (result != GTK_RESPONSE_OK) return TRUE;

        g_free(user_default->name);
        g_free(user_default->user);
	g_free(user_default->host);
        g_free(user_default->photo);
        g_free(user_default->description);
	g_free(user_default);
	gtk_list_store_clear(GTK_LIST_STORE(model));
	if (filename != NULL) g_free(filename);
    }
    gtk_widget_destroy(window);
    window = NULL;

    return FALSE;
}



static void dialog_about ()
{
    create_dialog_about(window);
}



static void destroy_window ()
{
    destroy_addressbook_window();
}



static GtkTreeIter *search_item_user (GtkTreeView *treeview,
				      gchar *search,
				      gboolean column_name,
				      gboolean column_user,
				      gboolean column_description,
				      gboolean column_photo,
				      gboolean ignore_case,
				      gboolean search_from_sel)
{
    GtkTreeIter iter;
    GtkTreeIter *ant_iter;
    gboolean valid;
    gboolean no_find;
    gchar *search_data;
    GtkTreeModel *model;
    GtkTreeSelection *selection;


    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    selection = gtk_tree_view_get_selection(treeview);

    if (search_from_sel)
    {
	valid = gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), &model, &iter);
	valid = gtk_tree_model_iter_next(model, &iter);
        if (!valid) valid = gtk_tree_model_get_iter_first(model, &iter);
    }
    else valid = gtk_tree_model_get_iter_first(model, &iter);

    no_find = TRUE;
    if (ignore_case) search_data = g_ascii_strdown(search, 500);
    else search_data = g_strndup(search, 500);

    ant_iter = gtk_tree_iter_copy(&iter);
    while (valid && no_find)
    {
	gchar *str_name;
	gchar *str_user;
	gchar *str_description;
	gchar *str_photo;
	gchar str_all[1009];
        gchar *str_search;


	gtk_tree_model_get(model, &iter,
			   ADB_COLUMN_NAME, &str_name,
			   ADB_COLUMN_USER, &str_user,
			   ADB_COLUMN_DESCRIPTION, &str_description,
			   ADB_COLUMN_PHOTO, &str_photo,
			   -1);
	memset(str_all, 0, 1009);
	if (column_name)
	{
	    strncat(str_all, str_name, 100);
	    strncat(str_all, " | ", 3);
	}
	if (column_user)
	{
	    strncat(str_all, str_user, 100);
	    strncat(str_all, " | ", 3);
	}
	if (column_description)
	{
	    strncat(str_all, str_description, 500);
	    strncat(str_all, " | ", 3);
	}
	if (column_photo) strncat(str_all, str_photo, 300);

	if (ignore_case) str_search = g_ascii_strdown(str_all, 1009);
	else str_search = g_strndup(str_all, 1009);

	if (g_strrstr(str_search, search_data) != NULL)
	{
	    no_find = FALSE;
            gtk_tree_selection_select_iter(selection, &iter);
	}
        g_free(str_search);
	g_free(str_name);
	g_free(str_user);
	g_free(str_description);
	g_free(str_photo);

        gtk_tree_iter_free(ant_iter);
        ant_iter = gtk_tree_iter_copy(&iter);
	valid = gtk_tree_model_iter_next(model, &iter);
    }
    if (no_find)
    {
	gtk_tree_selection_unselect_all(selection);
	return NULL;
    }
    gtk_tree_selection_select_iter(selection, ant_iter);
    return ant_iter;
}



static void create_dialog_search (GtkTreeView *treeview)
{
    GtkWidget *dialog;
    GtkWidget *label_search;
    GtkWidget *entry_search;
    GtkWidget *hbox1;
    GtkWidget *hbox2;
    GtkWidget *frame_options;
    GtkWidget *check_button1;
    GtkWidget *check_button2;
    GtkWidget *check_button3;
    GtkWidget *check_button4;
    GtkWidget *check_button5;
    GtkWidget *check_button6;
    GtkWidget *table;
    GtkWidget *hseparator;
    gint result;
    GtkTooltips *tips_help;
    static gchar *text_search = NULL;
    static gboolean state_button1 = TRUE;
    static gboolean state_button2 = TRUE;
    static gboolean state_button3 = FALSE;
    static gboolean state_button4 = FALSE;
    static gboolean state_button5 = FALSE;
    static gboolean state_button6 = TRUE;


    dialog = gtk_dialog_new_with_buttons("Buscar", GTK_WINDOW(window),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_FIND, GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
					 NULL);
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
    tips_help = gtk_tooltips_new();

    label_search = gtk_label_new("Buscar :");
    entry_search = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_search, "Palabra o frase a desea buscar ...", NULL);
    if (text_search != NULL) gtk_entry_set_text(GTK_ENTRY(entry_search), text_search);
    gtk_entry_set_max_length(GTK_ENTRY(entry_search), 250);
    hbox1 = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox1), label_search, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox1), entry_search, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox1, FALSE, FALSE, 15);

    hbox2 = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox2, TRUE, TRUE, 5);
    frame_options = gtk_frame_new("Opciones de busqueda");
    gtk_box_pack_start(GTK_BOX(hbox2), frame_options, TRUE, TRUE, 5);

    check_button1 = gtk_check_button_new_with_label("Buscar en Nombre");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), check_button1, "Habilitar la busqueda en la columna 'Nombre'", NULL);
    check_button2 = gtk_check_button_new_with_label("Buscar en Usuario");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), check_button2, "Habilitar la busqueda en la columna 'Usuario'", NULL);
    check_button3 = gtk_check_button_new_with_label("Buscar en Descripcion");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), check_button3, "Habilitar la busqueda en la columna 'Descripcion'", NULL);
    check_button4 = gtk_check_button_new_with_label("Buscar en Fotografia");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), check_button4, "Habilitar la busqueda en la columna 'Fotografia'", NULL);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button1), state_button1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button2), state_button2);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button3), state_button3);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button4), state_button4);

    table = gtk_table_new(2, 5, FALSE);
    gtk_container_add(GTK_CONTAINER(frame_options), table);

    gtk_table_attach(GTK_TABLE(table), check_button1, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5);
    gtk_table_attach(GTK_TABLE(table), check_button2, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 0);
    gtk_table_attach(GTK_TABLE(table), check_button3, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5);
    gtk_table_attach(GTK_TABLE(table), check_button4, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 0);

    hseparator = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(table), hseparator, 0, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5);

    check_button5 = gtk_check_button_new_with_label("Buscar a partir de la fila seleccionada");
    check_button6 = gtk_check_button_new_with_label("Ignorar mayusculas y minusculas");

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button5), state_button5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button6), state_button6);

    gtk_table_attach(GTK_TABLE(table), check_button5, 0, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 0);
    gtk_table_attach(GTK_TABLE(table), check_button6, 0, 2, 4, 5, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5);

    gtk_widget_show_all(dialog);
    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_ACCEPT)
    {
        GtkTreeIter *iter;


	update_statusbar("Buscando coincidencias con el patron ... ", statusbar);
	state_button1 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button1));
	state_button2 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button2));
	state_button3 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button3));
	state_button4 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button4));
	state_button5 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button5));
	state_button6 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button6));
	g_free(text_search);
	text_search = g_strndup(gtk_entry_get_text(GTK_ENTRY(entry_search)), 500);
	iter = search_item_user(treeview,
				text_search,
				state_button1,
				state_button2,
				state_button3,
				state_button4,
				state_button6,
				state_button5);

	if (iter == NULL) update_statusbar("No se han encontrado coincidencias con el patron de busqueda", statusbar);
        else update_statusbar("Item encontrado seleccionado", statusbar);
    }
    gtk_widget_destroy (dialog);
}



static void dialog_search ()
{
    create_dialog_search(GTK_TREE_VIEW(treeview));
}



static void menu_item_toolbar (gpointer data, guint action, GtkWidget *widget)
{
    UNUSED(data);
    UNUSED(action);


    if ((strstr(gtk_item_factory_path_from_widget(widget), "Normal")) != NULL)
    {
        gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show(GTK_WIDGET(toolbar));
    }
    if ((strstr(gtk_item_factory_path_from_widget(widget), "Minima")) != NULL)
    {
        gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
        gtk_widget_show(GTK_WIDGET(toolbar));
    }
    if ((strstr(gtk_item_factory_path_from_widget(widget), "Oculta")) != NULL)
    {
        gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_LARGE_TOOLBAR);
        gtk_widget_hide(GTK_WIDGET(toolbar));
    }
}



static gboolean save_items (gchar *filename)
{
    xmlDocPtr doc;
    ItemUser *user;
    GError *tmp_error = NULL;
    GtkTreeIter iter;
    gint number;
    gchar *nameuser;
    gchar **userhost;
    gint i;


    if (filename == NULL) return FALSE;
    doc = xml_newdoc();
    user = (ItemUser *) g_malloc0(sizeof(ItemUser));

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
    for (i=1 ; i<=user_counter; i++)
    {
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
			   ADB_COLUMN_NUMBER, &number,
			   ADB_COLUMN_NAME, &(user->name),
			   ADB_COLUMN_USER, &(nameuser),
			   ADB_COLUMN_CHECK, &(user->ignore),
			   ADB_COLUMN_DESCRIPTION, &(user->description),
			   ADB_COLUMN_PHOTO, &(user->photo),
			   -1);

	userhost = g_strsplit(nameuser, "@", 2);
	if (userhost != NULL)
	{
            user->user = userhost[0];
            user->host = userhost[1];
	}

	if (number == 1) xml_set_default_contact(user, doc);
        else xml_set_contact(user, doc);

        g_strfreev(userhost);
        g_free(nameuser);
	gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
    }
    g_free(user);

    xml_savedoc(doc, filename, &tmp_error);
    if (tmp_error != NULL)
    {
	GtkWidget *dialog;
	gint result;


	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK_CANCEL, "Imposible guardar en el fichero '%s': %s\n\n Continuar ?", filename, tmp_error->message);
	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	g_error_free(tmp_error);
        tmp_error = NULL;
        xml_freedoc(doc);
	if (result != GTK_RESPONSE_OK) return TRUE;
        else return FALSE;
    }
    xml_freedoc(doc);
    modified = FALSE;
    return TRUE;
    update_statusbar("Agenda guardada", statusbar);
}



static void button_open ()
{
    GtkWidget *file_sel;
    gint response;
    const gchar *f;


    file_sel = gtk_file_selection_new("Seleccione el fichero con la imagen del usuario");
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_sel));
    gtk_window_set_modal(GTK_WINDOW(file_sel), TRUE);
    response = gtk_dialog_run(GTK_DIALOG(file_sel));

    if (response == GTK_RESPONSE_OK)
    {
	if (filename != NULL) g_free(filename);
	f = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_sel));
	gtk_widget_destroy(file_sel);
        filename = g_strdup(f);
        create_items(filename);
    }
    else gtk_widget_destroy(file_sel);
}



static void button_new ()
{
    GtkWidget *dialog;
    gint result;


    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, "Atencion!, va a crear una agenda nueva. Continuar?");
    gtk_widget_show_all(dialog);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (result != GTK_RESPONSE_OK) return;

    if (modified)
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, "Los datos han sido modificados, Continuar sin guardar ?");
	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if (result != GTK_RESPONSE_OK) return;
    }

    if (filename != NULL) g_free(filename);
    filename = NULL;
    new_items();
}



static void button_save_as ()
{
    GtkWidget *file_sel;
    gint response;
    const gchar *f;


    file_sel = gtk_file_selection_new("Seleccione el fichero con la imagen del usuario");
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_sel));
    gtk_window_set_modal(GTK_WINDOW(file_sel), TRUE);
    response = gtk_dialog_run(GTK_DIALOG(file_sel));

    if (response == GTK_RESPONSE_OK)
    {
	if (filename != NULL) g_free(filename);
	f = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_sel));
	gtk_widget_destroy(file_sel);
        filename = g_strdup(f);
	button_save();
	update_statusbar("Fichero guardado correctamente", statusbar);
    }
    else gtk_widget_destroy(file_sel);
}



static void button_save ()
{
    if (filename != NULL) save_items(filename);
    else button_save_as();
}



static void new_items ()
{
    GtkTreeIter iter;
    gchar *aux;


    gtk_list_store_clear(GTK_LIST_STORE(model));
    user_counter = 0;

    if (user_default == NULL)
    {
	user_default = (ItemUser *) g_malloc0(sizeof(ItemUser));
        user_default->name = g_strdup(AB_DEFAULT_NAMEUSER);
	user_default->user = g_strdup(AB_DEFAULT_USERNAME);
	user_default->host = g_strdup(AB_DEFAULT_HOST);
	user_default->description = g_strdup(AB_DEFAULT_USERDESCRIPTION);
	user_default->ignore = TRUE;
	user_default->photo = NULL;
    }
    user_counter++;
    aux = g_strdup_printf("%s@%s", user_default->user, user_default->host);
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       ADB_COLUMN_NUMBER, user_counter,
		       ADB_COLUMN_NAME, user_default->name,
		       ADB_COLUMN_USER, aux,
		       ADB_COLUMN_CHECK, user_default->ignore,
		       ADB_COLUMN_DESCRIPTION, user_default->description,
		       ADB_COLUMN_PHOTO, user_default->photo,
		       ADB_COLUMN_EDITABLE, FALSE,
		       -1);
    modified = TRUE;
    g_free(aux);
    update_statusbar("Agenda nueva creada", statusbar);
}



static gboolean create_items (gchar *filename)
{
    xmlDocPtr doc;
    ItemUser *user;
    gchar *aux;
    GError *tmp_error = NULL;
    GtkTreeIter iter;
    GtkWidget *dialog;
    GSList *array = NULL;
    gint counter;
    gint len;


    doc = xml_opendoc(filename, &tmp_error);
    if (tmp_error != NULL)
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error al cargar la agenda de contactos xml %s", tmp_error->message);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return FALSE;
    }

    gtk_list_store_clear(GTK_LIST_STORE(model));
    if (user_default != NULL)
    {
        g_free(user_default->name);
        g_free(user_default->user);
        g_free(user_default->host);
        g_free(user_default->photo);
        g_free(user_default->description);
	g_free(user_default);
    }
    user_counter = 0;

    user_default = xml_get_default_contact(doc, NULL);
    if (user_default == NULL)
    {
	user_default = (ItemUser *) g_malloc0(sizeof(ItemUser));
        user_default->name = g_strdup(AB_DEFAULT_NAMEUSER);
	user_default->user = g_strdup(AB_DEFAULT_USERNAME);
	user_default->host = g_strdup(AB_DEFAULT_HOST);
	user_default->description = g_strdup(AB_DEFAULT_USERDESCRIPTION);
	user_default->ignore = TRUE;
	user_default->photo = NULL;
    }

    user_counter++;
    aux = g_strdup_printf("%s@%s", user_default->user, user_default->host);
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       ADB_COLUMN_NUMBER, user_counter,
		       ADB_COLUMN_NAME, user_default->name,
		       ADB_COLUMN_USER, aux,
		       ADB_COLUMN_CHECK, user_default->ignore,
		       ADB_COLUMN_DESCRIPTION, user_default->description,
		       ADB_COLUMN_PHOTO, user_default->photo,
		       ADB_COLUMN_EDITABLE, FALSE,
		       -1);
    g_free(aux);
    array = xml_get_contact(doc, NULL);
    len = g_slist_length(array);
    if (array != NULL)
    {
	counter = 0;
	while (counter < len)
	{
            user = g_slist_nth_data(array, counter);
	    if (user == NULL) break;

            aux = g_strdup_printf("%s@%s", user->user, user->host);
	    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			       ADB_COLUMN_NUMBER, ++user_counter,
			       ADB_COLUMN_NAME, user->name,
			       ADB_COLUMN_USER, aux,
			       ADB_COLUMN_CHECK, user->ignore,
			       ADB_COLUMN_DESCRIPTION, user->description,
			       ADB_COLUMN_PHOTO, user->photo,
			       ADB_COLUMN_EDITABLE, TRUE,
			       -1);
	    g_free(aux);
	    g_free(user->name);
            g_free(user->photo);
            g_free(user->description);
	    g_free(user->user);
	    g_free(user->host);
	    g_free(user);
            counter++;
	}
	g_slist_free(array);
    }
    xml_freedoc(doc);
    modified = FALSE;
    update_statusbar("Agenda leida correctamente", statusbar);
    return TRUE;
}



static void cell_edited_name (GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, gpointer data)
{
    if ((strlen(new_text) > 0))
    {
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *old_text;
	GtkWidget *dialog;
        gint result;


	UNUSED(cell);
        UNUSED(data);
	path = gtk_tree_path_new_from_string(path_string);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
    	gtk_tree_path_free(path);

	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, ADB_COLUMN_NAME, &old_text, -1);
	if (strcmp(old_text, user_default->name) != 0)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, "Seguro que desea cambiar el valor ?");
	    gtk_widget_show_all(dialog);
	    result = gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
	    if (result != GTK_RESPONSE_OK)
	    {
		g_free(old_text);
		return;
	    }
	}
	g_free(old_text);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, ADB_COLUMN_NAME, new_text, -1);
	update_statusbar("Nombre actualizado", statusbar);
    }
    else update_statusbar("Nombre no valido ...", statusbar);
}



static void cell_edited_user (GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, gpointer data)
{
    if ((strlen(new_text) > 3) && (strchr(new_text,'@') != NULL))
    {
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *old_text;
        gchar *aux;
	GtkWidget *dialog;
        gint result;


	UNUSED(cell);
        UNUSED(data);
	path = gtk_tree_path_new_from_string(path_string);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
	gtk_tree_path_free(path);

	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, ADB_COLUMN_USER, &old_text, -1);
        aux = g_strdup_printf("%s@%s", user_default->user, user_default->host);
	if (strcmp(old_text, aux) != 0)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, "Seguro que desea cambiar el valor ?");
	    gtk_widget_show_all(dialog);
	    result = gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
	    if (result != GTK_RESPONSE_OK)
	    {
		g_free(old_text);
                g_free(aux);
		return;
	    }
	}
        modified = TRUE;
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, ADB_COLUMN_USER, new_text, -1);
	g_free(old_text);
	g_free(aux);
	update_statusbar("Campo actualizado", statusbar);
    }
    else update_statusbar("Formato incorrecto, el formato es 'usuario@hostname'", statusbar);
}



static void cell_edited_check (GtkCellRendererText *cell, const gchar *path_string, gpointer data)
{
    GtkTreePath *path;
    GtkTreeIter iter;
    gboolean check;


    UNUSED(cell);
    UNUSED(data);
    modified = TRUE;
    path = gtk_tree_path_new_from_string(path_string);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);

    gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, ADB_COLUMN_CHECK, &check, -1);
    check ^= 1;
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, ADB_COLUMN_CHECK, check, -1);
    gtk_tree_path_free(path);
}



static void cell_edited_description (GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, GtkTreeView *treeview)
{
    GtkTreeIter iter;
    gchar *old_text;
    GtkTreeSelection *selection;
    GtkWidget *dialog;
    gint result;
    GtkTreeModel *model;


    UNUSED(cell);
    UNUSED(path_string);
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    selection = gtk_tree_view_get_selection(treeview);
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, ADB_COLUMN_DESCRIPTION, &old_text, -1);
	if (strcmp(old_text, user_default->description) != 0)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, "Seguro que desea cambiar la descripcion ?");
	    gtk_widget_show_all(dialog);
	    result = gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
	    if (result != GTK_RESPONSE_OK)
	    {
		g_free(old_text);
		return;
	    }
	}
	g_free (old_text);
        modified = TRUE;
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, ADB_COLUMN_DESCRIPTION, new_text, -1);
	update_statusbar("Descripcion actualizada", statusbar);
    }
    else update_statusbar("Descripcion NO actualizada", statusbar);
}



static void add_item_user (GtkWidget *button, gpointer data)
{
    GtkTreeIter iter;
    gchar *aux;


    UNUSED(data);
    UNUSED(button);
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    aux = g_strdup_printf("%s@%s", user_default->user, user_default->host);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       ADB_COLUMN_NUMBER, ++user_counter,
		       ADB_COLUMN_NAME, user_default->name,
		       ADB_COLUMN_USER, aux,
		       ADB_COLUMN_CHECK, FALSE,
		       ADB_COLUMN_DESCRIPTION, user_default->description,
		       ADB_COLUMN_PHOTO, user_default->photo,
		       ADB_COLUMN_EDITABLE, TRUE,
		       -1);
    g_free(aux);
    modified = TRUE;
    update_statusbar("Edite los campos del usario para adecuar la informacion, haciendo click en cada campo", statusbar);
}



static gboolean remove_item_user (GtkWidget *widget, GtkTreeView *treeview)
{
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GtkWidget *dialog;
    gint result;
    GtkTreeModel *model;
    gint number;


    UNUSED(widget);
    selection = gtk_tree_view_get_selection(treeview);
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, ADB_COLUMN_NUMBER, &number, -1);
	if (number == 1)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "El usuario seleccionado es el usuario 'por defecto', es decir, este usuario representa a todos los que no estan en la agenda, y define el comportamiento del programa con ellos. No se puede eliminar");
	    gtk_widget_show_all(dialog);
	    gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
	    return TRUE;
	}
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, "Realmente desea eliminar el usuario seleccionado ?");
	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if (result != GTK_RESPONSE_OK) return TRUE;
	gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

	/*
	gtk_tree_view_column_clicked(ADB_COLUMN_NUMBER);
	gtk_tree_model_get_iter_first(model, &iter);
	for (i=1 ; i<user_counter; i++)
	{
	    gtk_list_store_set(GTK_LIST_STORE(model), &iter, ADB_COLUMN_NUMBER, i, -1);
            gtk_tree_model_iter_next(model, &iter);
	}
	*/

        modified = TRUE;
	update_statusbar("Usuario borrado de la agenda, que pena ...", statusbar);
        user_counter--;
        return TRUE;
    }
    update_statusbar("Imposible borrar, no hay ningun usuario seleccionado", statusbar);
    return FALSE;
}



static gboolean cb_function ()
{
    GtkTreeSelection *selection;
    ItemUser *user;
    GtkTreeIter iter;
    GtkWidget *dialog;
    gchar *nameuser;
    gchar **userhost;
    gint number;


    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    if (gtk_tree_selection_get_selected(selection, (GtkTreeModel **) &model, &iter))
    {
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, ADB_COLUMN_NUMBER, &number, -1);
	if (number == 1)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "El usuario por defecto no es un usuario valido, Por favor, seleccione otro usuario");
	    gtk_widget_show_all(dialog);
	    gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
            return TRUE;
	}
	else
	{
	    gchar *photo;
	    gchar *description;
	    gint ignore;
	    gchar *name;


	    user = (ItemUser *) g_malloc0(sizeof(ItemUser));
	    gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
			       ADB_COLUMN_NUMBER, &number,
			       ADB_COLUMN_NAME, &name,
			       ADB_COLUMN_USER, &nameuser,
			       ADB_COLUMN_CHECK, &ignore,
			       ADB_COLUMN_DESCRIPTION, &description,
			       ADB_COLUMN_PHOTO, &photo,
			       -1);
	    userhost = g_strsplit(nameuser, "@", 2);
	    if (userhost != NULL)
	    {
		user->user = g_strdup(userhost[0]);
		user->host = g_strdup(userhost[1]);
	    }
	    g_strfreev(userhost);
            g_free(nameuser);

            user->name = name;
	    user->photo = photo;
	    user->description = description;
            user->ignore = ignore;
	    callback(user);
	    destroy_addressbook_window();
	    return FALSE;
	}
    }
    else
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "No hay ningun item seleccionado");
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
        return TRUE;
    }
}



static void add_columns (GtkTreeView *treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;


    /*
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Numero", renderer,
						      "text", ADB_COLUMN_NUMBER,
						      NULL);
    gtk_tree_view_column_set_sort_column_id(column, ADB_COLUMN_NUMBER);
    gtk_tree_view_append_column(treeview, column);
    gtk_tree_view_column_set_visible(column, FALSE);
    */

    renderer = gtk_cell_renderer_text_new();
    g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(cell_edited_name), treeview);
    column = gtk_tree_view_column_new_with_attributes("Nombre", renderer,
						      "text", ADB_COLUMN_NAME,
						      "editable", ADB_COLUMN_EDITABLE,
						      NULL);
    gtk_tree_view_column_set_sort_column_id(column, ADB_COLUMN_NAME);
    gtk_tree_view_column_set_min_width(column, 150);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);

    renderer = gtk_cell_renderer_text_new();
    g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(cell_edited_user), treeview);
    column = gtk_tree_view_column_new_with_attributes("Usuario", renderer,
						      "text", ADB_COLUMN_USER,
						      "editable", ADB_COLUMN_EDITABLE,
						      NULL);
    gtk_tree_view_column_set_sort_column_id(column, ADB_COLUMN_USER);
    gtk_tree_view_column_set_min_width(column, 200);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);

    renderer = gtk_cell_renderer_text_new();
    g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(cell_edited_description), treeview);
    column = gtk_tree_view_column_new_with_attributes("Descripcion", renderer,
						      "text", ADB_COLUMN_DESCRIPTION,
						      "editable", ADB_COLUMN_EDITABLE,
						      NULL);
    gtk_tree_view_column_set_sort_column_id(column, ADB_COLUMN_DESCRIPTION);
    gtk_tree_view_column_set_min_width(column, 250);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(cell_edited_check), treeview);
    column = gtk_tree_view_column_new_with_attributes("Ignorar", renderer,
						      "active", ADB_COLUMN_CHECK,
						      NULL);
    gtk_tree_view_column_set_sizing(GTK_TREE_VIEW_COLUMN(column), GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(GTK_TREE_VIEW_COLUMN(column), 50);
    gtk_tree_view_append_column(treeview, column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Fotografia", renderer,
						      "text", ADB_COLUMN_PHOTO,
						      NULL);
    gtk_tree_view_column_set_sort_column_id(column, ADB_COLUMN_PHOTO);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);

    return;
}



static void selection_item_user_changed (GtkTreeSelection *selection, GtkTreeView  *treeview)
{
    GtkTreeIter iter;
    Item_user_display *display;
    GtkTreeModel *m;


    m = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    display = g_object_get_data(G_OBJECT(treeview), "update-display");
    if (gtk_tree_selection_get_selected(selection, &m, &iter))
    {
	gchar *name;
	gchar *path;
	GError *error = NULL;

	gtk_tree_model_get(GTK_TREE_MODEL(m), &iter, ADB_COLUMN_NAME, &name, -1);
	gtk_label_set_text(GTK_LABEL(display->label_name), name);

	gtk_tree_model_get(GTK_TREE_MODEL(m), &iter, ADB_COLUMN_PHOTO, &path, -1);

	gtk_container_remove(GTK_CONTAINER(display->button_user_image), display->image_user);

	if (path != NULL)
	{
	    display->image_user = load_scaled_image_from_file(path, AB_DEFAULT_PHOTO_SIZEX, AB_DEFAULT_PHOTO_SIZEY, GDK_INTERP_BILINEAR, &error);
	    if ((error != NULL) || (display->image_user == NULL))
	    {
		if (error != NULL)
		{
		    update_statusbar(error->message, statusbar);
		    g_error_free(error);
                    error = NULL;
		}
		display->image_user = load_scaled_image_from_file(user_default->photo, AB_DEFAULT_PHOTO_SIZEX, AB_DEFAULT_PHOTO_SIZEY, GDK_INTERP_BILINEAR, &error);
		if ((error != NULL) || (display->image_user == NULL))
		{
		    if (error != NULL)
		    {
			update_statusbar(error->message, statusbar);
			g_error_free(error);
                        error = NULL;
		    }
		    display->image_user = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
		}
                else display->image_user = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
	    }
	}
	else display->image_user = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);

	g_free(name);
        g_free(path);

	gtk_container_add(GTK_CONTAINER(display->button_user_image), display->image_user);
	gtk_widget_show(display->image_user);
	update_statusbar("Usuario seleccionado", statusbar);
    }
}



static void create_file_selection (GtkWidget *widget, GtkTreeView *treeview)
{
    GtkWidget *file_sel;
    gint response;
    gchar *file_name;
    const gchar *f;
    GtkTreeSelection *selection;
    GtkTreeIter iter;


    UNUSED(widget);
    selection = gtk_tree_view_get_selection(treeview);
    if (gtk_tree_selection_get_selected(selection, (GtkTreeModel **) &model, &iter))
    {
	file_sel = gtk_file_selection_new("Seleccione el fichero con la imagen del usuario");
	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_sel));
        gtk_widget_show_all(file_sel);
	//gtk_window_set_modal(GTK_WINDOW(file_sel), TRUE);
	response = gtk_dialog_run(GTK_DIALOG(file_sel));

	if (response == GTK_RESPONSE_OK)
	{
	    gchar *c;


	    f = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_sel));
	    c = strrchr(f,'/');
	    if (c != NULL)
	    {
		if (strlen(c) < 1) f = NULL;
	    }
            else f = NULL;
	}
	else f = NULL;
	gtk_widget_destroy (file_sel);
        file_name = g_strdup(f);

	if (file_name != NULL)
	{
	    Item_user_display *display;
	    gchar *path;
            gint number;


	    display = g_object_get_data(G_OBJECT(treeview), "update-display");
	    gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, ADB_COLUMN_NUMBER, &number, ADB_COLUMN_PHOTO, &path, -1);
            if (path != NULL) g_free(path);

	    if (number == 1) user_default->photo = g_strdup(file_name);

	    gtk_list_store_set(GTK_LIST_STORE(model), &iter, ADB_COLUMN_PHOTO, file_name, -1);
	    selection_item_user_changed(selection, treeview);
            g_free(file_name);
	    modified = TRUE;
	}
    }
}



void create_addressbook_window (gchar *file_name, CB_gui_addbook cb)
{
    if (!window)
    {
	GtkWidget *table_main;
	GtkWidget *table_db;
	GtkWidget *button_add;
	GtkWidget *button_del;
        GtkWidget *button_sel;
	GtkWidget *sw;
	GtkWidget *vbox_db_sup;
	GtkWidget *vbox_db_inf;
	GtkWidget *frame_image;
	GtkWidget *label_user;
	GtkAccelGroup *accel_group;
	GtkItemFactory *item_factory;
	GtkWidget *button_image;
	GtkWidget *image;
	GtkWidget *hseparator_vbox;
	GtkTreeSelection *select;
	Item_user_display *display;
	GtkTooltips *tips_help;
        GError *error = NULL;


	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Libreta de contactos");
        gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(destroy_addressbook_window), NULL);
	tips_help = gtk_tooltips_new();

	table_main = gtk_table_new(1, 4, FALSE);
	gtk_container_add(GTK_CONTAINER(window), table_main);

	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
	g_object_unref(accel_group);
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

	g_object_ref(item_factory);
	gtk_object_sink(GTK_OBJECT(item_factory));
	g_object_set_data_full(G_OBJECT(window), "<main>", item_factory, (GDestroyNotify) g_object_unref);

	gtk_item_factory_create_items(item_factory, G_N_ELEMENTS (menu_items), menu_items, window);
	gtk_table_attach(GTK_TABLE(table_main), gtk_item_factory_get_widget (item_factory, "<main>"), 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);

	statusbar = gtk_statusbar_new();

	toolbar = gtk_toolbar_new();
	gtk_table_attach(GTK_TABLE(table_main), toolbar, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);

	table_db = gtk_table_new(2, 2, FALSE);
        gtk_table_attach(GTK_TABLE(table_main), table_db, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL,  GTK_EXPAND | GTK_FILL, 0, 0);

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_IN);
	gtk_table_attach(GTK_TABLE(table_db), sw, 0, 1, 0, 2, GTK_EXPAND | GTK_FILL,  GTK_EXPAND | GTK_FILL, 0, 0);


        // valores iniciales

	model = gtk_list_store_new(ADB_NUM_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
	user_default = (ItemUser *) g_malloc0(sizeof(ItemUser));
        user_default->name = g_strdup(AB_DEFAULT_NAMEUSER);
	user_default->user = g_strdup(AB_DEFAULT_USERNAME);
	user_default->host = g_strdup(AB_DEFAULT_HOST);
	user_default->description = g_strdup(AB_DEFAULT_USERDESCRIPTION);
	user_default->ignore = TRUE;
	user_default->photo = NULL;

	if (!create_items(file_name)) new_items();

	modified = FALSE;
        filename = g_strdup(file_name);


	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
        gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview), ADB_COLUMN_NAME);
        add_columns(GTK_TREE_VIEW(treeview));
        gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), TRUE);

	display = g_malloc0(sizeof(Item_user_display));
	g_object_set_data_full(G_OBJECT(treeview), "update-display", display, g_free);

	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK(selection_item_user_changed), treeview);

	gtk_container_add(GTK_CONTAINER(sw), treeview);

	vbox_db_sup = gtk_vbox_new(FALSE, 10);
        gtk_container_set_border_width(GTK_CONTAINER(vbox_db_sup), 10);


	frame_image = gtk_frame_new("Seleccion");
	gtk_frame_set_shadow_type(GTK_FRAME(frame_image), GTK_SHADOW_IN);
        gtk_container_set_border_width(GTK_CONTAINER(frame_image), 10);
	gtk_table_attach(GTK_TABLE(table_db), frame_image, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_container_add(GTK_CONTAINER(frame_image), vbox_db_sup);

	button_image = gtk_button_new();
	g_signal_connect(G_OBJECT(button_image), "clicked", G_CALLBACK (create_file_selection), treeview);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_image, "Pulse sobre la imagen para elegir la foto", NULL);

	if (user_default->photo != NULL)
	{
	    image = load_scaled_image_from_file(user_default->photo, AB_DEFAULT_PHOTO_SIZEX, AB_DEFAULT_PHOTO_SIZEY, GDK_INTERP_BILINEAR, &error);
	    if ((error != NULL) || (image == NULL))
	    {
		if (error != NULL)
		{
		    update_statusbar(error->message, statusbar);
		    g_error_free(error);
                    error = NULL;
		}
		image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
	    }
	}
        else image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);

	gtk_container_add(GTK_CONTAINER(button_image), image);
        gtk_container_set_border_width(GTK_CONTAINER (button_image), 5);
	gtk_box_pack_start(GTK_BOX(vbox_db_sup), button_image, FALSE, FALSE, 0);

	label_user = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(vbox_db_sup), label_user, FALSE, FALSE, 0);

	display->button_user_image = button_image;
	display->image_user = image;
	display->label_name = label_user;

	vbox_db_inf = gtk_vbox_new(FALSE, 20);
        gtk_container_set_border_width(GTK_CONTAINER (vbox_db_inf), 10);
	gtk_table_attach(GTK_TABLE(table_db), vbox_db_inf, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	hseparator_vbox = gtk_hseparator_new();
        gtk_box_pack_start(GTK_BOX(vbox_db_inf), hseparator_vbox, FALSE, FALSE, 0);

	button_add = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_signal_connect(G_OBJECT(button_add), "clicked", G_CALLBACK (add_item_user), NULL);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_add, "Anade un contacto a la agenda para ser editado", NULL);
	gtk_box_pack_start(GTK_BOX(vbox_db_inf), button_add, FALSE, FALSE, 0);

	button_del = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	g_signal_connect(G_OBJECT(button_del), "clicked", G_CALLBACK (remove_item_user), treeview);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_del, "Borra el contacto seleccionado", NULL);
	gtk_box_pack_start(GTK_BOX(vbox_db_inf), button_del, FALSE, FALSE, 0);

	if (cb != NULL)
	{
	    hseparator_vbox = gtk_hseparator_new();
	    gtk_box_pack_start(GTK_BOX(vbox_db_inf), hseparator_vbox, FALSE, FALSE, 0);

	    button_sel = button_image_from_stock("_Seleccionar" ,GTK_STOCK_INDEX, GTK_ICON_SIZE_BUTTON);
	    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_sel, "Acepta el item seleccionado y sale de la agenda", NULL);
	    gtk_box_pack_start(GTK_BOX(vbox_db_inf), button_sel, FALSE, FALSE, 0);
            callback = cb;
	    g_signal_connect(G_OBJECT(button_sel), "clicked", G_CALLBACK(cb_function), NULL);
	}

	statusbar = gtk_statusbar_new();
	gtk_table_attach(GTK_TABLE(table_main), statusbar, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 0);


	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_NEW,
				  "Crear una agenda de contactos nueva",
				  NULL, G_CALLBACK(button_new), NULL, -1);


	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_OPEN,
				  "Abrir un fichero agenda xml existente",
				  NULL, G_CALLBACK(button_open), NULL, -1);
	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_SAVE,
				  "Guardar los cambios realizados",
				  NULL, G_CALLBACK(button_save), NULL, -1);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_FIND,
				  "Buscar contenidos en la agenda",
				  NULL, G_CALLBACK(dialog_search), NULL, -1);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_QUIT,
				  "Salir de la agenda de contactos",
				  NULL, G_CALLBACK(destroy_addressbook_window), NULL, -1);

	gtk_window_set_default_size(GTK_WINDOW(window), 769, -1);

	update_statusbar("Contactos cargados correctamente, todo listo ...", statusbar);
    }

    if (!GTK_WIDGET_VISIBLE (window)) gtk_widget_show_all (window);
    else
    {
	destroy_addressbook_window();
	window = NULL;
    }
    return;
}



