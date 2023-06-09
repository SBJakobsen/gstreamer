#include <gst/gst.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define DELAY_VALUE 7500





typedef struct _CustomData {
  GstElement *pipeline;
  GMainLoop  *loop;
  GstElement *source;
  GstElement *mpph264enc;
  GstElement *queue;
  GstElement *h264parse;
  GstElement *avdec_h264;
  GstElement *fpssink;
  GstElement *fakesink;
} CustomData;

static void bus_call (GstBus *bus, GstMessage *msg, CustomData *data)
{
    
   

    switch (GST_MESSAGE_TYPE (msg)) {

        case GST_MESSAGE_EOS:
            g_print ("END OF STREAM RECEIVED\n");
            g_main_loop_quit (data->loop);
        break;
        case GST_MESSAGE_ERROR: {
            GError *err = NULL;
            gchar  *debug_info = NULL;
            
            gst_message_parse_error (msg, &err, &debug_info);
            g_print ("GST_MESSAGE_ERROR.\n");
            g_print ("Error from element:%s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_print ("Debug info: %s\n", debug_info ? debug_info : "none");

            // Stringcompare GST_OBJECT_NAME == "h264parse" -> reset pipeline
            if(strcmp(GST_OBJECT_NAME(msg->src), "h264parse") == 0)
            {
                g_print("Error is from the h264parse element\n");
                if(strstr(debug_info, "No H.264 NAL") != NULL)
                {
                    g_print("And it is THAT error\n");


                    GstStateChangeReturn ret;
                    g_print("Attempting to set pipeline to NULL \n");
                    ret = gst_element_set_state (data->pipeline, GST_STATE_NULL);
                    if (ret == GST_STATE_CHANGE_FAILURE) {
                        g_print ("Unable to set the pipeline to GST_STATE_NULL.\n");
                        //g_main_loop_quit (data->loop);
                        //return;
                    }
                    
                    // g_print ("Unreffing pipeline as part of memory leak test\n");
                    // gst_object_unref (data->pipeline);   
                    

                    g_print("And then attempting setting it back to PLAYING \n");
                    ret = gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
                    if (ret == GST_STATE_CHANGE_FAILURE) {
                        g_print ("Unable to set the pipeline to GST_STATE_PLAYING.\n");
                        //g_main_loop_quit (data->loop);
                    }

                    

                    // g_print("Attempting to start pipeline flush \n");
                    // gst_element_send_event(GST_ELEMENT (data->pipeline), gst_event_new_flush_start());

                    // g_print("Attempting to stop pipeline flush \n");
                    // gst_element_send_event(GST_ELEMENT (data->pipeline), gst_event_new_flush_stop(false));

                    // g_print("Attempting to re-start pipeline \n");
                    // ret = gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
                    // if (ret == GST_STATE_CHANGE_FAILURE) {
                    //     g_print ("Unable to set the pipeline to GST_STATE_PLAYING.\n");
                    //     g_main_loop_quit (data->loop);
                    //     return;
                    // }

                }
                else{
                    g_print("but is it NOT the known error. Quitting\n");
                    g_main_loop_quit (data->loop);
                }
            }
            else{
                
                g_main_loop_quit (data->loop);
                
            }

            g_clear_error (&err);
            g_free (debug_info);
            
            break;
        }
        case GST_MESSAGE_WARNING: {
            GError *err = NULL;
            gchar  *debug_info = NULL;
            g_print ("GST_MESSAGE_WARNING.\n");
            gst_message_parse_warning (msg, &err, &debug_info);
            g_print("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_print("Debugging information: %s\n", debug_info ? debug_info : "none");

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
        case GST_MESSAGE_STATE_CHANGED: {
                GstState old_state;
                GstState new_state;
                GstState pending_state;
                gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                g_print("GST_MESSAGE_STATE_CHANGED: %s state change: %s --> %s:\t\t Pending state: %s\n",
                    GST_OBJECT_NAME(msg->src), gst_element_state_get_name (old_state), gst_element_state_get_name (new_state),gst_element_state_get_name (new_state));
                // if(strcmp(GST_OBJECT_NAME(msg->src), "PIPELINE") == 0 && strcmp(gst_element_state_get_name (new_state), "NULL") == 0)
                // {
                //     g_print("PIPELINE WAS SET TO NULL, ATTEMPTING TO START PLAYING AGAIN");
                //     GstStateChangeReturn ret;
                //     ret = gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
                //     if (ret == GST_STATE_CHANGE_FAILURE) {
                //         g_print ("Unable to set the pipeline to GST_STATE_PLAYING.\n");
                //         //g_main_loop_quit (data->loop);
                //     }
                //     
                // }
                
            }
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
            g_print("GST_MESSAGE_TYPE enum: %d\n", GST_MESSAGE_TYPE (msg));
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

    CustomData data;
    GstBus *bus;
    guint bus_watch_id;

    GstMessage *msg;
    GstStateChangeReturn ret;
    
    

    data.loop = g_main_loop_new (NULL, FALSE);

    gchar *fps_msg;
    int delay_show_FPS = 0;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);


    /* Create the elements */
    data.source     =   gst_element_factory_make("v4l2src",         "source");
    data.mpph264enc =   gst_element_factory_make("mpph264enc",      "mpph264enc");
    data.h264parse  =   gst_element_factory_make("h264parse",       "h264parse");
    data.avdec_h264 =   gst_element_factory_make("avdec_h264",      "avdec_h264");
    data.queue      =   gst_element_factory_make("queue",           "queue");
    data.fpssink    =   gst_element_factory_make("fpsdisplaysink",  "fpssink");
    data.fakesink   =   gst_element_factory_make("fakesink",        "fakesink");

    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new ("PIPELINE");

    if (!data.pipeline || !data.source || !data.mpph264enc || !data.queue || !data.h264parse || !data.avdec_h264|| !data.fakesink) {
        g_print ("Not all elements could be created.\n");
        return -1;
    }

    /* Add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (data.pipeline));
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (bus_call), &data);
    g_signal_connect (data.fpssink, "fps-measurements", G_CALLBACK(fps_measurements_callback), NULL);

    
    /* Build the pipeline */
    gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.mpph264enc, data.queue, data.h264parse, data.avdec_h264, data.fpssink, NULL); //fakesink removed from here as it should be null

    // source -> mpph264 -> queue -> 264parse -> fpssink
    GstCaps *caps1;
    caps1 = gst_caps_from_string("video/x-raw,width=640,height=480, framerate=30/1");
    if (gst_element_link_filtered(data.source, data.mpph264enc, caps1) != TRUE) {
        g_print ("Source and mpph264enc could not be linked\n");
        gst_object_unref (data.pipeline);
        return -1;
    }

    if (gst_element_link (data.mpph264enc, data.queue) != TRUE) {
        g_print ("Mpph264enc and queue could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }

    if (gst_element_link (data.queue, data.h264parse) != TRUE) {
        g_print ("queue and h264parse could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }

    GstCaps *caps2;  
    caps2 = gst_caps_from_string("video/x-h264,stream-format=avc,alignment=au");
    if (gst_element_link_filtered(data.h264parse, data.avdec_h264, caps2) != TRUE) {
        g_print ("h264parse and avdec_h264 could not be linked");
        gst_object_unref (data.pipeline);
        return -1;
    }

    if (gst_element_link (data.avdec_h264, data.fpssink) != TRUE) {
        g_print ("avdec_h264 and h264parse could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }




    /* Modify the source's properties */
    g_object_set(data.source, "device", "/dev/video4", 
        "do-timestamp", true,
        NULL);

    /* Modify the sink's properties */
    g_object_set(data.fpssink, "text-overlay", false, "video-sink", data.fakesink, "signal-fps-measurements", true, NULL);
    g_object_set(data.fakesink, "sync", TRUE,
        "silent", false, NULL);

    /* Start playing */
    ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_print ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }

    /* Iterate */
    g_print("Running...\n");
    g_main_loop_run (data.loop);
    
    g_print ("Program finished.\n");
    /* Free resources */
    gst_object_unref (bus);
    gst_element_set_state (data.pipeline, GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_print ("On exit, unable to change pipeline state: PLAYING->NULL.\n");
    }
    gst_object_unref (data.pipeline);
    return 0;
}

int main (int argc, char *argv[])
{

    return stream_main (argc, argv);

}