#include <gst/gst.h>
#include <stdbool.h>
#include<unistd.h>

#define DELAY_VALUE 7500

int tutorial_main (int argc, char *argv[])
{
    GstElement *pipeline, *source, *converter, *scaler, *fpssink, *fakesink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    gchar *fps_msg;
    int delay_show_FPS = 0;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Create the elements */
    source      =   gst_element_factory_make ("v4l2src", "source");
    converter   =   gst_element_factory_make("videoconvert", "converter");
    scaler      =   gst_element_factory_make("videoscale", "scaler");
    fpssink     =   gst_element_factory_make ("fpsdisplaysink", "fpssink");
    fakesink    =   gst_element_factory_make ("fakesink", "fakesink");

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new ("test-pipeline");

    if (!pipeline || !source || !converter || !scaler || !fpssink || !fakesink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline */
    gst_bin_add_many (GST_BIN (pipeline), source, converter, scaler, fpssink, NULL); //fakesink
    if (gst_element_link_many (source, converter, scaler, NULL) != TRUE) {
        g_printerr ("Multiple elements could not be linked.\n");
        gst_object_unref (pipeline);
        return -1;
    }

    GstCaps *caps;  
    caps = gst_caps_from_string("video/x-raw,width=640,height=480");
    if (gst_element_link_filtered(scaler, fpssink, caps) != TRUE) {
        g_printerr ("Elements could not be linked with filter.\n");
        gst_object_unref (pipeline);
        return -1;
    }




    /* Modify the source's properties */
    g_object_set(source, "device", "/dev/video4", 
        NULL);
        //"num-buffers", 10,

    /* Modify the sink's properties */
    g_object_set(fpssink, "text-overlay", false, "video-sink", fakesink, NULL);
    g_object_set(fakesink, "sync", FALSE, NULL);

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