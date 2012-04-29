
/*
 *
 * FILE: gui_common.c
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

#include "gui_common.h"




GtkWidget *load_scaled_image_from_file (gchar *filename, gint width, gint height, GdkInterpType type, GError **error)
{
    GdkPixbuf *pix_image;
    GdkPixbuf *pix_scale;
    GtkWidget *image;
    GError *tmp_error = NULL;


    pix_image = gdk_pixbuf_new_from_file(filename, &tmp_error);
    if (pix_image == NULL)
    {
	g_propagate_error(error, tmp_error);
	return NULL;
    }
    pix_scale = gdk_pixbuf_scale_simple(pix_image, width, height, type);
    g_object_unref(G_OBJECT(pix_image));
    pix_image = pix_scale;
    image = gtk_image_new_from_pixbuf(pix_image);
    return image;
}



GtkWidget *load_image_from_file (gchar *filename, GError **error)
{
    GdkPixbufAnimation *pixanim;
    GtkWidget *image;
    GError *tmp_error = NULL;


    pixanim = gdk_pixbuf_animation_new_from_file(filename, &tmp_error);
    if (pixanim == NULL)
    {
	g_propagate_error(error, tmp_error);
        return NULL;
    }
    image = gtk_image_new_from_animation(pixanim);
    return image;
}



GtkWidget *button_image_from_stock (const gchar *button_label, const gchar *stock_id, GtkIconSize size)
{
    GtkWidget *button;
    GtkWidget *box;
    GtkWidget *label;
    GtkWidget *image;


    button = gtk_button_new();
    box = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(box), 2);
    image = gtk_image_new_from_stock(stock_id, size);
    label = gtk_label_new(button_label);
    gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), button_label);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 3);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 3);

    gtk_widget_show(image);
    gtk_widget_show(label);
    gtk_widget_show(box);
    gtk_container_add(GTK_CONTAINER(button), box);
    return button;
}



void create_dialog_about (GtkWidget *window)
{
    static GtkWidget *dialog = NULL;
    GtkWidget *table;
    GtkWidget *hseparator;
    GtkWidget *image_info;
    GtkWidget *image_logo;
    GtkWidget *frame_logo;
    GtkWidget *vbox_info;
    GtkWidget *label_info_program;
    GtkWidget *label_info_author;
    GtkWidget *label_info_mail;
    GtkWidget *textview;
    GtkTextBuffer *buffer;
    GtkTextIter start, end;
    PangoFontDescription *font_desc;
    GdkColor color;
    GtkTextTag *tag;
    GtkWidget *sw;
    GError *error = NULL;


    if (dialog != NULL)
    {
	gtk_widget_destroy(dialog);
	return;
    }
    dialog = gtk_dialog_new_with_buttons("Acerca de ...", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					 NULL);
    g_signal_connect(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    g_signal_connect(GTK_OBJECT(dialog), "destroy", G_CALLBACK(gtk_widget_destroyed), &dialog);

    table = gtk_table_new(3, 4, FALSE);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

/*    image_logo = load_scaled_image_from_file("image6s.jpeg", 80, 100, GDK_INTERP_BILINEAR, &error);
    if (error != NULL)
    {
	g_error_free(error);
	image_logo = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
    }
    frame_logo = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame_logo), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(frame_logo), image_logo);

*/   vbox_info = gtk_vbox_new(FALSE, 5);

    label_info_program = gtk_label_new("<span color=\"blue\"><b><big>  </big></b></span>");
    gtk_label_set_use_markup(GTK_LABEL(label_info_program), TRUE);
    label_info_author = gtk_label_new("<span color=\"blue\"><b><big>  </big></b></span>");
    gtk_label_set_use_markup(GTK_LABEL(label_info_author), TRUE);
    label_info_mail = gtk_label_new("<span color=\"blue\"><b><big>  </big></b></span>");
    gtk_label_set_use_markup(GTK_LABEL(label_info_mail), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox_info), label_info_program, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox_info), label_info_author, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox_info), label_info_mail, TRUE, TRUE, 5);


    hseparator = gtk_hseparator_new();

    image_info = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);

    textview = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

    gtk_text_buffer_set_text(buffer,
			     "\n\nThis program is free software; you can redistribute it \n"
			     " and/or modify it under the terms of the GNU General \n"
			     "Public License as published by the Free Software Foundation; \n"
			     "either version 2 of the License, or (at your option) any \n"
			     "later version.\n This program is distributed in the hope \n"
			     "that it will be useful, but WITHOUT ANY WARRANTY; without \n"
			     "even the implied warranty of MERCHANTABILITY or FITNESS FOR \n"
			     "A PARTICULAR PURPOSE.  See the GNU General Public License \n"
			     "for more details.\n You should have received a copy of the \n"
			     "GNU General Public License along with this program; if not, \n"
			     "write to the Free Software Foundation, Inc., 59 Temple Place\n"
			     "- Suite 330, Boston, MA 02111-1307, USA.\n", -1);

    font_desc = pango_font_description_from_string("Serif 15");
    gtk_widget_modify_font(textview, font_desc);
    pango_font_description_free(font_desc);

    /* Change default color throughout the widget */
//    gdk_color_parse("green", &color);
//    gtk_widget_modify_text(textview, GTK_STATE_NORMAL, &color);

    /* Change left margin throughout the widget */
//    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), 30);

    /* Use a tag to change the color for just one part of the widget */
//    tag = gtk_text_buffer_create_tag(buffer, "blue_foreground", "foreground", "blue", NULL);
//    gtk_text_buffer_get_iter_at_offset(buffer, &start, 7);
//    gtk_text_buffer_get_iter_at_offset(buffer, &end, 12);
//    gtk_text_buffer_apply_tag(buffer, tag, &start, &end);

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER (sw), textview);

    gtk_table_attach(GTK_TABLE (table), frame_logo, 0, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 5, 10);
    gtk_table_attach(GTK_TABLE (table), vbox_info, 2, 3, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 5, 10);
    gtk_table_attach(GTK_TABLE (table), hseparator, 0, 3, 1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 5, 0);
    gtk_table_attach(GTK_TABLE (table), image_info, 0, 1, 2, 3, GTK_SHRINK, GTK_SHRINK, 5, 10);
    gtk_table_attach(GTK_TABLE (table), sw, 1, 3, 2, 4, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 10);

    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
}



void update_statusbar (gchar *buffer, GtkWidget *statusbar)
{
    gchar *aux;


    aux = g_strdup_printf("   %s", buffer);
    gtk_statusbar_pop(GTK_STATUSBAR(statusbar), 0);
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, aux);
    g_free(aux);
}


