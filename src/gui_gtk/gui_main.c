
#include "gui_main.h"
#include "gui_ssip_neg.h"



gchar *plugins_inout_dir;
gchar *plugins_codecs_dir;
gchar *plugins_effect_dir;


GModule *module_io_plugin;
Plugin_InOut *io_plugin;
gint num_users_active_dialogs = 0;
GSList *list_session_users = NULL;


AudioProperties *audio_properties = NULL;
gint audio_blocksize = 1204;
gint *audio_cng;
ACaps *audio_caps = NULL;
VAD_State *audio_vad = NULL;


GtkWidget *toolbar;
GtkWidget *window;
GtkWidget *notebook_users;
GtkWidget *statusbar;
GtkWidget *vbox_volume;
gboolean vbox_vol_visible = TRUE;
gboolean audio_mute_vol = FALSE;
gboolean audio_mute_mic = FALSE;


GtkListStore *store_inout_plugin;
GtkListStore *store_codec_plugin;
GtkListStore *store_effect_plugin;


GSList *list_codec_plugins = NULL;
Node_listvocodecs *default_sel_codec;

GMutex *list_codec_mutex;
GMutex *audio_processor_mutex;
GMutex *mutex_list_session_users;


ConfigFile *configuration;
gchar *configuration_filename = NULL;


RTP_init_session_param session_ready;
GMutex *mutex_session_ready;


NotifyCall notify_call;
GMutex *mutex_notify_call;


gint rtp_base_port;
gboolean go_ssip_process = FALSE;
GThread *thread_ssip_loop = NULL;


gboolean control_audio_processor;
gboolean conection_reject;

//void w_create_dialog_about ();

GtkItemFactoryEntry menu_items[] =
{
    { "/_Archivo",                        NULL,         0,                           0, "<Branch>",    NULL                  },
    { "/_Archivo/tearoff1",	          NULL,	        0,	                     0, "<Tearoff>",   NULL                  },
    { "/Archivo/_Nuevo",                  "<control>O", ssip_connect_to,             0, "<StockItem>", GTK_STOCK_NEW         },
    { "/Archivo/_Agenda",                 "<control>O", open_addressbook,            0, "<StockItem>", GTK_STOCK_OPEN        },
    { "/Archivo/step1",                   NULL,         0,                           0, "<Separator>", NULL,                 },
    { "/Archivo/_Salir",                  "<control>Q", exit_program,                0, "<StockItem>", GTK_STOCK_QUIT        },

    { "/_Preferencias",                   NULL,         0,                           0, "<Branch>",    NULL                  },
    { "/_Preferencias/tearoff2",          NULL,	        0,	                     0, "<Tearoff>",   NULL                  },
    { "/_Preferencias/_Directorios",      "<control>D", menu_item_config_dirs,       0, "<StockItem>", GTK_STOCK_PREFERENCES },
    { "/_Preferencias/_Configuracion",    "<control>C", show_configuration_settings, 0, "<StockItem>", GTK_STOCK_PROPERTIES  },
    { "/_Preferencias/step3",             NULL,         0,                           0, "<Separator>", NULL,                 },
    { "/_Preferencias/_Audio info",       NULL,         show_audio_info,             0, "<StockItem>", GTK_STOCK_DIALOG_INFO },
    { "/_Preferencias/step4",             NULL,         0,                           0, "<Separator>", NULL,                 },
    { "/_Preferencias/_Guardar estado",   NULL,         save_preferences,            0, "<StockItem>", GTK_STOCK_SAVE        },
    { "/_Preferencias/step5",             NULL,         0,                           0, "<Separator>", NULL,                 },

    { "/_Preferencias/_Notificar estado al SSIP-Proxy",         NULL, 0,                  0, "<Branch>",    NULL                                         },
    { "/_Preferencias/_Notificar estado al SSIP-Proxy/online",  NULL, ssip_proxy_toolbar, 1, "<RadioItem>", NULL                                         },
    { "/_Preferencias/_Notificar estado al SSIP-Proxy/offline", NULL, ssip_proxy_toolbar, 2, "/Preferencias/Notificar estado al SSIP-Proxy/online", NULL },

    { "/_Preferencias/step6",             NULL,         0,                           0, "<Separator>", NULL                  },
    { "/_Preferencias/_Toolbar",          NULL,         0,                           0, "<Branch>",    NULL                  },
    { "/_Preferencias/_Toolbar/Normal",   NULL,         menu_item_toolbar,           0, "<RadioItem>", NULL                  },
    { "/_Preferencias/_Toolbar/Minima",   NULL,         menu_item_toolbar,           0, "/Preferencias/Toolbar/Normal", NULL },
    { "/_Preferencias/_Toolbar/Oculta",   NULL,         menu_item_toolbar,           0, "/Preferencias/Toolbar/Normal", NULL },

    /* If you wanted this to be right justified you would use "<LastBranch>", not "<Branch>".
     * Right justified help menu items are generally considered a bad idea now days. */

    { "/A_yuda",                          NULL,         0,                           0, "<LastBranch>", NULL                  },
    { "/A_yuda/_Acerca de ...",           NULL,         0,                           0, "<StockItem>",  GTK_STOCK_HELP        },

//    { "/A_yuda/_Acerca de ...",           NULL,         w_create_dialog_about,       0, "<StockItem>",  GTK_STOCK_HELP        },
};

gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);



//void w_create_dialog_about ()
//{
//    create_dialog_about(window);
//}



void save_preferences ()
{
    cfgf_write_file(configuration, configuration_filename, NULL);
}



void show_audio_info ()
{
    GtkWidget *view;
    GtkWidget *sw;
    GtkWidget *dialog;
    GtkWidget *frame;
    GtkTextBuffer *buffer;
    GtkTextIter iter;
    gchar *aux;


    if (audio_caps == NULL)
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
					"Todavia no ha sido iniciado el plugin InOut de audio, no se pueden indicar las caracteristicas del sistema de audio");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
        return;
    }
    dialog = gtk_dialog_new_with_buttons("Informacion adicional", GTK_WINDOW(window),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_CLOSE, GTK_RESPONSE_NONE,
					 NULL);
    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

    view = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sw), view);
    gtk_container_add(GTK_CONTAINER(frame), sw);

    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
    gtk_text_buffer_create_tag(buffer, "blue_foreground", "foreground", "blue", NULL);
    gtk_text_buffer_create_tag(buffer, "red_foreground", "foreground", "red", NULL);
    gtk_text_buffer_create_tag(buffer, "heading", "weight", PANGO_WEIGHT_BOLD, "size", 15 * PANGO_SCALE, NULL);
    gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, "size", 10 * PANGO_SCALE, NULL);

    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\nAudio capacities (actual InOut plugin) ...\n\n", -1, "heading", NULL);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio device in: ", -1, "blue_foreground", NULL);
    gtk_text_buffer_insert(buffer, &iter, audio_caps->device_in, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio device out: ", -1, "blue_foreground", NULL);
    gtk_text_buffer_insert(buffer, &iter, audio_caps->device_out, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio mixer in: ", -1, "blue_foreground", NULL);
    gtk_text_buffer_insert(buffer, &iter, audio_caps->mixer_in, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio mixer out: ", -1, "blue_foreground", NULL);
    gtk_text_buffer_insert(buffer, &iter, audio_caps->mixer_out, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio rate in (Hz): ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->rate_in);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio rate out (Hz): ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->rate_out);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio blocksize in (bytes): ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->op_blocksize_in);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio blocksize out (bytes): ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->op_blocksize_out);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Number channels in: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->channels_in);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Number channels out: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->channels_out);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Supported audio IN formats\n", -1, "bold", NULL);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Unsigned 8 bits: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->u8_format_in) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Signed 8 bits: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->s8_format_in) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Unsigned 16 bits LE: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->u16le_format_in) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Unsigned 16 bits BE: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->u16be_format_in) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Signed 16 bits LE: ", -1, "red_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->s16le_format_in) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Signed 16 bits BE: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->s16be_format_in) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Supported audio OUT formats\n", -1, "bold", NULL);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Unsigned 8 bits: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->u8_format_out) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Signed 8 bits: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->s8_format_out) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Unsigned 16 bits LE: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->u16le_format_out) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Unsigned 16 bits BE: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->u16be_format_out) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Signed 16 bits LE: ", -1, "red_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->s16le_format_out) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Signed 16 bits BE: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->s16be_format_out) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Blocking read: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->block_read) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Blocking write: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->block_write) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Can fflush audio: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->fflush) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Trigger audio: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->trigger) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Can mmap audio: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->mmap) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Full duplex audio: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (audio_caps->full_duplex) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio IN properties\n", -1, "bold", NULL);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Number of fragments allocated: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->fragstotal_in);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Size of a fragment (bytes): ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->fragsize_in);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Number of full fragments that can be read without blocking: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->fragments_in);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Bytes that can be read without blocking: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->bytes_in);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio volume (0-100) (r | l): ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d | %d", audio_caps->volume_out_r, audio_caps->volume_out_l);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio OUT properties\n", -1, "bold", NULL);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Number of fragments allocated: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->fragstotal_out);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Size of a fragment (bytes): ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->fragsize_out);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Number of full fragments that can be read without blocking: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->fragments_out);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Bytes that can be read without blocking: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", audio_caps->bytes_out);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio volume (0-100) (r | l): ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d | %d", audio_caps->volume_out_r, audio_caps->volume_out_l);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n\n\n", -1);

    gtk_window_resize(GTK_WINDOW(dialog), 500, 300);

    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
    return;
}



void ssip_proxy_toolbar (gpointer data, guint action, GtkWidget *widget)
{
    static guint last_action = 1;
    GtkWidget *dialog;
    gint gv;
    gboolean b;
    gboolean has_ssip;
    gchar *host;
    gint ssipport;


    UNUSED(data);
    UNUSED(widget);
    if (cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PROXY, &gv) && (gv == 1))
    {
	if (action != last_action)
	{
	    if (action == 1)
	    {
		b = TRUE;
		b &= has_ssip = cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCAL_HOST, &host);
		b &= cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCAL_HOST_PORT, &gv);
		b &= cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCALHOST_PORT, &ssipport);
		if (!b)
		{
		    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						    "La notificacion de estado al SSIP-proxy no esta configurada correctamente");
		    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		    gtk_widget_show_all(dialog);
		    if (has_ssip) g_free(host);
		    return;
		}
		b = init_ssip_proxy_params(host, gv, ssipport);
		g_free(host);
	    }
	    else exit_ssip();
	    last_action = action;
	}
    }
    else
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
					"Imposible notificar estado, no hay ningun SSIP-proxy configurado");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
    }
}



void show_error_open_dir (gchar *dir, GError *tmp_error)
{
    GtkWidget *dialog;


    g_printerr("Error accediendo al directorio '%s': '%s'.\n", dir, tmp_error->message);
    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Error accediendo al directorio '%s': '%s'.", dir, tmp_error->message);
    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
    g_free(tmp_error);
}



void show_error_module_noinit (GError *tmp_error)
{
    GtkWidget *dialog;


    g_printerr("Error inicializando el modulo: '%s'.\n", tmp_error->message);
    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error inicializando el modulo: '%s'.\n", tmp_error->message);
    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
    g_free(tmp_error);
}



void show_error_configfile (GError *tmp_error)
{
    GtkWidget *dialog;


    g_printerr("Error accediendo al fichero '%s': %s\n", configuration_filename, tmp_error->message);
    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
				    "Problema accediendo al fichero de configuracion: %s",
				    tmp_error->message);
    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
    g_error_free(tmp_error);
}



void show_error_module_incorrect (gchar *d, const gchar *error)
{
    GtkWidget *dialog;


    g_printerr("Modulo no reconocido '%s': '%s'.\n", d, error);
    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Modulo '%s' incorrecto: '%s'.\n", d, error);
    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
}



void show_error_module_noload (gchar *dir)
{
    GtkWidget *dialog;


    g_printerr("Error cargando el modulo '%s'.\n", dir);
    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error cargando el modulo '%s'", dir);
    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
}



void show_error_module_noconfigure ()
{
    GtkWidget *dialog;


    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "El modulo seleccionado no tiene sistema de configuracion.\n");
    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
}



void show_error_active_dialogs ()
{
    GtkWidget *dialog;


    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "No se puede cambiar el plugin In-Out Audio y/o sus parametros mientras hay conversaciones activas.\n");
    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
}



void config_dirs ()
{
    gchar *value_inout = NULL;
    gchar *value_codecs = NULL;
    gchar *value_effect = NULL;
    gchar *value_contact = NULL;
    GError *tmp_error = NULL;
    gchar *value;
    GtkWidget *dialog;
    GtkTooltips *tips_help;
    GtkWidget *frame;
    GtkWidget *table;
    GtkWidget *entry_inout;
    GtkWidget *entry_codecs;
    GtkWidget *entry_effect;
    GtkWidget *entry_contact;
    GtkWidget *button_inout;
    GtkWidget *button_codecs;
    GtkWidget *button_effect;
    GtkWidget *button_contact;
    GtkWidget *label;
    gint result;


    dialog = gtk_dialog_new_with_buttons("Configuracion", GTK_WINDOW(window),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
					 NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 210);
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
    tips_help = gtk_tooltips_new();

    frame = gtk_frame_new("Directorios de ubicacion");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 5);

    table = gtk_table_new(4, 3, FALSE);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_container_set_border_width(GTK_CONTAINER(table), 10);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);

    label = gtk_label_new("In-Out Audio Plugins");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    label = gtk_label_new("Audio Codecs Plugins");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    label = gtk_label_new("Effect Plugins");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    label = gtk_label_new("Contactos");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    entry_inout = gtk_entry_new();
    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_INOUT, &value_inout)) gtk_entry_set_text(GTK_ENTRY(entry_inout), value_inout);
    gtk_table_attach(GTK_TABLE(table), entry_inout, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_inout, "Directorio en donde se encuentran los plugins 'In-Out Audio', puede seleccionarlo comodamente pulsando el boton", NULL);

    entry_codecs = gtk_entry_new();
    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_CODECS, &value_codecs)) gtk_entry_set_text(GTK_ENTRY(entry_codecs), value_codecs);
    gtk_table_attach(GTK_TABLE(table), entry_codecs, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_codecs, "Directorio en donde se encuentran los plugins 'Codecs', es decir, los encargados de comprimir o descomprimir el audio. Puede seleccionarlo comodamente pulsando el boton", NULL);

    entry_effect = gtk_entry_new();
    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_EFFECT, &value_effect)) gtk_entry_set_text(GTK_ENTRY(entry_effect), value_effect);
    gtk_table_attach(GTK_TABLE(table), entry_effect, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_effect, "Directorio en donde se encuentran los plugins 'Effect', que se aplicaran al audio de entrada y/o salida. Puede seleccionarlo comodamente pulsando el boton", NULL);

    entry_contact = gtk_entry_new();
    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_DB, &value_contact)) gtk_entry_set_text(GTK_ENTRY(entry_contact), value_contact);
    gtk_table_attach(GTK_TABLE(table), entry_contact, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_contact, "Fichero xml que contiene la base de datos de sus contactos", NULL);

    button_inout = gtk_button_new_from_stock("gtk-open");
    g_signal_connect(button_inout, "clicked", G_CALLBACK(entry_set), entry_inout);
    gtk_table_attach(GTK_TABLE(table), button_inout, 2, 3, 0, 1, GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_button_set_relief(GTK_BUTTON(button_inout), GTK_RELIEF_NONE);

    button_codecs = gtk_button_new_from_stock("gtk-open");
    g_signal_connect(button_codecs, "clicked", G_CALLBACK(entry_set), entry_codecs);
    gtk_table_attach(GTK_TABLE(table), button_codecs, 2, 3, 1, 2, GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_button_set_relief(GTK_BUTTON(button_codecs), GTK_RELIEF_NONE);

    button_effect = gtk_button_new_from_stock("gtk-open");
    g_signal_connect(button_effect, "clicked", G_CALLBACK(entry_set), entry_effect);
    gtk_table_attach(GTK_TABLE(table), button_effect, 2, 3, 2, 3, GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_button_set_relief(GTK_BUTTON(button_effect), GTK_RELIEF_NONE);

    button_contact = gtk_button_new_from_stock("gtk-open");
    g_signal_connect(button_contact, "clicked", G_CALLBACK(entry_set), entry_contact);
    gtk_table_attach(GTK_TABLE(table), button_contact, 2, 3, 3, 4, GTK_FILL, GTK_EXPAND, 0, 0);
    gtk_button_set_relief(GTK_BUTTON(button_contact), GTK_RELIEF_NONE);

    gtk_widget_show_all(dialog);
    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_ACCEPT)
    {
	value = gtk_editable_get_chars(GTK_EDITABLE(entry_inout), 0, -1);
	if (value[strlen(value)] == '/') value[strlen(value)] = 0;
	cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_INOUT, value);
	g_free(value);
	value = gtk_editable_get_chars(GTK_EDITABLE(entry_codecs), 0, -1);
	if (value[strlen(value)] == '/') value[strlen(value)] = 0;
	cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_CODECS, value);
	g_free(value);
	value = gtk_editable_get_chars(GTK_EDITABLE(entry_effect), 0, -1);
	if (value[strlen(value)] == '/') value[strlen(value)] = 0;
	cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_EFFECT, value);
	g_free(value);
	value = gtk_editable_get_chars(GTK_EDITABLE(entry_contact), 0, -1);
	cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_DB, value);
	g_free(value);

	gtk_widget_destroy(dialog);

	g_free(value_inout);
	g_free(value_codecs);
	g_free(value_effect);
	g_free(value_contact);

	if (!cfgf_write_file(configuration, configuration_filename, &tmp_error))
	{
	    show_error_configfile(tmp_error);
	}
	else
	{
	    cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_EFFECT, &plugins_effect_dir);
	    cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_CODECS, &plugins_codecs_dir);
	    cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_INOUT, &plugins_inout_dir);
	}
    }
    else gtk_widget_destroy(dialog);
    return;
}



void session_recv_timeout (GtkDialog *dialog, gint result, TSession *tse)
{
    if (result == GTK_RESPONSE_OK)
    {
	destroy_session(tse->id);
    }
    else tse->asession->rtpinfo->show = FALSE;
    gtk_widget_destroy(GTK_WIDGET(dialog));
}



void session_recv_bye (GtkDialog *dialog, gint result, TSession *tse)
{
    if (result == GTK_RESPONSE_OK)
    {
	destroy_session(tse->id);
    }
    else tse->asession->rtpinfo->show = FALSE;
    gtk_widget_destroy(GTK_WIDGET(dialog));
}



gboolean control_sessions ()
{
    NodeVoCodec *nodecodec;
    TSession *tse;
    GSList *list;
    GSList *list_aux;
    GError *tmp_error = NULL;
    gint new_payload;
    gint page;
    gint result;
    gint mute_mic;
    gboolean must_exit = FALSE;
    gint counter = 0;
    GtkWidget *dialog;


    g_mutex_lock(mutex_list_session_users);
    tse = (TSession *) g_slist_nth_data(list_session_users, counter);
    while (tse != NULL)
    {
	g_mutex_lock(tse->asession->rtpinfo->mutex);
	if (tse->asession->rtpinfo->bye)
	{
	    if (!tse->asession->rtpinfo->show)
	    {
		if (tse->asession->rtpinfo->next_notify == 0)
		{
		    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL,
						    "La conversacion con %s ha finalizado, destruir la sesion RTP ?", tse->userhost);
		    g_signal_connect(GTK_OBJECT(dialog), "response", G_CALLBACK(session_recv_bye), tse);
		    gtk_widget_show_all(dialog);
		    tse->asession->rtpinfo->next_notify = 120;
                    tse->asession->rtpinfo->show = TRUE;
		}
		else tse->asession->rtpinfo->next_notify--;
	    }
	}
	g_mutex_unlock(tse->asession->rtpinfo->mutex);

	g_mutex_lock(tse->asession->rtpinfo->mutex);
	if (tse->asession->rtpinfo->timeout)
	{
	    if (!tse->asession->rtpinfo->show)
	    {
		if (tse->asession->rtpinfo->next_notify == 0)
		{
		    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL,
						    "La conversacion con %s se ha interrumpido, destruir la sesion RTP ?", tse->userhost);
		    g_signal_connect(GTK_OBJECT(dialog), "response", G_CALLBACK(session_recv_timeout), tse);
		    gtk_widget_show_all(dialog);
		    tse->asession->rtpinfo->next_notify = 120;
                    tse->asession->rtpinfo->show = TRUE;
		}
		else tse->asession->rtpinfo->next_notify--;
	    }
	}
	g_mutex_unlock(tse->asession->rtpinfo->mutex);

	g_mutex_lock(tse->asession->rtpinfo->mutex);
	if (tse->asession->rtpinfo->change_payload)
	{
            new_payload = tse->asession->rtpinfo->payload;
	    g_mutex_unlock(tse->asession->rtpinfo->mutex);

	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%s ha cambiado el codec de compresion de audio ...", tse->userhost);
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);

	    list_aux = g_slist_find_custom(tse->plugins_codec, &new_payload, (GCompareFunc) compare_payload_node_plugin_vocodec);
	    if ((list_aux) || (new_payload == IO_PCM_PAYLOAD))
	    {
		tse->asession->process = FALSE;
		mute_mic = *(tse->asession->mute_mic);
		*(tse->asession->mute_mic) = FALSE;
		g_thread_join(tse->loop);

		nodecodec = tse->asession->node_codec_rx;

		if ((nodecodec != NULL) && (tse->asession->node_codec_tx != NULL))
		{
		    if (tse->asession->node_codec_rx->pc->payload != tse->asession->node_codec_tx->pc->payload)
		    {
			codec_destroy(nodecodec->codec);
		    }
		    nodecodec->selected_rx = FALSE;
		}
		else
		{
		    if (nodecodec != NULL)
		    {
			codec_destroy(nodecodec->codec);
			nodecodec->selected_rx = FALSE;
		    }
		}
                must_exit = FALSE;
		if (new_payload != IO_PCM_PAYLOAD)
		{
		    nodecodec = (NodeVoCodec *) g_slist_nth_data(list_aux, 0);
		    nodecodec->selected_rx = TRUE;

		    codec_init(nodecodec->codec, tse->asession->ap_dsp, tse->cfg, CODEC_DECODE, &tmp_error);
		    if (tmp_error != NULL)
		    {
			show_error_module_noinit(tmp_error);
			page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_users));
			gtk_notebook_remove_page(GTK_NOTEBOOK(notebook_users), page);
			gtk_widget_queue_draw(GTK_WIDGET(notebook_users));
                        gtk_window_resize(GTK_WINDOW(window), 1, 1);

			must_exit = TRUE;
			tse->asession->node_codec_tx = NULL;
			tse->asession->process = TRUE;
                        break;
		    }
		}
		else nodecodec = NULL;

		if (!must_exit)
		{
		    tse->asession->node_codec_rx = nodecodec;
		    tse->asession->process = TRUE;

		    tse->loop = g_thread_create((GThreadFunc) session_io_loop, tse->asession, TRUE, &tmp_error);
		    if (tmp_error != NULL)
		    {
			dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
							"ERROR GRAVE cambiando el codec de compresion de salida en la sesion abierta con el usuario %s.\n\nImposible crear sesion thread de E/S de audio: %s.", tse->userhost, tmp_error->message);
			g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
			gtk_widget_show_all(dialog);
			page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_users));
			gtk_notebook_remove_page(GTK_NOTEBOOK(notebook_users), page);
			gtk_widget_queue_draw(GTK_WIDGET(notebook_users));
			gtk_window_resize(GTK_WINDOW(window), 1, 1);

			must_exit = TRUE;
                        break;
		    }
		    else *(tse->asession->mute_mic) = mute_mic;
		}
	    }
	    else
	    {
		if (tse->asession->rtpinfo->next_notify == 0)
		{
		    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						    "Imposible cambiar el codec compresor de audio de entrada en la sesion abierta con %s, no se ha encontrado el codec necesario ...", tse->userhost);
		    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		    gtk_widget_show_all(dialog);
                    tse->asession->rtpinfo->next_notify = 120;
		}
                else tse->asession->rtpinfo->next_notify--;
	    }
	}
        else g_mutex_unlock(tse->asession->rtpinfo->mutex);

	tse = (TSession *) g_slist_nth_data(list_session_users, ++counter);
    }
    g_mutex_unlock(mutex_list_session_users);

    if (must_exit)
    {
	g_mutex_lock(mutex_list_session_users);
	list = g_slist_nth(list_session_users, counter);
	list_session_users = g_slist_remove_link(list_session_users, list);
	g_mutex_unlock(mutex_list_session_users);
	destroy_partial_session(tse);
	g_slist_free_1(list);
	destroy_audio_processor();
    }

    g_mutex_lock(mutex_session_ready);
    if (session_ready.ready)
    {
	if (!control_audio_processor)
	{
	    result = init_audio_processor(TRUE);
	    if (result > 0)
	    {
		tse = create_session(session_ready.user, session_ready.host, session_ready.image_user, session_ready.init_param, session_ready.selected_rx_payload, session_ready.selected_rx_payload, result);
		if ((tse != NULL) && (session_ready.des))
		{
#ifdef RTP_CRYPT_SUPPORT
		    tse->asession->crypt = rtp_set_encryption_key(tse->asession->rtps, session_ready.despass, &tmp_error);
		    if (tmp_error != NULL)
		    {
			dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
							"Imposible iniciar sesion RTP cifrada: %s", tmp_error->message);
			g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
			gtk_widget_show_all(dialog);
			g_clear_error(&tmp_error);
		    }
#endif
		}
		session_ready.ready = FALSE;
		g_mutex_unlock(session_ready.mutex);
		control_audio_processor = FALSE;
		conection_reject = FALSE;

                g_free(session_ready.user);
                g_free(session_ready.host);
		g_free(session_ready.image_user);
                g_free(session_ready.init_param); // los otros campos no se borran, ya que se copian las referencias
                g_free(session_ready.despass);
	    }
	    else control_audio_processor = TRUE;
	}

	if (conection_reject)
	{
	    control_audio_processor = FALSE;
	    session_ready.ready = FALSE;
	    conection_reject = FALSE;

	    g_free(session_ready.user);
	    g_free(session_ready.host);
	    g_free(session_ready.image_user);
	    if (session_ready.init_param->username != NULL) g_free(session_ready.init_param->username);
	    if (session_ready.init_param->toolname != NULL) g_free(session_ready.init_param->toolname);
	    if (session_ready.init_param->usermail != NULL) g_free(session_ready.init_param->usermail);
	    if (session_ready.init_param->userlocate != NULL) g_free(session_ready.init_param->userlocate);
	    if (session_ready.init_param->usernote != NULL) g_free(session_ready.init_param->usernote);
	    g_free(session_ready.init_param->addr);
	    g_free(session_ready.init_param);
	    g_free(session_ready.despass);

	    g_mutex_unlock(session_ready.mutex);
	}
    }
    g_mutex_unlock(mutex_session_ready);

    g_mutex_lock(mutex_notify_call);
    if (notify_call.ready)
    {
	if (!notify_call.process)
	{
	    user_filter();
	    notify_call.process = TRUE;
	}
    }
    g_mutex_unlock(mutex_notify_call);

    return TRUE;
}



void open_addressbook ()
{
    gchar *file_name;
    GtkWidget *dialog;


    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_DB, &file_name))
    {
	create_addressbook_window(file_name, NULL);
	g_free(file_name);
    }
    else
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
					GTK_BUTTONS_CLOSE, "No se ha especificado ningun fichero de agenda de contactos");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
    }
}



void menu_item_config_dirs ()
{
    config_dirs();
    if (num_users_active_dialogs != 0) show_error_active_dialogs();
    else
    {
	set_io_plugin();
	create_models();
    }
}



void menu_item_toolbar (gpointer data, guint action, GtkWidget *widget)
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



void entry_set (GtkButton *button, GtkWidget *entry)
{
    GtkWidget *sel;
    gint result;

    UNUSED(button);
    sel = gtk_file_selection_new("Seleccion");
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(sel));
    result = gtk_dialog_run(GTK_DIALOG(sel));
    if (result == GTK_RESPONSE_OK) gtk_entry_set_text(GTK_ENTRY(entry), gtk_file_selection_get_filename(GTK_FILE_SELECTION(sel)));
    gtk_widget_destroy(GTK_WIDGET(sel));
    return;
}



void toggle_button_cng (GtkToggleButton *togglebutton, gpointer user_data)
{
    UNUSED(togglebutton);
    UNUSED(user_data);
    if (*audio_cng == 0) *audio_cng = 0;
    else *audio_cng = 1;
}



void toggle_button_vad (GtkToggleButton *togglebutton, gpointer user_data)
{
    GtkWidget *dialog;
    guchar *buffer;
    gint result;
    GAsyncQueue *gap;
    ABuffer *in;
    gboolean error;
    gint j;
    gint i;
    gint mute = 0;
    TSession *tse;


    UNUSED(togglebutton);
    UNUSED(user_data);
    if (audio_vad == NULL)
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Ssshh, silencio, no hable, voy a intentar determinar el nivel de ruido en este lugar");
//	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

	buffer = (guchar *) g_malloc0(32000);
	result = init_audio_processor(FALSE);
	if (result > 0)
	{
            error = FALSE;
	    if (audio_processor_loop_create_mic(result * 10 , &gap))
	    {
		if ((in = init_read_audio_buffer(gap, 1024, 128, audio_properties, NULL, NULL)) != NULL)
		{
		    j = 0;
                    i = 0;
		    while (i < 250)
		    {
			read_audio_buffer(in, &mute, (buffer+j));
                        i++;
			j += 128;
		    }
		    audio_vad = audio_vat_create(-1, -1, -1);
		    i = audio_vat_set_cutoff_amplitude(audio_vad, buffer, 32000, 125);


		    printf("La amplitud de corte es %d\n", i);
                    fflush(NULL);


		    free_read_audio_buffer(in);
		}
                else error = TRUE;
		audio_processor_loop_free_mic(result * 10);
	    }
	    else error = TRUE;
	    destroy_audio_processor();

	    if (error)
	    {
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						"Imposible inciar sistema VAD, error creando colas de audio");
		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_show_all(dialog);
	    }
	}
        g_free(buffer);
    }
    else
    {
        i = 0;
	g_mutex_lock(mutex_list_session_users);
	tse = (TSession *) g_slist_nth_data(list_session_users, i);
	while (tse != NULL)
	{
	    *(tse->asession->vad) = FALSE;
	    tse = (TSession *) g_slist_nth_data(list_session_users, ++i);
	}
	g_mutex_unlock(mutex_list_session_users);

	audio_vat_destroy(audio_vad);
        audio_vad = NULL;
    }
}



void toggle_controls_visible (GtkWidget *button, GtkWidget *vbox)
{
    gtk_container_remove(GTK_CONTAINER(toolbar), button);
    if (vbox_vol_visible)
    {
	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_ADD,
				 "Visualizar controles de volumen", NULL,
				 G_CALLBACK(toggle_controls_visible),
				 vbox, -1);
	gtk_widget_hide(vbox_volume);
	vbox_vol_visible = FALSE;
    }
    else
    {
	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_REMOVE,
				 "Ocultar controles de volumen", NULL,
				 G_CALLBACK(toggle_controls_visible),
				 vbox, -1);
	gtk_widget_show(vbox_volume);
	vbox_vol_visible = TRUE;
    }
    gtk_window_resize(GTK_WINDOW(window), 1, 1);
}



void get_adjustment_vol (GtkAdjustment *get, gpointer set)
{
    gint v = 0;


    UNUSED(set);
    if (io_plugin != NULL)
    {
	if (io_plugin->set_out_volume != NULL)
	{
	    v = gtk_adjustment_get_value(get);
	    cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_AUDIO_OUT_VOL, v);
            if (!audio_mute_vol) io_plugin->set_out_volume(v, v);
	}
    }
}



void get_adjustment_mic (GtkAdjustment *get, gpointer set)
{
    gint v = 0;


    UNUSED(set);
    if (io_plugin != NULL)
    {
	if (io_plugin->set_in_volume != NULL)
	{
	    v = gtk_adjustment_get_value(get);
	    cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_AUDIO_IN_GAIM, v);
            if (!audio_mute_mic) io_plugin->set_in_volume(v, v);
	}
    }
}



void set_adjustment_mic (GtkAdjustment *set, gint v)
{
    if (io_plugin != NULL)
    {
	if (io_plugin->set_in_volume != NULL)
	{
	    gtk_adjustment_set_value(GTK_ADJUSTMENT(set), (gdouble) v);
	    io_plugin->set_in_volume(v, v);
	}
    }
}



void set_adjustment_vol (GtkAdjustment *set, gint v)
{
    if (io_plugin != NULL)
    {
	if (io_plugin->set_out_volume != NULL)
	{
	    gtk_adjustment_set_value(GTK_ADJUSTMENT(set), (gdouble) v);
	    io_plugin->set_out_volume(v, v);
	}
    }
}



void toggle_button_mute (GtkToggleButton *check_button, gpointer data)
{
    gint value;


    UNUSED(data);
    if (gtk_toggle_button_get_active(check_button))
    {
        audio_mute_vol = TRUE;
	if (io_plugin->set_out_volume != NULL) io_plugin->set_out_volume(0, 0);
    }
    else
    {
	audio_mute_vol = FALSE;
	cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_AUDIO_OUT_VOL, &value);
	if (io_plugin->set_out_volume != NULL) io_plugin->set_out_volume(value, value);
    }
    return;
}



void toggle_button_mic_mute (GtkToggleButton *check_button, gpointer data)
{
    gint value;


    UNUSED(data);
    if (gtk_toggle_button_get_active(check_button))
    {
        audio_mute_mic = TRUE;
	if (io_plugin->set_in_volume != NULL) io_plugin->set_in_volume(0, 0);
    }
    else
    {
	audio_mute_mic = FALSE;
	cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_AUDIO_IN_GAIM, &value);
	if (io_plugin->set_in_volume != NULL) io_plugin->set_in_volume(value, value);
    }
    return;
}



void exit_program ()
{
    GtkWidget *dialog;


    if (num_users_active_dialogs > 0)
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
					GTK_BUTTONS_CLOSE, "No puede salir ahora del programa, hay conversaciones activas ...");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
    }
    else
    {
	exit_ssip();
        gtk_widget_destroy(window);
    }
}



void gui_main_destroy (GtkWidget *widget, gpointer data)
{
    UNUSED(widget);
    UNUSED(data);


    gtk_main_quit ();
}



gboolean create_main_window ()
{
    GtkAccelGroup *accel_group;
    GtkItemFactory *item_factory;
    GtkWidget *frame_mic;
    GtkWidget *frame_vol;
    GtkWidget *vbox_mic;
    GtkWidget *hbox;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *image_button;
    GtkWidget *vbox_vol;
    GtkObject *adj_vol;
    GtkObject *adj_mic;
    GtkWidget *hscale_mic;
    GtkWidget *hbox_button;
    GtkWidget *button_vat;
    GtkWidget *button_mic_mute;
    GtkWidget *hscale_vol;
    GtkWidget *button_cng;
    GtkWidget *button_vol_mute;
    GtkTooltips *tips_help;
    gint r;
    gint l;
    gint gv;
    GSList *list_aux = NULL;


    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Dialogos");
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 340, -1);
    gtk_container_set_border_width(GTK_CONTAINER(window), 1);
    g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(exit_program), NULL);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gui_main_destroy), NULL);
    tips_help = gtk_tooltips_new();

    table = gtk_table_new(5, 1, FALSE);
    gtk_container_add(GTK_CONTAINER(window), table);

    accel_group = gtk_accel_group_new();
    item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);
    g_object_set_data_full(G_OBJECT(window), "<main>", item_factory, (GDestroyNotify) g_object_unref);
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
    gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);

    gtk_table_attach(GTK_TABLE (table), gtk_item_factory_get_widget(item_factory, "<main>"), 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    vbox_volume = gtk_vbox_new(FALSE, 10);

    toolbar = gtk_toolbar_new();
    gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_NEW,
			     "Crear una nueva conservacion", NULL,
			     G_CALLBACK(ssip_connect_to), NULL,-1);
    gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_OPEN,
			     "Abrir fichero de agenda", NULL,
			     G_CALLBACK(open_addressbook), NULL, -1);
    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

    gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_PROPERTIES,
			     "Configurar los parametros y las preferencias generales del programa", NULL,
			     G_CALLBACK(show_configuration_settings), NULL, -1);
    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

    gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_REMOVE,
			     "Ocultar controles de volumen", NULL,
			     G_CALLBACK(toggle_controls_visible),
			     vbox_volume, -1);

    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_table_attach(GTK_TABLE(table), toolbar, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    notebook_users = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook_users), GTK_POS_TOP);
    gtk_table_attach(GTK_TABLE(table), notebook_users, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    //////////

    gtk_table_attach(GTK_TABLE(table), vbox_volume, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    frame_mic = gtk_frame_new("Volumen entrada (%)");
    gtk_container_set_border_width(GTK_CONTAINER(frame_mic), 5);
    gtk_box_pack_start(GTK_BOX(vbox_volume), frame_mic, FALSE, FALSE, 0);

    vbox_mic = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox_mic), 5);
    gtk_container_add(GTK_CONTAINER(frame_mic), vbox_mic);

    /* value, lower, upper, step_increment, page_increment, page_size       */
    /* Note that the page_size value only makes a difference for            */
    /* scrollbar widgets, and the highest value you'll get is actually      */
    /* (upper - page_size).                                                 */
    adj_mic = gtk_adjustment_new(0.0, 0.0, 101.0, 1.0, 1.0, 1.0);
    g_signal_connect(G_OBJECT(adj_mic), "value_changed", G_CALLBACK(get_adjustment_mic), NULL);

    hscale_mic = gtk_hscale_new(GTK_ADJUSTMENT(adj_mic));
    gtk_range_set_update_policy(GTK_RANGE(hscale_mic), GTK_UPDATE_DELAYED);
    gtk_scale_set_digits(GTK_SCALE(hscale_mic), 0);
    gtk_scale_set_value_pos(GTK_SCALE(hscale_mic), GTK_POS_BOTTOM);
    gtk_scale_set_draw_value(GTK_SCALE(hscale_mic), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox_mic), hscale_mic, TRUE, TRUE, 0);

    hbox_button = gtk_hbox_new(TRUE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(hbox_button), 8);
    gtk_box_pack_start(GTK_BOX(vbox_mic), hbox_button, TRUE, TRUE, 0);

    button_vat = gtk_toggle_button_new();
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new_with_mnemonic("_VAD");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_vat, "Voide Activity Detection. El sistema detecta silencios y no envia paquetes, permitiendo ahorrar ancho de banda. Debe estar activado para poder funcionar en cada sesion.", NULL );
    image_button = gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_BUTTON);
    g_signal_connect(G_OBJECT(button_vat), "toggled", G_CALLBACK(toggle_button_vad), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), image_button, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button_vat), hbox);

    button_mic_mute = gtk_toggle_button_new();
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new_with_mnemonic("_MUTE");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_mic_mute, "Activa el MUTE local para el microfono", NULL);
    image_button = gtk_image_new_from_stock(GTK_STOCK_STOP, GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(hbox), image_button, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button_mic_mute), hbox);
    g_signal_connect(G_OBJECT(button_mic_mute), "clicked", G_CALLBACK(toggle_button_mic_mute), NULL);

    gtk_box_pack_start(GTK_BOX(hbox_button), button_vat, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_button), button_mic_mute, TRUE, TRUE, 0);

    ////////////

    frame_vol = gtk_frame_new("Volumen altavoces (%)");
    gtk_container_set_border_width(GTK_CONTAINER(frame_vol), 5);
    gtk_box_pack_start(GTK_BOX(vbox_volume), frame_vol, TRUE, TRUE, 0);

    vbox_vol = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox_vol), 5);
    gtk_container_add(GTK_CONTAINER(frame_vol), vbox_vol);

    /* value, lower, upper, step_increment, page_increment, page_size        */
    /* Note that the page_size value only makes a difference for             */
    /* scrollbar widgets, and the highest value you'll get is actually       */
    /* (upper - page_size).                                                  */
    adj_vol = gtk_adjustment_new(0.0, 0.0, 101.0, 1.0, 1.0, 1.0);
    g_signal_connect(G_OBJECT(adj_vol), "value_changed", G_CALLBACK(get_adjustment_vol), NULL);

    hscale_vol = gtk_hscale_new(GTK_ADJUSTMENT(adj_vol));
    gtk_range_set_update_policy(GTK_RANGE(hscale_vol), GTK_UPDATE_DELAYED);
    gtk_scale_set_digits(GTK_SCALE(hscale_vol), 0);
    gtk_scale_set_value_pos(GTK_SCALE(hscale_vol), GTK_POS_BOTTOM);
    gtk_scale_set_draw_value(GTK_SCALE(hscale_vol), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox_vol), hscale_vol, TRUE, TRUE, 0);

    hbox_button = gtk_hbox_new(TRUE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(hbox_button), 8);
    gtk_box_pack_start(GTK_BOX(vbox_vol), hbox_button, TRUE, TRUE, 0);

    button_cng = gtk_toggle_button_new();
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new_with_mnemonic("_CNG");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_cng, "Activa el sistema CNG, (Comfort Noise Generation), simula el enlace activo en las conversaciones", NULL );
    image_button = gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_BUTTON);
    g_signal_connect(G_OBJECT(button_cng), "toggled", G_CALLBACK(toggle_button_cng), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), image_button, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button_cng), hbox);

    button_vol_mute = gtk_toggle_button_new();
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new_with_mnemonic("_MUTE");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_vol_mute, "Silenciar todo el audio de los altavoces, MUTE, en una palabra", NULL);
    image_button = gtk_image_new_from_stock(GTK_STOCK_STOP, GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(hbox), image_button, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button_vol_mute), hbox);
    g_signal_connect(G_OBJECT(button_vol_mute), "clicked", G_CALLBACK(toggle_button_mute), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_button), button_cng, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_button), button_vol_mute, TRUE, TRUE, 0);

    statusbar = gtk_statusbar_new();
    gtk_table_attach(GTK_TABLE(table), statusbar, 0, 1, 4, 5, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    gtk_window_present(GTK_WINDOW(window));
    gtk_widget_show_all(window);


    // init

    init_configfile();
    if (cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_AUDIO_IN_GAIM, &gv))
    {
	set_adjustment_mic(GTK_ADJUSTMENT(adj_mic), gv);
    }
    else
    {
	if (io_plugin != NULL)
	{
	    if (io_plugin->get_in_volume != NULL) io_plugin->get_in_volume(&r, &l);
            else r =  0;
	    if (io_plugin->set_in_volume != NULL)
	    {
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj_mic), (gdouble) r);
		io_plugin->set_in_volume(r, r);
	    }
	}
    }

    if (cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_AUDIO_OUT_VOL, &gv))
    {
	set_adjustment_vol(GTK_ADJUSTMENT(adj_vol), gv);
    }
    else
    {
	if (io_plugin != NULL)
	{
	    if (io_plugin->get_out_volume != NULL) io_plugin->get_out_volume(&r, &l);
            else r =  0;
	    if (io_plugin->set_out_volume != NULL)
	    {
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj_vol), (gdouble) r);
		io_plugin->set_out_volume(r, r);
	    }
	}
    }

    if (cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DEFAULT_CODEC, &gv))
    {
    	g_mutex_lock(list_codec_mutex);

	list_aux = g_slist_find_custom(list_codec_plugins, &gv, (GCompareFunc) compare_payload_node_plugin_vocodec);
	if (list_aux)
	{
	    list_codec_plugins = g_slist_remove_link(list_codec_plugins, list_aux);
	    list_codec_plugins = g_slist_concat(list_aux, list_codec_plugins);

	    if (gv == IO_PCM_PAYLOAD) default_sel_codec = NULL;
	    else default_sel_codec = g_slist_nth_data(list_aux, 0);
	}
        g_mutex_unlock(list_codec_mutex);
    }

    update_statusbar(" Todo listo ...", statusbar);
    return TRUE;
}




int main (int argc, char **argv)
{

    gint gv;
    gint i;
    gchar *configuration_dir;


    gtk_set_locale();
    if (gtk_init_check (&argc, &argv) == FALSE)
    {
	fprintf(stderr,"Unable to init GTK+\n");
	return 0;
    }

    if (argc != 1)
    {
	i=1;
	while (i < argc)
	{
	    if (g_strrstr(argv[i], "--help") != NULL)
	    {
		printf("\n\t%s v %s (c) %s, %s %s\n", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_DATE, PROGRAM_AUTHOR, PROGRAM_AMAIL);
		printf("Modo de empleo:\n\n");
		printf("\t%s [<fichero>]\n", PROGRAM_NAME);
		printf("\n\tdonde fichero es el fichero de configuracion del programa,\n");
		printf("\tpor defecto es '%s' en el home del usuario\n", PROGRAM_DEFAULT_CONFFILE);
		printf("\tTodas las opciones de configuracion deben ser indicadas en ese\n");
		printf("\tfichero de configuracion.\n\n");
		exit(-1);
	    }
	    else configuration_filename = g_strdup(argv[i]);
            i++;
	}
    }

    if (configuration_filename == NULL)
    {
        configuration_filename = g_strdup_printf("%s/%s/%s", g_get_home_dir(), PROGRAM_DEFAULT_DIR, PROGRAM_DEFAULT_CONFFILE);
        configuration_dir = g_strdup_printf("%s/%s", g_get_home_dir(), PROGRAM_DEFAULT_DIR);
	if (!g_file_test(configuration_dir, (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
	{
	    gv = mkdir(configuration_dir, 00700);
            g_free(configuration_dir);
	    if (gv == -1)
	    {
		perror("Imposible crear directorio de configuracion en el home del usuario");
                exit(-1);
	    }
	}
    }

    if (!g_module_supported ())
    {
        printf("Modules not supported\n");
	return 0;
    }

    g_thread_init(NULL);
    gdk_threads_init();

    list_codec_mutex = g_mutex_new();
    audio_processor_mutex = g_mutex_new();
    mutex_list_session_users = g_mutex_new();

    mutex_notify_call = g_mutex_new();

    mutex_session_ready = g_mutex_new();
    session_ready.mutex = g_mutex_new();
    session_ready.ready = FALSE;

    audio_properties = (AudioProperties *) g_malloc(sizeof(AudioProperties));
    audio_properties->channels = 1;
    audio_properties->rate = AUDIO_HZ;
    audio_properties->format = AUDIO_FORMAT;

    audio_cng = (gint *) g_malloc(sizeof(gint));
    *audio_cng = 0;

    gdk_threads_enter();
    create_main_window();
    gdk_threads_leave();

    gdk_threads_enter();
    init_ssip();
    gdk_threads_leave();

    gtk_timeout_add(500, control_sessions, NULL);

    gdk_threads_enter();
    gtk_main();
    gdk_threads_leave();

    g_mutex_free(list_codec_mutex);
    g_mutex_free(audio_processor_mutex);
    g_mutex_free(mutex_list_session_users);
    g_mutex_free(mutex_notify_call);
    g_mutex_free(mutex_session_ready);
    g_mutex_free(session_ready.mutex);

    g_free(audio_properties);
    g_free(audio_cng);

    if (audio_caps != NULL)
    {
	g_free(audio_caps->device_in);
	g_free(audio_caps->mixer_in);
	g_free(audio_caps->device_out);
	g_free(audio_caps->mixer_out);
        g_free(audio_caps);
    }
    g_free(configuration_filename);

    return 0;
}




