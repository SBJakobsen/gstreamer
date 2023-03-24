#include <gst/gst.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

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

            // Stringcompare GST_OBJECT_NAME == "h264parse" -> reset pipeline
            //if(strcmp(GST_OBJECT_NAME(msg->src))

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

static void fps_measurements_callback (GstElement * fpsdisplaysink, gdouble fps, gdouble droprate, gdouble avgfps, gpointer udata)
{
    
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    g_print("Fpsdisplay %02d:%02d:%02d. FPS: %f,\tDropped: %f,\tAverage %f \n", tm.tm_hour, tm.tm_min, tm.tm_sec, fps, droprate, avgfps);

}



int stream_main (int argc, char *argv[])
{
    GMainLoop *loop;

    GstElement *pipeline, *source, *mpph264enc, *h264parse, *kvssink;
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
    kvssink     =   gst_element_factory_make("kvssink",         "kvssink");

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new ("test-pipeline");

    if (!pipeline || !source || !mpph264enc || !h264parse || !kvssink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    /* Add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (bus_call), loop);

    
    /* Build the pipeline */
    gst_bin_add_many (GST_BIN (pipeline), source, mpph264enc, h264parse, kvssink, NULL); //fakesink removed from here as it should be null

    // source -> mpph264 -> 264parse > kvssink
    GstCaps *caps1;
    caps1 = gst_caps_from_string("video/x-raw,width=640,height=480,framerate=30/1");
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
    if (gst_element_link_filtered(h264parse, kvssink, caps2) != TRUE) {
        g_printerr ("h264parse and avdec_h264 could not be linked");
        gst_object_unref (pipeline);
        return -1;
    }



    /* Modify the source's properties */
    g_object_set(source, "device", "/dev/video4", 
        "do-timestamp", true,
        NULL);

    g_printerr("About to set kvssink parameters!\n");

    /* Modify the sink's properties */
    g_object_set(kvssink, 
        "stream-name", "15e0dc81d12c414aa02b49b990921c8d",
        "framerate", 30,
        "restart-on-error", true,
        "retention-period", 730,
        "log-config", "./usr/src/app/kvs_log_configuration",
        "iot-certificate", "iot-certificate,endpoint=\"tesdt\",cert-path=\"/certs/cert.pem\",key-path=\"/certs/privkey.pem\",ca-path=\"/certs/root-CA.pem\",role-aliases=\"alias\"",
        NULL);
        

    gchar *stream_name;
    guint framerate;
    bool restart_on_error;
    guint retention_period;
    gchar *log_config;
    gchar *iot_certificate;

    g_object_get(kvssink,
        "stream-name", &stream_name,
        "framerate", &framerate,
        "restart-on-error", &restart_on_error,
        "retention-period", &retention_period,
        "log-config", &log_config,
        "iot-certificate", &iot_certificate,
        NULL);
    
    g_print("\nThe values are \nstream name: %s\nFramerate: %i \nRetention-period: %i \nLog-config: %s \niot-certificate: %s\n\n", stream_name, framerate, retention_period, log_config, iot_certificate);


    

    g_printerr("Finished setting kvssink parameters!\n");

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