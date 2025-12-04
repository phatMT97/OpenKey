//#include "stdafx.h"
#include "sciter-x.h"
#include "sciter-x-behavior.h"

#include <gtk/gtk.h>

using namespace std;

namespace sciter
{
/*
BEHAVIOR: native-clock
  - draws content layer using sciter-x-graphics.hpp primitives.

SAMPLE:
   See: samples/behaviors/native-drawing.htm
*/

struct native_textarea: public event_handler
{
  GtkWidget* this_widget;
  HELEMENT   this_element;
    // ctor
  native_textarea(): this_widget(nullptr), this_element(0) {}
    virtual ~native_textarea() {}

    virtual bool subscription( HELEMENT he, UINT& event_groups )
    {
      event_groups = HANDLE_BEHAVIOR_EVENT | HANDLE_METHOD_CALL;
      return true;
    }

    virtual void attached  (HELEMENT he )
    {
      this_element = he;

      dom::element self = this_element;

      RECT rc = self.get_location(VIEW_RELATIVE | CONTENT_BOX);

      GtkWidget* parent = self.get_element_hwnd(false);

      this_widget = gtk_text_view_new ();
      gtk_container_add(GTK_CONTAINER(parent), GTK_WIDGET(this_widget));
      gtk_widget_show(this_widget);
      self.attach_hwnd(this_widget);
    }
    virtual void detached  (HELEMENT he )
    {
      if (this_widget) {
        gtk_widget_destroy(this_widget);
      }
      asset_release();
    }

    virtual bool handle_method_call(HELEMENT he, METHOD_PARAMS& params) {
      switch (params.methodID)
      {
        case GET_VALUE:
        {
          VALUE_PARAMS* pvp = (VALUE_PARAMS*)(&params);
          GtkTextIter start = {0};
          GtkTextIter end = {0};
          GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (this_widget));
          gtk_text_buffer_get_start_iter( buffer, &start );
          gtk_text_buffer_get_end_iter( buffer, &end );
          gchar *u8 = gtk_text_buffer_get_text( buffer, &start, & end, FALSE );
          pvp->val = sciter::value(aux::utf2w(aux::chars_of(u8)).chars());
          g_free(u8);
          return true;
        }
        case SET_VALUE: {
          VALUE_PARAMS* pvp = (VALUE_PARAMS*)(&params);
          sciter::string text = pvp->val.to_string();
          GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (this_widget));
          aux::w2utf u8(text);
          gtk_text_buffer_set_text (buffer, u8.c_str(), u8.length());
          return true;
        }
      }
      return false;
    }

/*    BEGIN_FUNCTION_MAP
      FUNCTION_6("nativeGetPath", nativeGetPath);
      FUNCTION_2("nativeImage", nativeImage);
    END_FUNCTION_MAP */

};

struct native_textarea_factory: public behavior_factory {

  native_textarea_factory(): behavior_factory("native-textarea") { }

  // the only behavior_factory method:
  virtual event_handler* create(HELEMENT he) {
    return new native_textarea();
  }

};

// instantiating and attaching it to the global list
native_textarea_factory native_textarea_factory_instance;

}
