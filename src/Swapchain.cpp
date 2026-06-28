#include "Swapchain.h"

#include <QVulkanFunctions>

#include "VulkanWidget.h"

Swapchain::Swapchain(VulkanWidget* iPVulkanWidget, QVulkanInstance* iPqvulkanInstance, const VkPhysicalDevice* iPphysicalDevice, const VkDevice* iPLogicalDevice)
    : m_pVulkanWidget(iPVulkanWidget)
    , m_pVulkanInstance(iPqvulkanInstance)
    , m_pPhysicalDevice(iPphysicalDevice)
    , m_pLogicalDevice(iPLogicalDevice)
{

}

void Swapchain::createSwapchain(const VkSurfaceKHR& iSurface, VkSurfaceFormatKHR& oSurfaceFormat)
{
    Q_ASSERT(m_pVulkanInstance->isValid());

    // Get SwapChainDetails so we can pick best settings
    m_swapchainDetails = getSwapChainDetails(iSurface);

    // 1. CHOOSE BEST SURFACE FORMAT
    oSurfaceFormat = chooseBestSurfaceFormat(m_swapchainDetails.formats);

    // 2. CHOOSE BEST PRESENTATION MODE
    const VkPresentModeKHR presentMode = chooseBestPresentationMode(m_swapchainDetails.presentationModes);

    // 3. CHOOSE SWAPCHAIN IMAGE RESOLUTION
    m_extent = chooseSwapExtent(m_pVulkanWidget->width(), m_pVulkanWidget->height(), m_swapchainDetails.surfaceCapabilities);

    // Number of images in Swapchain. Get 1 more than the mininum to allow triple buffering
    uint32_t imageCount = m_swapchainDetails.surfaceCapabilities.minImageCount + 1;
    // Clamp down the image count if higher than max
    // maxImageCount = 0 means "no upper limit"
    if (m_swapchainDetails.surfaceCapabilities.maxImageCount) {
        imageCount = std::min(imageCount, m_swapchainDetails.surfaceCapabilities.maxImageCount);
    }

    // Creation information for Swapchain
    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = iSurface;                                                    // Swapchain surface
    swapchainCreateInfo.imageFormat = oSurfaceFormat.format;                                   // Swapchain format
    swapchainCreateInfo.imageColorSpace = oSurfaceFormat.colorSpace;                           // Swapchain color space
    swapchainCreateInfo.presentMode = presentMode;                                              // Swapchain presentation mode
    swapchainCreateInfo.imageExtent = m_extent;                                                 // Swapchain image extents
    swapchainCreateInfo.minImageCount = imageCount;                                             // Swapchain images in swapchain
    swapchainCreateInfo.imageArrayLayers = 1;                                                   // Number of layers for each image in chain
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;                       // What attachment images will be used as
    swapchainCreateInfo.preTransform = m_swapchainDetails.surfaceCapabilities.currentTransform; // Transform to perform on sawp chain
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;                     // How to handle blending images with external graphics (e.g. other windows)
    swapchainCreateInfo.clipped = VK_TRUE;                                                      // Whether to clip parts of images not in view (e.g. behin another window, off screen, etc)

    /*
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else
    */
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    // If old swap cahin been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
    swapchainCreateInfo.oldSwapchain = m_oldSwapchain;

    // Create Swapchain
    auto* vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)(m_pVulkanInstance->getInstanceProcAddr("vkCreateSwapchainKHR"));
    Q_ASSERT(vkCreateSwapchainKHR != nullptr);
    VkResult result = vkCreateSwapchainKHR(*m_pLogicalDevice, &swapchainCreateInfo, nullptr, &m_swapchain);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create a swapchain");
    m_swapchainImages.clear();

    uint32_t swapchainImageCount = 0;
    auto* vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)(m_pVulkanInstance->getInstanceProcAddr("vkGetSwapchainImagesKHR"));
    Q_ASSERT(vkGetSwapchainImagesKHR != nullptr);
    vkGetSwapchainImagesKHR(*m_pLogicalDevice, m_swapchain, &swapchainImageCount, nullptr);
    std::vector<VkImage> images(swapchainImageCount);
    vkGetSwapchainImagesKHR(*m_pLogicalDevice, m_swapchain, &swapchainImageCount, images.data());

    m_swapchainImages.reserve(images.size());

    // Create image view
    for (const VkImage& image : images) {
        // Store image handle
        swapchainImage_t swapchainImage{};
        swapchainImage.image = image;
        swapchainImage.imageView = createImageView(swapchainImage.image, oSurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);

        m_swapchainImages.push_back(swapchainImage);
    }
}

void Swapchain::recreateSwapchain(const VkSurfaceKHR& iSurface, VkSurfaceFormatKHR& oSurfaceFormat)
{
    printDebugInfo("Recreate SwapChain");

    Q_ASSERT(m_pVulkanInstance->isValid());

    if (m_pVulkanWidget->width() == 0 || m_pVulkanWidget->height() == 0) {
        printDebugInfo("Window minimized, skip recreateSwapChain");
        return;
    }

    // Wait until Idle status
    QVulkanDeviceFunctions* pDeviceFunctions = m_pVulkanInstance->deviceFunctions(*m_pLogicalDevice);
    Q_ASSERT(pDeviceFunctions != VK_NULL_HANDLE);

    pDeviceFunctions->vkDeviceWaitIdle(*m_pLogicalDevice);

    // Move current swapchain to old swapchain to recreate
    m_oldSwapchain = m_swapchain;
    m_swapchain    = VK_NULL_HANDLE;

    // Destroy swapchain resources
    destroySwapchainImageViews();

    // Recreate a swapchain
    createSwapchain(iSurface, oSurfaceFormat);

    // Destroy old swapchain
    if (m_oldSwapchain != VK_NULL_HANDLE) {
        auto* vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)m_pVulkanInstance->getInstanceProcAddr("vkDestroySwapchainKHR");
        Q_ASSERT(vkDestroySwapchainKHR != nullptr);
        vkDestroySwapchainKHR(*m_pLogicalDevice, m_oldSwapchain, nullptr);
        m_oldSwapchain = VK_NULL_HANDLE;
    }
}

void Swapchain::destroySwapchain()
{
    Q_ASSERT(m_pVulkanInstance->isValid());

    // Wait until Idle status
    QVulkanDeviceFunctions* pDeviceFunctions = m_pVulkanInstance->deviceFunctions(*m_pLogicalDevice);
    Q_ASSERT(pDeviceFunctions != VK_NULL_HANDLE);

    pDeviceFunctions->vkDeviceWaitIdle(*m_pLogicalDevice);


    destroySwapchainImageViews();

    auto* vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)(m_pVulkanInstance->getInstanceProcAddr("vkDestroySwapchainKHR"));
    Q_ASSERT(vkDestroySwapchainKHR != VK_NULL_HANDLE);

    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(*m_pLogicalDevice, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }

    if (m_oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(*m_pLogicalDevice, m_oldSwapchain, nullptr);
        m_oldSwapchain = VK_NULL_HANDLE;
    }
}

const VkSwapchainKHR& Swapchain::getSwapchain() const
{
    return m_swapchain;
}

std::vector<swapchainImage_t>& Swapchain::getSwapchainImages()
{
    return m_swapchainImages;
}

const size_t Swapchain::getSwapchainImageCount() const
{
    return m_swapchainImages.size();
}

const VkExtent2D Swapchain::getExtent() const
{
    return m_extent;
}

void Swapchain::destroySwapchainImageViews()
{
    Q_ASSERT(m_pVulkanInstance->isValid());

    QVulkanDeviceFunctions* pDeviceFunctions = m_pVulkanInstance->deviceFunctions(*m_pLogicalDevice);
    Q_ASSERT(pDeviceFunctions != nullptr);

    // Destroy the old swapchain
    for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
        if (m_swapchainImages[i].imageView != VK_NULL_HANDLE) {
            pDeviceFunctions->vkDestroyImageView(*m_pLogicalDevice, m_swapchainImages[i].imageView, nullptr);
            m_swapchainImages[i].imageView = VK_NULL_HANDLE;
        }
    }

    m_swapchainImages.clear();
}

const swapchainDetails_t Swapchain::getSwapChainDetails(const VkSurfaceKHR& iSurface)
{
    printDebugInfo("Get Swapchain Details");

    swapchainDetails_t swapchainDetails{};

    // CAPABILITIES
    // Get the surface capabilities for the given surface on the given physical device
    auto* vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)(m_pVulkanInstance->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    Q_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR != nullptr);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*m_pPhysicalDevice, iSurface, &swapchainDetails.surfaceCapabilities);

    // FORMATS
    uint32_t formatCount = 0;
    auto* vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)(m_pVulkanInstance->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceFormatsKHR"));
    Q_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR != nullptr);
    vkGetPhysicalDeviceSurfaceFormatsKHR(*m_pPhysicalDevice, iSurface, &formatCount, nullptr);
    if (formatCount != 0) {
        swapchainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(*m_pPhysicalDevice, iSurface, &formatCount, swapchainDetails.formats.data());
    }

    // PRESENTATION MODES
    uint32_t presentationCount = 0;
    auto* vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)(m_pVulkanInstance->getInstanceProcAddr("vkGetPhysicalDeviceSurfacePresentModesKHR"));
    Q_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR != nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(*m_pPhysicalDevice, iSurface, &presentationCount, nullptr);

    // if presentation modees returned, get list of presentation modes
    if (presentationCount != 0) {
        swapchainDetails.presentationModes.resize(presentationCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(*m_pPhysicalDevice, iSurface, &presentationCount, swapchainDetails.presentationModes.data());
    }

    return swapchainDetails;
}

const VkSurfaceFormatKHR Swapchain::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &iFormats)
{
    // Best format is subjective, but ours will be:
    // Format    : VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM as backup)
    // ColorSpace: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR

    printDebugInfo("Choose best surface format");

    Q_ASSERT(iFormats.size() > 0);

    // If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
    if (iFormats.size() == 1 && iFormats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for (const VkSurfaceFormatKHR& format : iFormats) {
        if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) &&
            (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
            return format;
        }
    }

    // If can't find optimal format, then just return the first format
    return iFormats[0];
}

const VkPresentModeKHR Swapchain::chooseBestPresentationMode(const std::vector<VkPresentModeKHR> &iPresentationModes)
{
    printDebugInfo("Choose best presentaion mode");

    // Look for Mailbox presentation mode
    for (const VkPresentModeKHR& presentationMode : iPresentationModes) {
        if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentationMode;
        }
    }

    // If can't find Mailbox, use FIFO as Vulkan spec says it must be present;
    return VK_PRESENT_MODE_FIFO_KHR;
}

const VkExtent2D Swapchain::chooseSwapExtent(const uint32_t iWidth, const uint32_t iHeight, const VkSurfaceCapabilitiesKHR &iSurfaceCapabilities)
{
    printDebugInfo("Choose Swap Extent");

    // If current extent is at numeric limits, then extent can vary. Otherwise, it's the size of the window.
    if (iSurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        printDebugInfo("Extent width: " + QString::number(iSurfaceCapabilities.currentExtent.width));
        printDebugInfo("Extent height: " + QString::number(iSurfaceCapabilities.currentExtent.height));
        return iSurfaceCapabilities.currentExtent;
    }

    // Create new extent using window size
    VkExtent2D newExtent{};

    // Surface also defines max and min, so make sure within boundaries by clamping value
    newExtent.width = std::max(iSurfaceCapabilities.minImageExtent.width, std::min(iSurfaceCapabilities.maxImageExtent.width, iWidth));
    newExtent.height = std::max(iSurfaceCapabilities.minImageExtent.height, std::min(iSurfaceCapabilities.maxImageExtent.height, iHeight));

    printDebugInfo("New extent width: " + QString::number(newExtent.width));
    printDebugInfo("New extent height: " + QString::number(newExtent.height));

    return newExtent;
}

const VkImageView Swapchain::createImageView(const VkImage iImage, const VkFormat iFormat, const VkImageAspectFlags iAspectFlags)
{
    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = iImage;                                          // Image to vreate view for
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;                        // Type of image (1D, 2D, 3D, Cube, etc)
    imageViewCreateInfo.format = iFormat;                                        // Format of image data
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;            // Allows remapping of rgba components to other rgba values
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Sub-resources allow the view to view only a part of an image
    imageViewCreateInfo.subresourceRange.aspectMask = iAspectFlags;              // Which aspect of image to view (e.g. COLOR_BIT for viewing color)
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;                       // Start mipmap level to view from
    imageViewCreateInfo.subresourceRange.levelCount = 1;                         // Number of mipmap levels to view
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;                     // Start array level to view from
    imageViewCreateInfo.subresourceRange.layerCount = 1;                         // Number of array levels to view

    // Create image view and return it
    VkImageView imageView{};

    QVulkanDeviceFunctions* pDeviceFunctions = m_pVulkanInstance->deviceFunctions(*m_pLogicalDevice);
    Q_ASSERT(pDeviceFunctions != nullptr);
    VkResult result = pDeviceFunctions->vkCreateImageView(*m_pLogicalDevice, &imageViewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create Image view");

    return imageView;
}

void Swapchain::printDebugInfo(const QString &iString) const
{
    emit m_pVulkanWidget->sendDebugInfo(iString);
}

