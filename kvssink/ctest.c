#include <gst/gst.h>
#include <stdbool.h>

#define DELAY_VALUE 2000000

int tutorial_main (int argc, char *argv[])
{
    GstElement *pipeline, *source, *mpph264enc, *h264parse, *kvssink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    gchar *fps_msg;
    guint delay_show_FPS = 0;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Create the elements */
    source      =   gst_element_factory_make("v4l2src",     "source");
    mpph264enc  =   gst_element_factory_make("mpph264enc",  "mpph264enc")
    h264parse   =   gst_element_factory_make("h264parse",   "h264parse");
    kvssink     =   gst_element_factory_make("kvssink",     "kvssink");

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new ("test-pipeline");

    if (!pipeline || !source || !mpph264enc || !h264parse !kvssink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }
    
    /* Build the pipeline */
    gst_bin_add_many (GST_BIN (pipeline), source, mpph264enc, h264parse, kvssink, NULL);
    
    
    GstCaps *caps1;  
    caps1 = gst_caps_from_string("video/x-raw,width=640,height=480, framerate=30/1,format=YUY2");
    if (gst_element_link_filtered(source, mpph264enc, caps1) != TRUE) {
        g_printerr ("Source and mpph264 could not be linked.\n");
        gst_object_unref (pipeline);
        return -1;
    }

    GstCaps *caps2;  
    caps2 = gst_caps_from_string("video/x-h264,stream-format=avc,alignment=au");
    if (gst_element_link_filtered(h264parse, kvssink, caps2) != TRUE) {
        g_printerr ("h264parse and kvssink could not be linked together.\n");
        gst_object_unref (pipeline);
        return -1;
    }

    if (gst_element_link(mpph264enc, h264parse) != TRUE) {
        g_printerr ("mpph264enc and h264parse could not be linked\n");
        gst_object_unref (pipeline);
        return -1;
    }

    
    // HEEEEEEEEEEEERE



    /* Modify the source's properties */
    g_object_set (source, "device", "/dev/video4", NULL);

    /* Modify the sink's properties */
    g_object_set (fpssink, "text-overlay", false, "video-sink", fakesink, NULL);

    /* Start playing */
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (pipeline);
        return -1;
    }

    /* Wait until error or EOS */
    bus = gst_element_get_bus (pipeline);
    while(1)
    {
        
        msg = gst_bus_pop (bus);
        /* Parse message */
        if (msg != NULL) {
            GError *err;
            gchar *debug_info;
            
            switch (GST_MESSAGE_TYPE (msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error (msg, &err, &debug_info);
                    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
                    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
                    g_clear_error (&err);
                    g_free (debug_info);
                    goto stop_pipeline;
                    break;
                case GST_MESSAGE_WARNING:
                    gst_message_parse_warning (msg, &err, &debug_info);
                    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
                    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
                    g_clear_error(&err);
                    g_free (debug_info);
                    break;
                case GST_MESSAGE_EOS:
                    g_print ("End-Of-Stream reached.\n");
                    break;
                default:
                    
            }
            gst_message_unref (msg);
        }   
       
        //g_printerr ("This point was reached.\n");
        g_object_get (G_OBJECT (fpssink), "last-message", &fps_msg, NULL);
        delay_show_FPS++;
        if (fps_msg != NULL) {
            if ((delay_show_FPS % DELAY_VALUE) == 0) {
            g_print ("Frame info: %s\n", fps_msg);
            delay_show_FPS = 0;
            }
        }
    }
    
    stop_pipeline:
    g_print ("Program finished.\n");
    /* Free resources */
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    return 0;
}

int main (int argc, char *argv[])
{

    return tutorial_main (argc, argv);

}