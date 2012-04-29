
/*
 *
 * FILE: gui_main.h
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

#ifndef _GUI_MAIN_H_
#define _GUI_MAIN_H_


#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <gmodule.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>

#include "../io.h"
#include "../audio.h"
#include "../audio_buffer.h"
#include "../audio_util.h"
#include "../rtp/rtp.h"
#include "../plugin_inout.h"
#include "../plugin_effect.h"
#include "../configfile.h"
#include "../audio_processor.h"
#include "../audio_convert.h"
#include "../audio_util.h"
#include "../codec_plugin/codec.h"
#include "../common.h"
#include "gui_common.h"
#include "../notify.h"




#define PROGRAM_IOP_SELECT_COLOR "green"


#define GUI_IMAGE_USER_SIZEX  110
#define GUI_IMAGE_USER_SIZEY  140




struct _RTP_init_session_param
{
    GMutex *mutex;
    gchar *user;
    gchar *host;
    gchar *image_user;
    InitRTPsession *init_param;
    gint selected_rx_payload;
    gint selected_tx_payload;
    gboolean des;
    gchar *despass;
    gboolean ready;
};

typedef struct _RTP_init_session_param RTP_init_session_param;




struct _TSession
{
    gint id;
    ASession *asession;
    InitRTPsession *irtpsession;
    ConfigFile *cfg;
    GThread *loop;
    GtkListStore *store_effect;
    GtkListStore *store_codec;
    GSList *plugins_codec;
    gchar *userhost;
};

typedef struct _TSession TSession;




enum
{
    EP_COLUMN_ID,
    EP_COLUMN_NAME,
    EP_COLUMN_ACTIVE,
    EP_COLUMN_FILENAME,
    EP_COLUMN_DESCRIPTION,
    EP_NUM_COLUMNS
};



enum
{
    CP_COLUMN_ID,
    CP_COLUMN_NAME,
    CP_COLUMN_PAYLOAD,
    CP_COLUMN_BW,
    CP_COLUMN_FILENAME,
    CP_COLUMN_DESCRIPTION,
    CP_NUM_COLUMNS
};



enum
{
    IOP_COLUMN_ID,
    IOP_COLUMN_NAME,
    IOP_COLUMN_FILENAME,
    IOP_COLUMN_DESCRIPTION,
    IOP_COLUMN_COLOR,
    IOP_NUM_COLUMNS
};




struct _Node_listvocodecs
{
    gchar *filename;
    gint id;
    gchar *name;
    gint16 payload;
    gint16 ms;
    gint16 compresed_frame;
    gint16 uncompresed_frame;
    gint bw;
};

typedef struct _Node_listvocodecs Node_listvocodecs;



struct _NotifyCall
{
    gchar *from_user;
    SsipPkt *pkt_r;
    Socket_tcp *skt;
    gboolean process;
    gboolean ready;
    gboolean accept;
};

typedef struct _NotifyCall NotifyCall;





extern GMutex *mutex_notify_call;
extern NotifyCall notify_call;

extern gchar *plugins_inout_dir;
extern gchar *plugins_codecs_dir;
extern gchar *plugins_effect_dir;
extern gchar *contact_db;

extern GModule *module_io_plugin;
extern Plugin_InOut *io_plugin;
extern GSList *list_session_users;
extern gint num_users_active_dialogs;

extern AudioProperties *audio_properties;
extern gint audio_blocksize;
extern gint *audio_cng;
extern ACaps *audio_caps;
extern VAD_State *audio_vad;

extern GtkWidget *toolbar;
extern GtkWidget *window;
extern GtkWidget *notebook_users;
extern GtkWidget *statusbar;
extern GtkWidget *vbox_volume;
extern gboolean vbox_vol_visible;
extern gboolean audio_mute_vol;
extern gboolean audio_mute_mic;

extern GtkListStore *store_inout_plugin;
extern GtkListStore *store_codec_plugin;
extern GtkListStore *store_effect_plugin;

extern GSList *list_codec_plugins;
extern Node_listvocodecs *default_sel_codec;
extern GMutex *list_codec_mutex;

extern GMutex *audio_processor_mutex;

extern GMutex *mutex_list_session_users;


extern ConfigFile *configuration;
extern gchar *configuration_filename;

extern GtkItemFactoryEntry menu_items[];
extern gint nmenu_items;


extern gint rtp_base_port;
extern gint ssip_port;

extern gboolean go_ssip_process;
extern GThread *thread_ssip_loop;


extern RTP_init_session_param session_ready;
extern GMutex *mutex_session_ready;


extern gboolean control_audio_processor;
extern gboolean conection_reject;




#include "gui_ssip_neg.h"



// gui_main.c



void save_preferences ();
void show_audio_info ();
void ssip_proxy_toolbar (gpointer data, guint action, GtkWidget *widget);
void show_error_open_dir (gchar *dir, GError *tmp_error);
void show_error_module_noinit (GError *tmp_error);
void show_error_configfile (GError *tmp_error);
void show_error_module_incorrect (gchar *d, const gchar *error);
void show_error_module_noload (gchar *dir);
void show_error_module_noconfigure ();
void show_error_active_dialogs ();
void config_dirs ();
void session_recv_timeout (GtkDialog *dialog, gint result, TSession *tse);
void session_recv_bye (GtkDialog *dialog, gint result, TSession *tse);
gboolean control_sessions ();
void open_addressbook ();
void menu_item_config_dirs ();
void menu_item_toolbar (gpointer data, guint action, GtkWidget *widget);

void entry_set (GtkButton *button, GtkWidget *entry);
void toggle_button_cng (GtkToggleButton *togglebutton, gpointer user_data);
void toggle_button_vad (GtkToggleButton *togglebutton, gpointer user_data);
void toggle_controls_visible (GtkWidget *button, GtkWidget *vbox);
void get_adjustment_vol (GtkAdjustment *get, gpointer set);
void get_adjustment_mic (GtkAdjustment *get, gpointer set);
void set_adjustment_mic (GtkAdjustment *set, gint v);
void set_adjustment_vol (GtkAdjustment *set, gint v);
void toggle_button_mute (GtkToggleButton *check_button, gpointer data);
void toggle_button_mic_mute (GtkToggleButton *check_button, gpointer data);
void exit_program ();
void gui_main_destroy (GtkWidget *widget, gpointer data);
gboolean create_main_window ();













void menu_item_config_dirs (void);
void menu_item_toolbar (gpointer data, guint action, GtkWidget *widget);
void entry_set (GtkButton *button, GtkWidget *entry);
void configure_about_effect (GtkButton *button, GtkTreeView *treeview);
void configure_about_codec (GtkButton *button, GtkTreeView *treeview);
void about_configure_select_inout (GtkButton *button, GtkTreeView *treeview);
void toggled_efplugin (GtkCellRendererToggle *cell, gchar *path_str, GtkTreeView *treeview);
void select_audio_blocksize (GtkButton *button, GtkWidget *entry);
void response_configuration_settings (GtkWidget *dialog, gint arg1, gpointer user_data);
void toggle_controls_visible (GtkWidget *button, GtkWidget *vbox);
void get_adjustment_vol (GtkAdjustment *get, gpointer set);
void get_adjustment_mic (GtkAdjustment *get, gpointer set);
void set_adjustment_mic (GtkAdjustment *set, gint v);
void set_adjustment_vol (GtkAdjustment *set, gint v);
void toggle_button_mute (GtkToggleButton *check_button, gpointer data);
void toggle_button_mic_mute (GtkToggleButton *check_button, gpointer data);

// user

void toggle_button_user_mute (GtkToggleButton *check_button, gpointer data);
void toggle_button_user_mic_mute (GtkToggleButton *check_button, gpointer data);
void toggle_button_user_vad (GtkToggleButton *check_button, gpointer data);
void change_user_vol (GtkAdjustment *get, gpointer data);




// gui_main_functions.c

void init_configfile (void);
gboolean unset_io_plugin (void);
gboolean set_io_plugin (void);
inline gint square_2 (gint p);
gint init_audio_processor (gboolean with_control);
gboolean destroy_audio_processor (void);
gboolean create_model_inout (void);
gboolean create_models (void);
void destroy_models (void);



gint compare_payload_node_plugin_vocodec (gconstpointer a, gconstpointer b);




// gui_main.c


//void show_configuration_settings (void);
//void menu_item_config_dirs (void);
//void menu_item_toolbar (void);


void save_preferences ();
void show_audio_info ();




void gtk_ifactory_cb (gpointer callback_data, guint callback_action, GtkWidget *widget);
gboolean set_io_plugin (void);


void show_error_open_dir (gchar *dir, GError *tmp_error);
void show_error_module_noinit (GError *tmp_error);
void show_error_configfile (GError *tmp_error);
void show_error_module_incorrect (gchar *d, const gchar *error);
void show_error_module_noload (gchar *dir);
void show_error_module_noconfigure (void);
void show_error_active_dialogs (void);
void add_effect_plugins_columns (GtkWidget *treeview, GtkTreeModel *model, gpointer func);
void config_dirs (void);

/*
void show_all_plugins ()
*/

void show_configuration_settings (void);
gint compare_node_plugin_effect (gconstpointer a, gconstpointer b);
void clear_list_plugins_effect (GSList *list_plugins_effect);
void clear_list_plugins_codec (GSList *list_plugins_codec);

/*
void toggled_cb_efplugin (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
*/

gboolean create_main_window (void);
gboolean configure_preferences (void);



void destroy_user_session (GtkWidget *widget, TSession *tse);

gboolean destroy_session (gint id);
void destroy_partial_session (TSession *tse);
TSession *create_session (gchar *user, gchar *host, gchar *image_user, InitRTPsession *init_param, gint selected_rx_codec_payload, gint selected_tx_codec_payload, gint id);




void add_effect_plugins_columns (GtkWidget *treeview, GtkTreeModel *model, gpointer func);

gint compare_node_plugin_effect (gconstpointer a, gconstpointer b);

gint compare_payload_node_plugin_vocodec (gconstpointer a, gconstpointer b);

gint compare_id_node_plugin_vocodec (gconstpointer a, gconstpointer b);

void clear_list_plugins_effect (GSList *list_plugins_effect);






gboolean exit_ssip ();
gboolean init_ssip_proxy_params (gchar *proxy_host, gint proxy_port, gint ssip_local_port);
gboolean init_ssip ();

gboolean ssip_process_loop_create();
void ssip_process_loop_destroy();
void ssip_connect_to();




void ssip_proxy_toolbar (gpointer data, guint action, GtkWidget *widget);



#endif


