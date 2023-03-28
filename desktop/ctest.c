#include <gst/gst.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define WIDTHBUF 20
#define HEIGHTBUF 20
#define FRAMEBUF 20
#define RESINBUF 20
#define CERTSBUF 20
#define ENDPBUF 20
#define ROLEBUF 20
#define AWSRBUF 20

bool quitloop = true;

typedef struct _CustomData {
  GstElement *pipeline;
  GMainLoop  *loop;
  GstElement *source;
  GstElement *videoconvert;
  GstElement *x264enc;
  GstElement *h264parse;
  GstElement *kvssink;
} CustomData;

typedef struct _EnvVariables {
    char charwidth[WIDTHBUF];
    char charheight[HEIGHTBUF];
    char charframerate[FRAMEBUF];
    int  width;
    int  height;
    int  framerate;
    char resin_device_uuid[RESINBUF];
    char certsdir[CERTSBUF];
    char aws_endpoint[ENDPBUF];
    char role_alias[ROLEBUF];
    char aws_region[AWSRBUF];
} EnvVariables;



gboolean get_env_variables ( EnvVariables *vars){
    char *p_width               = "WIDTH";
    char *p_height              = "HEIGHT";
    char *p_framerate           = "FRAMERATE";
    char *p_resin_device_uuid   = "RESIN_DEVICE_UUID";
    char *p_certsdir            = "CERTSDIR";
    char *p_aws_endpoint        = "AWS_ENDPOINT";
    char *p_role_alias          = "ROLE_ALIAS";
    char *p_aws_region          = "AWS_REGION";
    
    if( !getenv(p_width) || !getenv(p_height) || !getenv(p_framerate) || !getenv(p_resin_device_uuid) || 
        !getenv(p_certsdir) || !getenv(p_aws_endpoint) || !getenv(p_role_alias) || !getenv(p_aws_region))
        {
        g_print("Missing some required environment variable\n");
        return false;
    }

    if( snprintf(vars->width, WIDTHBUF, "%s",               getenv(p_width)) >= WIDTHBUF ||
        snprintf(vars->height, HEIGHTBUF, "%s",             getenv(p_height)) >= HEIGHTBUF || 
        snprintf(vars->framerate, FRAMEBUF, "%s",           getenv(p_framerate)) >= FRAMEBUF || 
        snprintf(vars->resin_device_uuid, RESINBUF, "%s",   getenv(p_resin_device_uuid)) >= RESINBUF || 
        snprintf(vars->certsdir, CERTSBUF, "%s",            getenv(p_certsdir)) >= CERTSBUF || 
        snprintf(vars->aws_endpoint, ENDPBUF, "%s",         getenv(p_aws_endpoint)) >= ENDPBUF || 
        snprintf(vars->role_alias, ROLEBUF, "%s",           getenv(p_role_alias)) >= ROLEBUF || 
        snprintf(vars->aws_region, AWSRBUF, "%s",           getenv(p_aws_region)) >= AWSRBUF
        ){
        g_print("A environment variable did not fit within its' buffer");
        return false;
    }

    // Implement converting WIDTH, HEIGHT and FRAMERATE to integers. 





    

}

static void bus_call (GstBus *bus, GstMessage *msg, CustomData *data)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);



    switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_EOS:
            g_print ("[%d/%d - %02d:%02d:%02d] ", tm.tm_mday, tm.tm_mon+1 ,tm.tm_hour, tm.tm_min, tm.tm_sec);
            g_print ("END OF STREAM RECEIVED\n");
            g_main_loop_quit (data->loop);
        break;
        case GST_MESSAGE_ERROR: {
            GError *err = NULL;
            gchar  *debug_info = NULL;
            gst_message_parse_error (msg, &err, &debug_info);

            g_print ("[%d/%d - %02d:%02d:%02d] ", tm.tm_mday, tm.tm_mon+1 ,tm.tm_hour, tm.tm_min, tm.tm_sec);
            g_print ("GST_MESSAGE_ERROR from element: %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_print ("Debug info: %s\n", debug_info ? debug_info : "none");

            
            if(strcmp(GST_OBJECT_NAME(msg->src), "h264parse") == 0 && strstr(debug_info, "No H.264 NAL") != NULL)
            {
                g_print("Attempting to recover \n");

                GstStateChangeReturn ret;
                g_print("Setting pipeline to NULL \n");
                ret = gst_element_set_state (data->pipeline, GST_STATE_NULL);
                if (ret == GST_STATE_CHANGE_FAILURE) {
                    g_print ("Unable to set the pipeline to GST_STATE_NULL.\n");
                }
                
                GstStructure *iot_certificate = gst_structure_new_from_string ("iot-certificate,endpoint=crhxlosa5p0oo.credentials.iot.eu-west-1.amazonaws.com,cert-path=/usr/src/app/certs/cert.pem,key-path=/usr/src/app/certs/privkey.pem,ca-path=/usr/src/app/certs/root-CA.pem,role-aliases=fbview-kinesis-video-access-role-alias");

                g_print("Setting kvssink parameters again\n");
                /* Modify the sink's properties */
                g_object_set(data->kvssink, 
                    "stream-name", "15e0dc81d12c414aa02b49b990921c8d",
                    "framerate", (guint)30,
                    "restart-on-error", true,
                    "retention-period", 730,
                    "aws-region", "eu-west-1",
                    "log-config", "/usr/src/app/kvs_log_configuration",
                    "iot-certificate", iot_certificate,
                    NULL);

                g_print("Setting pipeline to PLAYING \n");
                ret = gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
                if (ret == GST_STATE_CHANGE_FAILURE) {
                    g_print ("Unable to set the pipeline to GST_STATE_PLAYING.\n");
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
            g_print ("[%d/%d - %02d:%02d:%02d] ", tm.tm_mday, tm.tm_mon+1 ,tm.tm_hour, tm.tm_min, tm.tm_sec);
            g_print ("GST_MESSAGE_WARNING.\n");
            gst_message_parse_warning (msg, &err, &debug_info);
            g_print("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_print("Debugging information: %s\n", debug_info ? debug_info : "none");

            g_clear_error(&err);
            g_free (debug_info);
            break;
        }
        case GST_MESSAGE_TAG: // Ignore Tags
            GstTagList *tags = NULL;

            gst_message_parse_tag (msg, &tags);
            g_print ("[%d/%d - %02d:%02d:%02d] ", tm.tm_mday, tm.tm_mon+1 ,tm.tm_hour, tm.tm_min, tm.tm_sec);
            g_print ("GST_MESSAGE_TAG from element %s\n", GST_OBJECT_NAME (msg->src));
            gst_tag_list_unref (tags);
            break;
        case GST_MESSAGE_STATE_CHANGED: {
                GstState old_state;
                GstState new_state;
                GstState pending_state;
                gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                g_print ("[%d/%d - %02d:%02d:%02d] ", tm.tm_mday, tm.tm_mon+1 ,tm.tm_hour, tm.tm_min, tm.tm_sec);
                g_print("GST_MESSAGE_STATE_CHANGED: %s state change: %s --> %s:\t Pending state: %s\n",
                GST_OBJECT_NAME(msg->src), gst_element_state_get_name (old_state), gst_element_state_get_name (new_state),gst_element_state_get_name (new_state));
                
            }
            break;
        case GST_MESSAGE_NEW_CLOCK:
            g_print ("[%d/%d - %02d:%02d:%02d] ", tm.tm_mday, tm.tm_mon+1 ,tm.tm_hour, tm.tm_min, tm.tm_sec);
            g_print("GST_MESSAGE_NEW_CLOCK\n");
            break;
        case GST_MESSAGE_STREAM_STATUS:
            g_print ("[%d/%d - %02d:%02d:%02d] ", tm.tm_mday, tm.tm_mon+1 ,tm.tm_hour, tm.tm_min, tm.tm_sec);
            g_print("GST_MESSAGE_STREAM_STATUS\n");
            break;
        case GST_MESSAGE_LATENCY:
            g_print ("[%d/%d - %02d:%02d:%02d] ", tm.tm_mday, tm.tm_mon+1 ,tm.tm_hour, tm.tm_min, tm.tm_sec);
            g_print("GST_MESSAGE_LATENCY\n");
            break;
        case GST_MESSAGE_ASYNC_DONE:
            g_print ("[%d/%d - %02d:%02d:%02d] ", tm.tm_mday, tm.tm_mon+1 ,tm.tm_hour, tm.tm_min, tm.tm_sec);
            g_print("GST_MESSAGE_ASYNC_DONE\n");
            break;
        case GST_MESSAGE_QOS:           // Ignore QoS
            g_print("GST_MESSAGE_QOS\n");
            break;
        case GST_MESSAGE_STREAM_START:
            g_print ("[%d/%d - %02d:%02d:%02d] ", tm.tm_mday, tm.tm_mon+1 ,tm.tm_hour, tm.tm_min, tm.tm_sec);
            g_print("GST_MESSAGE_STREAM_START\n");
            break;
        default:
            g_print ("[%d/%d - %02d:%02d:%02d] ", tm.tm_mday, tm.tm_mon+1 ,tm.tm_hour, tm.tm_min, tm.tm_sec);
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

    EnvVariables vars;
    if(!get_env_variables(&vars))
    {
        exit(1);
    }

    data.loop = g_main_loop_new (NULL, FALSE);

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
    data.source         =   gst_element_factory_make("v4l2src",         "source");
    data.videoconvert   =   gst_element_factory_make("videoconvert",    "videoconvert");
    data.x264enc        =   gst_element_factory_make("x264enc",         "x264enc");
    data.h264parse      =   gst_element_factory_make("h264parse",       "h264parse");
    data.kvssink        =   gst_element_factory_make("kvssink",         "kvssink");

    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new ("PIPELINE");

    if (!data.pipeline || !data.source || !data.videoconvert || !data.x264enc || !data.kvssink) {
        g_print ("Not all elements could be created.\n");
        return -1;
    }


    /* Add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (data.pipeline));
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (bus_call), &data);
    //g_signal_connect (data.fpssink, "fps-measurements", G_CALLBACK(fps_measurements_callback), NULL);

    
    /* Build the pipeline */
    gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.videoconvert, data.x264enc, data.h264parse, data.kvssink, NULL); //fakesink removed from here as it should be null

    // source -> videoconvert -> x264enc -> 264parse -> kvssink


    if (gst_element_link (data.source, data.videoconvert) != TRUE) {
        g_print ("source and videoconvert could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }

    GstCaps *caps1;
    caps1 = gst_caps_from_string("video/x-raw,width=640,height=480,framerate=30/1");
    if (gst_element_link_filtered(data.videoconvert, data.x264enc, caps1) != TRUE) {
        g_print ("Source and x264enc could not be linked\n");
        gst_object_unref (data.pipeline);
        return -1;
    }

    if (gst_element_link (data.x264enc, data.h264parse) != TRUE) {
        g_print ("x264enc and h264parsecould not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }

    if (gst_element_link (data.h264parse, data.kvssink) != TRUE) {
        g_print ("h264parse and kvssink could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }



    /* Modify the source's properties */
    g_object_set(data.source, "device", "/dev/video4", 
        "do-timestamp", true,
        NULL);

    GstStructure *iot_certificate = gst_structure_new_from_string ("iot-certificate,endpoint=crhxlosa5p0oo.credentials.iot.eu-west-1.amazonaws.com,cert-path=/home/soren/projects/gstreamer/desktop/certs/cert.pem,key-path=/home/soren/projects/gstreamer/desktop/certs/privkey.pem,ca-path=/home/soren/projects/gstreamer/desktop/certs/root-CA.pem,role-aliases=fbview-kinesis-video-access-role-alias");


    g_print("About to set kvssink parameters!\n");
    /* Modify the sink's properties */
    g_object_set(data.kvssink, 
        "stream-name", "15e0dc81d12c414aa02b49b990921c8d",
        "framerate", (guint)30,
        "restart-on-error", true,
        "retention-period", 730,
        "aws-region", "eu-west-1",
        "log-config", "./kvs_log_configuration",
        "iot-certificate", iot_certificate,
        NULL);
        
        
    // g_object_set(data.kvssink,
    //     "iot-certificate", "endpoint=crhxlosa5p0oo.credentials.iot.eu-west-1.amazonaws.com",
    //     NULL);
        //cert-path=/certs/cert.pem,key-path=/certs/privkey.pem,ca-path=/certs/root-CA.pem,role-aliases=fbview-kinesis-video-access-role-alias
        //,
        //"iot-certificate", "iot-certificate,endpoint=crhxlosa5p0oo.credentials.iot.eu-west-1.amazonaws.com,cert-path=/certs/cert.pem,key-path=/certs/privkey.pem,
        //ca-path=/certs/root-CA.pem,role-aliases=fbview-kinesis-video-access-role-alias",

    // gchar *stream_name;
    // gint framerate;
    // bool restart_on_error;
    // guint retention_period;
    // gchar *log_config;
    // gchar *iot_certificate;

    // g_object_get(data.kvssink,
    //     "stream-name", &stream_name,
    //     "framerate", &framerate,
    //     "restart-on-error", &restart_on_error,
    //     "retention-period", &retention_period,
    //     "log-config", &log_config,
    //     "iot-certificate", &iot_certificate,
    //     NULL);
    
    // g_print("\nThe values are \nstream name: %s\nFramerate: %i \nRetention-period: %i \nLog-config: %s \niot-certificate: %s\n\n", stream_name, framerate, retention_period, log_config, iot_certificate);

    g_print("Finished setting kvssink parameters!\n");

    g_print("Test program over. Quitting\n");
    return 0;

    /* Start playing */
    ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_print ("Unable to set the pipeline to the playing state.\n");
        //gst_object_unref (data.pipeline);
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