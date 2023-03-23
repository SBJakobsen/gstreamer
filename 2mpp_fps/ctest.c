#include <gst/gst.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define DELAY_VALUE 7500

bool quitloop = true;


static void bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *) data;
   

    switch (GST_MESSAGE_TYPE (msg)) {

        case GST_MESSAGE_EOS:
            g_print ("End of stream\n");
            g_main_loop_quit (loop);
        break;
        case GST_MESSAGE_ERROR: {
            GError *err = NULL;
            gchar  *debug_info = NULL;
            
            gst_message_parse_error (msg, &err, &debug_info);
            g_printerr ("GST_MESSAGE_ERROR.\n");
            g_printerr ("Error from element:%s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_printerr ("Debug info: %s\n", debug_info ? debug_info : "none");

            g_clear_error (&err);
            g_free (debug_info);
            
            if(quitloop){
                g_main_loop_quit (loop);
            }
            
            break;
        }
        case GST_MESSAGE_WARNING: {
            GError *err = NULL;
            gchar  *debug_info = NULL;
            g_printerr ("GST_MESSAGE_WARNING.\n");
            gst_message_parse_warning (msg, &err, &debug_info);
            g_print ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_print ("Debugging information: %s\n", debug_info ? debug_info : "none");

            g_clear_error(&err);
            g_free (debug_info);
            break;
        }
        case GST_MESSAGE_TAG: // Ignore Tags
            // GstTagList *tags = NULL;

            // gst_message_parse_tag (msg, &tags);
            // g_print ("GST_MESSAGE_TAG from element %s\n", GST_OBJECT_NAME (msg->src));
            // gst_tag_list_unref (tags);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            g_print("GST_MESSAGE_STATE_CHANGED\n");
            break;
        case GST_MESSAGE_NEW_CLOCK:
            g_print("GST_MESSAGE_NEW_CLOCK\n");
            break;
        case GST_MESSAGE_STREAM_STATUS:
            g_print("GST_MESSAGE_STREAM_STATUS\n");
            break;
        case GST_MESSAGE_LATENCY:
            g_print("GST_MESSAGE_LATENCY\n");
            break;
        case GST_MESSAGE_ASYNC_DONE:
            g_print("GST_MESSAGE_ASYNC_DONE\n");
            break;
        case GST_MESSAGE_QOS:           // Ignore QoS
            //g_print("GST_MESSAGE_QOS\n");
            break;
        case GST_MESSAGE_STREAM_START:
            g_print("GST_MESSAGE_STREAM_START\n");
            break;
        default:
            g_printerr("GST_MESSAGE_TYPE enum: %d\n", GST_MESSAGE_TYPE (msg));
        break;
    }
}

static void v4l2src_call (GstElement * source, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *) data;
   

    switch (GST_MESSAGE_TYPE (msg)) {

        case GST_MESSAGE_EOS:
            g_print ("End of stream\n");
            g_main_loop_quit (loop);
        break;
        case GST_MESSAGE_ERROR: {
            GError *err = NULL;
            gchar  *debug_info = NULL;
            
            gst_message_parse_error (msg, &err, &debug_info);
            g_printerr ("GST_MESSAGE_ERROR specifically from v4l2src.\n");
            g_printerr ("Error from element:%s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_printerr ("Debug info: %s\n", debug_info ? debug_info : "none");

            g_clear_error (&err);
            g_free (debug_info);
            
            if(quitloop){
                g_main_loop_quit (loop);
            }
            
            break;
        }
        case GST_MESSAGE_WARNING: {
            GError *err = NULL;
            gchar  *debug_info = NULL;
            g_printerr ("GST_MESSAGE_WARNING specifically from v4l2src.\n");
            gst_message_parse_warning (msg, &err, &debug_info);
            g_print ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_print ("Debugging information: %s\n", debug_info ? debug_info : "none");

            g_clear_error(&err);
            g_free (debug_info);
            break;
        }
        default:
            g_printerr("GST_MESSAGE_TYPE enum: %d\n", GST_MESSAGE_TYPE (msg));
        break;
    }
}

static void fps_measurements_callback (GstElement * fpsdisplaysink, gdouble fps, gdouble droprate, gdouble avgfps, gpointer udata)
{
    g_print("Fpsdisplay. FPS: %f,\tDropped: %f,\tAverage %f \n", fps, droprate, avgfps);
}



int stream_main (int argc, char *argv[])
{
    GMainLoop *loop;

    GstElement *pipeline, *source, *mpph264enc, *h264parse, *avdec_h264, *fpssink, *fakesink;
    GstBus *bus;
    guint bus_watch_id;

    GstMessage *msg;
    GstStateChangeReturn ret;

    loop = g_main_loop_new (NULL, FALSE);

    gchar *fps_msg;
    int delay_show_FPS = 0;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    if(argc > 1){
        g_print("argc > 1, disabling quitloop\n");
        quitloop = false;
    }
    else {
        g_print("argc = 1, quitloop is enabled\n");
    }

    /* Create the elements */
    source      =   gst_element_factory_make("v4l2src",         "source");
    mpph264enc  =   gst_element_factory_make("mpph264enc",      "mpph264enc");
    h264parse   =   gst_element_factory_make("h264parse",       "h264parse");
    avdec_h264  =   gst_element_factory_make("avdec_h264",      "avdec_h264");
    fpssink     =   gst_element_factory_make("fpsdisplaysink",  "fpssink");
    fakesink    =   gst_element_factory_make("fakesink",        "fakesink");

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new ("test-pipeline");

    if (!pipeline || !source || !mpph264enc || !h264parse || !avdec_h264 || !fakesink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    /* Add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (bus_call), loop);
    g_signal_connect (fpssink, "fps-measurements", G_CALLBACK(fps_measurements_callback), NULL);
    g_signal_connect (source, "message", G_CALLBACK(v4l2src_call), loop);

    
    /* Build the pipeline */
    gst_bin_add_many (GST_BIN (pipeline), source, mpph264enc, h264parse, avdec_h264, fpssink, NULL); //fakesink removed from here as it should be null

    // source -> mpph264 -> 264parse > avdec_264 -> fpssink
    GstCaps *caps1;
    caps1 = gst_caps_from_string("video/x-raw,width=640,height=480, framerate=30/1");
    if (gst_element_link_filtered(source, mpph264enc, caps1) != TRUE) {
        g_printerr ("Source and mpph264enc could not be linked\n");
        gst_object_unref (pipeline);
        return -1;
    }

    if (gst_element_link (mpph264enc, h264parse) != TRUE) {
        g_printerr ("Mpph264enc and h264parsecould not be linked.\n");
        gst_object_unref (pipeline);
        return -1;
    }

    GstCaps *caps2;  
    caps2 = gst_caps_from_string("video/x-h264,stream-format=avc,alignment=au");
    if (gst_element_link_filtered(h264parse, avdec_h264, caps2) != TRUE) {
        g_printerr ("h264parse and avdec_h264 could not be linked");
        gst_object_unref (pipeline);
        return -1;
    }

    if (gst_element_link (avdec_h264, fpssink) != TRUE) {
        g_printerr ("avdec_h264 and fpssink could not be linked.\n");
        gst_object_unref (pipeline);
        return -1;
    }



    /* Modify the source's properties */
    g_object_set(source, "device", "/dev/video4", 
        NULL);

    /* Modify the sink's properties */
    g_object_set(fpssink, "text-overlay", false, "video-sink", fakesink, "signal-fps-measurements", true, NULL);
    g_object_set(fakesink, "sync", TRUE,
        "silent", false, NULL);

    /* Start playing */
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (pipeline);
        return -1;
    }

    /* Iterate */
    g_print ("Running...\n");
    g_main_loop_run (loop);
    
    g_print ("Program finished.\n");
    /* Free resources */
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    return 0;
}

int main (int argc, char *argv[])
{

    return stream_main (argc, argv);

}