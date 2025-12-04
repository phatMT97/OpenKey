
#include "sciter-x.h"
#include "sciter-x-behavior.h"
#include "sciter-x-graphics.hpp"

#ifdef USE_VULKAN

#include "sciter-x-vulkan.h"

namespace sciter
{

// "Vulkan Area" - controller of vulkan rendering DOM element 

  struct vulkanarea: public event_handler
  {
    HWINDOW  hwnd;
    HELEMENT self;
    SciterVulkanEnvironment vkenv = {0}; 

    // ctor
    vulkanarea() {}
    virtual ~vulkanarea() {}

    virtual bool subscription( HELEMENT he, UINT& event_groups )
    {
      event_groups = HANDLE_ALL;   // it does drawing
      return true;
    }

    virtual void attached  (HELEMENT he ) 
    {
      self = he;
      hwnd = dom::element(he).get_element_hwnd(true);
      SciterWindowExec(hwnd, SCITER_WINDOW_GET_VULKAN_ENVIRONMENT, UINT_PTR(&vkenv), sizeof(vkenv));
      SciterVulkanContext vkctx = {0};
      SciterWindowExec(hwnd, SCITER_WINDOW_GET_VULKAN_CONTEXT, UINT_PTR(&vkctx), sizeof(vkctx));      
      initialize(vkctx);
    }

    virtual void detached  (HELEMENT he ) { 
      SciterVulkanContext vkctx = {0};
      SciterWindowExec(hwnd, SCITER_WINDOW_GET_VULKAN_CONTEXT, UINT_PTR(&vkctx), sizeof(vkctx));      
      shutdown(vkctx);
      asset_release(); 
    }

    virtual bool handle_draw   (HELEMENT he, DRAW_PARAMS& params ) 
    {
      if( params.cmd != DRAW_CONTENT ) 
        return false; // drawing only content layer
      SciterVulkanContext vkctx = {0};
      SciterWindowExec(hwnd, SCITER_WINDOW_GET_VULKAN_CONTEXT, UINT_PTR(&vkctx), sizeof(vkctx));      
      return render(vkctx); // done drawing
    }

    // overridables  

    virtual void initialize( const SciterVulkanContext& vkctx ) {}
    virtual void shutdown( const SciterVulkanContext& vkctx ) {}
    virtual bool render(const SciterVulkanContext& vkctx) { return false; }

    
    // script API
    /*
    SOM_PASSPORT_BEGIN_EX(molehill,molehill_opengl)
      //SOM_FUNCS()
      SOM_PROPS(
        SOM_PROP(theta),
        SOM_PROP(color1),
        SOM_PROP(color2),
        SOM_PROP(color3)
      )
    SOM_PASSPORT_END
    */
  };


  struct vulkanarea_factory: public behavior_factory {

    vulkanarea_factory(): behavior_factory("vulkanarea") { }

    // the only behavior_factory method:
    virtual event_handler* create(HELEMENT he) {
      return new vulkanarea();
    }

  };

  // instantiating and attaching it to the global list
  vulkanarea_factory vulkanarea_factory_instance;

}

#endif
