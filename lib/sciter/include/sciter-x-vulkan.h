#ifndef __SCITER_VULKAN_H__
#define __SCITER_VULKAN_H__

#if defined(USE_VULKAN)

#include <vulkan/vulkan_core.h>
#include "sciter-x-types.h"

typedef struct SciterVulkanEnvironment {
  PFN_vkGetInstanceProcAddr   vkInstProc;
} SciterVulkanEnvironment;

typedef struct SciterVulkanContext {

  uint32_t          vkWidth;  // window client area
  uint32_t          vkHeight; // window client area

  VkInstance        vkInstance;
  VkPhysicalDevice  vkPhysicalDevice;
  VkDevice          vkDevice;

  VkFormat          vkSurfaceFormat; // swap chain format

  VkSurfaceKHR      vkSurface;
  VkSwapchainKHR    vkSwapchain;
  uint32_t          vkGraphicsQueueIndex;
  VkQueue           vkGraphicsQueue;
  uint32_t          vkPresentQueueIndex;
  VkQueue           vkPresentQueue;

  uint32_t          vkImageCount;
  VkImage *      vkImages;       // images in the swapchain
  VkImageLayout *vkImageLayouts; // layouts of these images when not color attachment
  uint32_t          vkCurrentBackbufferIndex;

  uint32_t          vkImageIndex;
  uint32_t          vkSwapchainSerialNo; // increases each time when swap chain gets recreated

} SciterVulkanContext;

/**
 * @brief This class provides a way for the client to call 'VulkanWindowContextShared'
 * functions.  These functions have been implemented by the shared context class.
 */
struct CustomVulkanContext {
  CustomVulkanContext()          = default;
  virtual ~CustomVulkanContext() = default;

  CustomVulkanContext(const CustomVulkanContext &source) = default;
  CustomVulkanContext &operator=(const CustomVulkanContext &source) = default;

  CustomVulkanContext(CustomVulkanContext &&rhs) = default;
  CustomVulkanContext &operator=(CustomVulkanContext &&rhs) = default;

  /**
   * @brief Sets the index of the current active image to render to
   * @param vkImageIndex Current image index to draw to.  This will draw to the
   * SkSurface which holds the underlying image.
   */
  virtual void setImageIndex(const uint32_t vkImageIndex) = 0;

  /**
   * @brief Set the image layout of the image located at vkImageIndex.  This is to
   * ensure Skia handles the image layout correctly.
   * @param vkImageIndex Image index that Skia will be notified of its layout.
   * @param vkImageLayout Image layout of the image at the vkImageIndex location.
   */
  virtual void setImageLayout(const uint32_t vkImageIndex, const VkImageLayout vkImageLayout) = 0;

  /**
   * @brief Set the image index and current layout of the active image to draw to.
   * This is to ensure Skia draws to the correct image and handles the image layout correctly.
   * @param vkImageIndex Image index that Skia will be notified of its layout.
   * @param vkImageLayout Image layout of the image at the vkImageIndex location.
   */
  void setActiveImage(const uint32_t vkImageIndex, const VkImageLayout vkImageLayout) {
    setImageIndex(vkImageIndex);
    setImageLayout(vkImageIndex, vkImageLayout);
  }

  /**
   * @brief When called the existing backend SkSurfaces and textures will be destroyed.  New
   * backend textures and SkSurfaces will be created.  This must be called whenever the
   * client remakes the shared VkImage(s).
   * @param imageWidth Image width
   * @param imageHeight Image height
   */
  virtual void updateImages(const int imageWidth, const int imageHeight) = 0;
};

/**
 * @brief This class provides a way to share handles with the client and Skia.  The
 * client will create the Vulkan context and will share it's handles with Skia.
 * Additionally, the client can implement Vulkan functions that will be
 * called during rendering.
 */
struct SciterWindowVulkanBridge {
  SciterWindowVulkanBridge()          = default;
  virtual ~SciterWindowVulkanBridge() = default;

  SciterWindowVulkanBridge(const SciterWindowVulkanBridge &source) = default;
  SciterWindowVulkanBridge &operator=(const SciterWindowVulkanBridge &source) = default;

  SciterWindowVulkanBridge(SciterWindowVulkanBridge &&rhs) = default;
  SciterWindowVulkanBridge &operator=(SciterWindowVulkanBridge &&rhs) = default;

  /**
   * @brief Initialize the client Vulkan context.  Pass the shared client Vulkan handles to the
   * members within this class.
   * @param width Image width.
   * @param height Image height.
   */
  virtual bool on_vk_initialize(const int width, const int height) = 0;

  /**
   * @brief Executes at the start of each loop if the window is resized.  The
   * client needs to handle the window resize here.  The client only needs
   * to handle their images being resized.  On the backend, on_vk_image_change()
   * will be called to update Skia's images.
   * @param width Image width.
   * @param height Image height.
   */
  virtual bool on_vk_size_change(int width, int height) = 0;

  /**
   * @brief Code to execute whenever the shared images that Skia is drawing
   * to change.  The following must be updated:
   * vkImageCount, vkImages, vkSurfaceFormat, vkImageUsageFlags, vkImageSharingMode.
   * Afterwords, CustomVulkanContext::updateImages(w, h) must be called.
   */
  virtual bool on_vk_image_change() = 0;

  /**
   * @brief Code that starts the rendering process.  If that client wants to draw to the shared
   * image before Skia, they can do so here.  If swapchain images are being used, they would be
   * acquired and drawn to here.  However, it is not a requirement to use swapchain images.
   * This function will execute before Skia performs any drawing to the shared image.
   */
  virtual bool on_vk_render_start() = 0;

  /**
   * @brief Code to execute at the end of the rendering process.  This most likely involves the
   * client presenting the final image.  Skia will not present when the custom context is used.
   */
  virtual bool on_vk_render_end() = 0;

  /**
   * @brief Set the custom vulkan context here.  This allows the 'frame' class to call functions
   * in 'CustomVulkanContext' which have been implemented by the VulkanWindowContextShared class.
   * @param p pointer to the custom Vulkan context
   */
  virtual void set_custom_vulkan_context(CustomVulkanContext *p) = 0;

  VkInstance vkInstance{};    /*!< Vulkan instance */
  uint32_t   vkInstanceAPI{}; /*!< Vulkan instance version */

  VkPhysicalDeviceFeatures  vkDeviceFeatures{};          /*!< Activated physical device features */
  VkPhysicalDeviceFeatures2 vkDeviceFeatures2{};         /*!< Extended activated device features */
  bool                      vkUseDeviceFeatures2{false}; /*!< Boolean if vkDeviceFeatures2 is used */

  VkPhysicalDevice vkPhysicalDevice{};     /*!< Vulkan physical device */
  VkDevice         vkDevice{};             /*!< Vulkan logical device */
  VkQueue          vkGraphicsQueue{};      /*!< Vulkan graphics queue handle */
  uint32_t         vkGraphicsQueueIndex{}; /*!< Vulkan graphics queue index */

  uint32_t          vkImageCount{};       /*!< Number of images created by the client */
  VkImage *         vkImages{};           /*!< Images created by the client for Skia to render to */
  VkFormat          vkImageFormat{};      /*!< Format of the image being rendered to */
  VkSharingMode     vkImageSharingMode{}; /*!< Image sharing mode: Concurrent or exclusive */
  VkImageUsageFlags vkImageUsageFlags{};  /*!< Image usage flags.  At a minimum, this must be: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT*/

  std::vector<const char *> vkInstanceExtensions{}; /*!< Loaded instance extensions */
  std::vector<const char *> vkDeviceExtensions{};   /*!< Loaded device extensions */

  std::function<PFN_vkVoidFunction(const char *, VkInstance, VkDevice)> vkGetProc{}; /*!< Function to load Vulkan function pointers */

  bool          vkTransitionReturnLayout{false}; /*!< Set to true if Skia should transition the image layout when its finished drawing */
  VkImageLayout vkReturnImageLayout{};           /*!< Layout Skia should transition the image to */
};

#endif

#endif
