#include <gst/gst.h>

int
main (int argc, char *argv[])
{
  GstElement *pipeline;
  GstMessage *msg;
  GstBus *bus;
  GError *error = NULL;

  gst_init (&argc, &argv);


  pipeline = gst_parse_launch ("v4l2src do-timestamp=TRUE device=/dev/video4 \
      ! videoconvert \
      ! videoscale \
      ! video/x-raw,width=640,height=480 \
      ! fpsdisplaysink", &error);


  if (!pipeline) {
    g_print ("Parse error: %s\n", error->message);
    exit (1);
  }

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  bus = gst_element_get_bus (pipeline);

  /* wait until we either get an EOS or an ERROR message. Note that in a real
   * program you would probably not use gst_bus_poll(), but rather set up an
   * async signal watch on the bus and run a main loop and connect to the
   * bus's signals to catch certain messages or all messages */
  msg = gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, -1);

  switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_EOS: {
        g_print ("EOS\n");
        break;
        }
        case GST_MESSAGE_ERROR: {
        GError *err = NULL; /* error to show to users                 */
        gchar *dbg = NULL;  /* additional debug string for developers */

        gst_message_parse_error (msg, &err, &dbg);
        if (err) {
            g_printerr ("ERROR: %s\n", err->message);
            g_error_free (err);
        }
        if (dbg) {
            g_printerr ("[Debug details: %s]\n", dbg);
            g_free (dbg);
        }
        }
        default:
        g_printerr ("Unexpected message of type %d", GST_MESSAGE_TYPE (msg));
        break;
  }
  gst_message_unref (msg);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  gst_object_unref (bus);

  return 0;
}