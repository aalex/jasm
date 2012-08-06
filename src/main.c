/*
 * Copyright (C) 2012 Alexandre Quessy
 *
 * This file is part of Jasm.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Jasm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Jasm.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Main file.
 * Jasm records and plays back audio video loops.
 */

#include <stdlib.h>
#include <lo/lo.h>
#include <clutter-gst/clutter-gst.h>
#include "config.h"

// Macros. (ews!)
#define UNUSED(x) ((void) (x))

// constants
static const guint STAGE_WIDTH = 1024;
static const guint STAGE_HEIGHT = 768;
static const ClutterColor black = { 0x00, 0x00, 0x00, 0xff };
static const gchar * const STAGE_TITLE = "jasm - audio-video looper";
static const gchar * const LIVEVIDEO_ACTOR = "livevideo0";
static const gchar * const BOX_ACTOR = "image";
static const gchar * const PROGRAM_DESC = "- audio-video looper";

/**
 * Storage for command line options.
 */
static gboolean option_videotestsrc = FALSE;
static gboolean option_verbose = FALSE;
static gboolean option_loop = TRUE;
static gboolean option_fullscreen = FALSE;
static gboolean option_version = FALSE;
static guint option_osc_receive_port = 19999;

static GOptionEntry entries[] =
{
  { "videotestsrc", 't', 0, G_OPTION_ARG_NONE, &option_videotestsrc,
    "Use video test source instead of camera", NULL },
  { "verbose", 'v', 0, G_OPTION_ARG_NONE, &option_verbose,
    "Be verbose", NULL },
  { "version", 'V', 0, G_OPTION_ARG_NONE, &option_version,
    "Show version info and exit", NULL },
  { "fullscreen", 'f', 0, G_OPTION_ARG_NONE, &option_fullscreen,
    "Launch window as full screen", NULL },
  { "osc-receive-port", 'p', 0, G_OPTION_ARG_INT, &option_osc_receive_port, "OSC port to listen on (M)", "M" },
  { NULL }
};

/**
 * Function definitions.
 */
static void on_texture_size_change (ClutterTexture *texture, gint width, gint height, gpointer user_data);
static ClutterActor * setup_camera_texture(ClutterActor *stage);
static void on_fullscreen(ClutterStage* stage, gpointer user_data);
static void on_unfullscreen(ClutterStage* stage, gpointer user_data);
static gboolean key_press_event(ClutterActor *stage, ClutterEvent *event, gpointer user_data);
static void update_textures_sizes(ClutterStage *stage);
static void on_osc_error(int num, const char *msg, const char *path);


void on_texture_size_change (ClutterTexture *texture,
    gint width,
    gint height,
    gpointer user_data)
{
  ClutterActor *stage;
  gfloat new_x, new_y, new_width, new_height;
  gfloat stage_width, stage_height;
  stage = clutter_actor_get_stage (CLUTTER_ACTOR (texture));
  if (stage == NULL)
    return;
  clutter_actor_get_size (stage, &stage_width, &stage_height);
  new_height = (height * stage_width) / width;
  if (new_height <= stage_height)
    {
      new_width = stage_width;
      new_x = 0;
      new_y = (stage_height - new_height) / 2;
    }
  else
    {
      new_width  = (width * stage_height) / height;
      new_height = stage_height;
      new_x = (stage_width - new_width) / 2;
      new_y = 0;
    }
  clutter_actor_set_position (CLUTTER_ACTOR (texture), new_x, new_y);
  clutter_actor_set_size (CLUTTER_ACTOR (texture), new_width, new_height);
}

void update_textures_sizes(ClutterStage *stage)
{
    // if (option_verbose)
    //     g_print("%s\n", __FUNCTION__);
    GList *textures = NULL;
    textures = g_list_append(textures, (gpointer) CLUTTER_TEXTURE(clutter_container_find_child_by_name(CLUTTER_CONTAINER(stage), BOX_ACTOR)));
    GList *iter = NULL;
    for (iter = textures; iter; iter = iter->next)
    {
        gfloat width;
        gfloat height;
        clutter_actor_get_size(CLUTTER_ACTOR(stage), &width, &height);
        clutter_actor_set_size(CLUTTER_ACTOR(iter->data), width, height);
    }
    g_list_free(textures);
}

/**
 * In fullscreen mode, hides the cursor.
 */
void on_fullscreen(ClutterStage* stage, gpointer user_data)
{
    UNUSED(user_data);
    clutter_stage_hide_cursor(stage);
    update_textures_sizes(stage);
}

/**
 * In windowed mode, shows the cursor.
 */
void on_unfullscreen(ClutterStage* stage, gpointer user_data)
{
    UNUSED(user_data);
    clutter_stage_show_cursor(stage);
    update_textures_sizes(stage);
}

gboolean key_press_event(ClutterActor *stage, ClutterEvent *event, gpointer user_data)
{
    UNUSED(user_data);
    guint keyval = clutter_event_get_key_symbol(event);
    ClutterModifierType state = clutter_event_get_state(event);
    gboolean ctrl_pressed = (state & CLUTTER_CONTROL_MASK ? TRUE : FALSE);
    if (keyval == CLUTTER_KEY_Escape)
    {
        if (clutter_stage_get_fullscreen(CLUTTER_STAGE(stage)))
            clutter_stage_set_fullscreen(CLUTTER_STAGE(stage), FALSE);
        else
            clutter_stage_set_fullscreen(CLUTTER_STAGE(stage), TRUE);
    }
    else if (keyval == CLUTTER_KEY_q)
    {
        // Quit application on ctrl-q, this quits the main loop
        // (if there is one)
        if (ctrl_pressed)
        {
            if (option_verbose)
                g_print("ctrl-q pressed. quitting.\n");
            clutter_main_quit();
        }
    }
    return TRUE;
}

int main(int argc, char *argv[])
{
  ClutterTimeline  *timeline;
  ClutterActor     *stage;

  if (argc < 1)
    {
      g_error ("Usage: %s", argv[0]);
      return EXIT_FAILURE;
    }
  else
  {
    GError *error = NULL;
    GOptionContext *context;
  
    context = g_option_context_new (PROGRAM_DESC);
    g_option_context_add_main_entries (context, entries, NULL); //GETTEXT_PACKAGE);
    //g_option_context_add_group (context, gtk_get_option_group (TRUE));
    if (! g_option_context_parse (context, &argc, &argv, &error))
      {
        g_print ("option parsing failed: %s\n", error->message);
        return EXIT_FAILURE; // exit (1);
      }
  }

  if (option_version)
  {
    g_print("contextize version %s\n", PACKAGE_VERSION);
    return 0;
  }

  g_thread_init(NULL); // to load images asynchronously. must be called before clutter_init
  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    {
      g_error ("Failed to initialize clutter\n");
      return EXIT_FAILURE;
    }
  gst_init (&argc, &argv);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size(stage, STAGE_WIDTH, STAGE_HEIGHT);
  clutter_stage_set_color(CLUTTER_STAGE(stage), &black);
  clutter_stage_set_title(CLUTTER_STAGE(stage), STAGE_TITLE);

  /* Make a timeline */
  timeline = clutter_timeline_new (1000);
  g_object_set(timeline, "loop", TRUE, NULL);

  ClutterActor *texture_live = setup_camera_texture(stage);

  // create the layout
  ClutterLayoutManager *layout;
  ClutterActor *box;
  layout = clutter_bin_layout_new(CLUTTER_BIN_ALIGNMENT_CENTER,
                                  CLUTTER_BIN_ALIGNMENT_CENTER);
  box = clutter_box_new(layout); /* then the container */
  clutter_actor_set_name(box, BOX_ACTOR);
  /* we can use the layout object to add actors */
  clutter_bin_layout_add(CLUTTER_BIN_LAYOUT(layout), texture_live,
                         CLUTTER_BIN_ALIGNMENT_FILL,
                         CLUTTER_BIN_ALIGNMENT_FILL);
  clutter_container_add_actor(CLUTTER_CONTAINER(stage), box);
  clutter_actor_set_size(box, STAGE_WIDTH, STAGE_HEIGHT);
  clutter_actor_show_all(box);

  g_signal_connect(G_OBJECT(stage), "fullscreen", G_CALLBACK(on_fullscreen), NULL);
  g_signal_connect(G_OBJECT(stage), "unfullscreen", G_CALLBACK(on_unfullscreen), NULL);
  g_signal_connect(G_OBJECT(stage), "key-press-event", G_CALLBACK(key_press_event), NULL);

  /* start the timeline */
  clutter_timeline_start (timeline);

  clutter_actor_show_all (stage);

  if (option_fullscreen)
      clutter_stage_set_fullscreen(CLUTTER_STAGE(stage), TRUE);

  // OSC server
  lo_server_thread osc_server = NULL;
  if (option_osc_receive_port != 0)
  {
    gchar *osc_port_str = g_strdup_printf("%i", option_osc_receive_port);
    if (option_verbose)
      g_print("Listening on osc.udp://localhost:%s\n", osc_port_str);
    osc_server = lo_server_thread_new(osc_port_str, on_osc_error);
    g_free(osc_port_str);
  
    lo_server_thread_start(osc_server);
  }

  clutter_main();

  if (option_osc_receive_port != 0)
    lo_server_thread_free(osc_server);
  return EXIT_SUCCESS;
}

void on_osc_error(int num, const char *msg, const char *path)
{
    g_print("liblo server error %d in path %s: %s\n", num, path, msg);
}

void link_or_die(GstElement *from, GstElement *to)
{
    gboolean is_linked = gst_element_link(from, to);
    gchar *from_name = gst_element_get_name(from);
    gchar *to_name = gst_element_get_name(to);
    if (! is_linked)
    {
        g_error("Could not link %s to %s.\n", from_name, to_name);
    }
    g_free(from_name);
    g_free(to_name);
}

ClutterActor * setup_camera_texture(ClutterActor *stage)
{
  ClutterActor     *texture;
  GstPipeline      *pipeline;
  GstElement       *videosrc0;
  GstElement       *flip0;
  GstElement       *colorspace0;
  GstElement       *alpha0;
  GstElement       *colorspace1;
  GstElement       *cluttersink0;
  GstElement       *videomixer;
  GstElement       *colorspacemix;
  GstElement       *multifilesrc;
  GstElement       *decodebin;
  GstElement       *colorspace;
  GstElement       *videoformat;
  GstElement       *deinterlace;

  /* We need to set certain props on the target texture currently for
   * efficient/corrent playback onto the texture (which sucks a bit)
  */
  texture = g_object_new(CLUTTER_TYPE_TEXTURE,
        "disable-slicing", TRUE,
        NULL);

  /* Set up pipeline */
  pipeline = GST_PIPELINE(gst_pipeline_new (NULL));

  if (option_videotestsrc)
    videosrc0 = gst_element_factory_make("videotestsrc", "videotestsrc0");
  else
    videosrc0 = gst_element_factory_make("gconfvideosrc", "gconfvideosrc0"); // or autovideosrc?
  // TODO: check if option_flip_horizontal
  flip0 = gst_element_factory_make("videoflip", "videoflip0");
  g_object_set(G_OBJECT (flip0), "method", 4, NULL); // GST_VIDEO_FLIP_METHOD_HORIZ
  colorspace0 = gst_element_factory_make("ffmpegcolorspace", "colorspace0");
  //colorspace1 = gst_element_factory_make("ffmpegcolorspace", "colorspace1");
  //alpha0 = gst_element_factory_make("alpha", "alpha0");
  //videomixer = gst_element_factory_make("videomixer", NULL);
  //colorspacemix = gst_element_factory_make("ffmpegcolorspace", NULL);
  //multifilesrc = gst_element_factory_make("multifilesrc", NULL);
  //decodebin = gst_element_factory_make("jpegdec", NULL);
  //colorspace = gst_element_factory_make("ffmpegcolorspace", NULL);
  videoformat = gst_element_factory_make("capsfilter", NULL);
  cluttersink0 = gst_element_factory_make("cluttersink", "cluttersink0");
  //deinterlace = gst_element_factory_make("deinterlace", NULL);
  
  //g_object_set(alpha0, "method", 3, NULL); // 1 -> ALPHA_METHOD_GREEN 3 -> ALPHA_METHOD_CUSTOM  
  //g_object_set(G_OBJECT (multifilesrc),"location",DEFAULT_IMAGE_PATH,NULL); 
  GstCaps *imagecaps = gst_caps_from_string ("video/x-raw-yuv,format=(fourcc)AYUV");
  g_object_set (videoformat,"caps",  
    imagecaps,
    NULL);
  g_object_set(cluttersink0, "texture", CLUTTER_TEXTURE(texture), NULL); 
  
  GstElement *tee0 = gst_element_factory_make("tee", "tee0");
  GstElement *queue0 = gst_element_factory_make("queue", "queue0");
  GstElement *queue1 = gst_element_factory_make("queue", "queue1");
  GstElement *queue2 = gst_element_factory_make("queue", "queue2");
  GstElement *xvimagesink0 = gst_element_factory_make("xvimagesink", "xvimagesink0");
  g_object_set(xvimagesink0, "force-aspect-ratio", TRUE, NULL);

  gst_bin_add_many(GST_BIN(pipeline), 
        videosrc0, 
        //alpha0, 
        colorspace0, 
        tee0, 
        flip0, 
        queue0, 
        queue1, 
        queue2, 
        //colorspace1, 
        cluttersink0, 
        xvimagesink0, 
        //videomixer, 
        //colorspacemix, 
       // multifilesrc, 
       // decodebin, 
        //colorspace, 
        videoformat, 
        //deinterlace,
        NULL); 

   //image branch 
   //link_or_die(multifilesrc,decodebin);  
   //link_or_die(decodebin,colorspace);  
   //link_or_die(colorspace,videoformat);  
   //link_or_die(videoformat,videomixer);  
  
   //link_or_die(videosrc0, deinterlace);
   link_or_die(videosrc0, colorspace0); 
   link_or_die(colorspace0, flip0); 
   link_or_die(flip0, queue2); 
   link_or_die(queue2, tee0); 
   if (! gst_element_link_pads(tee0, "src0", queue0, "sink")) 
     g_error("Could not link %s to %s.\n", "tee0", "queue0"); 
   if (! gst_element_link_pads(tee0, "src1", queue1, "sink")) 
     g_error("Could not link %s to %s.\n", "tee0", "queue1"); 
   link_or_die(queue0, cluttersink0); 
   //link_or_die(alpha0, videomixer); 
   //link_or_die(videomixer,colorspacemix); 
   //link_or_die(colorspacemix,cluttersink0); 
   link_or_die(queue1, xvimagesink0); 

  //saving objects for extrernal control
  //g_object_set_data (G_OBJECT (texture), "gstalpha", alpha0);
  //g_object_set_data (G_OBJECT (texture), "gstfile", multifilesrc);

  // TODO:
  // if (option_verbose)
  // {
  //   g_print("videosrc0 -- colorspace0 -- queue2 --  tee0 -- queue0 -- alpha0 -- cluttersink0\n");
  //   g_print("                                            -- queue1 -- xvimagesink0\n");
  // }

  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  clutter_actor_set_name(texture, LIVEVIDEO_ACTOR);
  return texture;
}
 
