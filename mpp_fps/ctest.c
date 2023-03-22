#include <gst/gst.h>
#include <stdbool.h>
#include <unistd.h>

#define DELAY_VALUE 7500

int tutorial_main (int argc, char *argv[])
{
    GstElement *pipeline, *source, *mpph264enc, *h264parse, *avdec_h264, *fpssink, *fakesink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    gchar *fps_msg;
    int delay_show_FPS = 0;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

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

    /* Link elements */
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
    g_object_set(fpssink, "text-overlay", false, "video-sink", fakesink, NULL);
    g_object_set(fakesink, "sync", TRUE, NULL);

    /* Start playing */
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (pipeline);
        return -1;
    }

    /* Wait until error or EOS */
    bus = gst_element_get_bus (pipeline);
    loop_start:
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
                break;
                    
            }
            gst_message_unref (msg);
        }   
       
        //g_printerr ("This point was reached.\n");
        g_object_get (G_OBJECT (fpssink), "last-message", &fps_msg, NULL);
        delay_show_FPS++;
        if (fps_msg != NULL) {
            if (delay_show_FPS > DELAY_VALUE) {
                //g_print ("Value of delay_show_FPS: %i and value of DELAY_VALUE: %i\n", delay_show_FPS, DELAY_VALUE);
                g_print ("Frame info: %s\n", fps_msg);
                delay_show_FPS = 0;
            }
        }
        g_free(fps_msg);
        sleep(0.2);
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