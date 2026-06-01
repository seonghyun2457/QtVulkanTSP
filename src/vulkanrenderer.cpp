#include "vulkanrenderer.h"

#include "vulkanwidget.h"

#include <QtAssert>
#include <QVariant>

#include <exception>
#include <QDebug>

#include <algorithm>

VulkanRenderer::VulkanRenderer(VulkanWidget* window)
    : m_window(window)
{

}

VulkanRenderer::~VulkanRenderer()
{
    qDebug() << "destroy VulkanRenderer";
}

bool VulkanRenderer::initialize()
{
    bool succeded = true;
    try {
        createInstance();
        createSurface();
        selectPhysicalDevice();
        createLogicalDevice();
        createCommandPools();
        createSwapChain();
        createDescriptorSetLayout();


        createSynchronization();
    } catch (const std::runtime_error& e) {
        printDebugInfo(e.what());
        succeded = false;
    }

    return succeded;
}

void VulkanRenderer::cleanup()
{
    Q_ASSERT(m_pDeviceFunctions != VK_NULL_HANDLE);

    // Wait until Idle status
    m_pDeviceFunctions->vkDeviceWaitIdle(m_logicalDevice);

    // Destroy descriptor set layout
    {
        auto* vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)(m_vulkanInstance.getInstanceProcAddr("vkDestroyDescriptorSetLayout"));
        Q_ASSERT(vkDestroyDescriptorSetLayout != nullptr);
        if (m_descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_logicalDevice, m_descriptorSetLayout, nullptr);
            m_descriptorSetLayout = VK_NULL_HANDLE;
        }
    }

    // Destroy command pools
    {
        std::set<VkCommandPool> uniqueCommandPools;
        if (m_graphicsCommandPool != VK_NULL_HANDLE) uniqueCommandPools.insert(m_graphicsCommandPool);
        if (m_computeCommandPool != VK_NULL_HANDLE) uniqueCommandPools.insert(m_computeCommandPool);
        if (m_transferCommandPool != VK_NULL_HANDLE) uniqueCommandPools.insert(m_transferCommandPool);

        for (VkCommandPool commandPool : uniqueCommandPools) {
            m_pDeviceFunctions->vkDestroyCommandPool(m_logicalDevice, commandPool, nullptr);
        }

        m_graphicsCommandPool = VK_NULL_HANDLE;
        m_computeCommandPool = VK_NULL_HANDLE;
        m_transferCommandPool = VK_NULL_HANDLE;
    }

    // Destroy Semaphores and Fences
    {
        auto* vkDestroySemaphore = (PFN_vkDestroySemaphore)(m_vulkanInstance.getInstanceProcAddr("vkDestroySemaphore"));
        Q_ASSERT(vkDestroySemaphore != nullptr);

        auto* vkDestroyFence = (PFN_vkDestroyFence)(m_vulkanInstance.getInstanceProcAddr("vkDestroyFence"));
        Q_ASSERT(vkDestroyFence != nullptr);

        for (size_t i = 0; i < m_imagesAvailable.size(); ++i) {
            vkDestroySemaphore(m_logicalDevice, m_imagesAvailable[i], nullptr);
        }

        for (size_t i = 0; i < m_renderFinished.size(); ++i) {
            vkDestroySemaphore(m_logicalDevice, m_renderFinished[i], nullptr);
        }

        for (size_t i = 0; i < m_fences.size(); ++i) {
            vkDestroyFence(m_logicalDevice, m_fences[i], nullptr);
        }
    }

    // Destroy SwapChain
    {
        // Destroy Image view
        destroySwapChainResources();

        auto* vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)(m_vulkanInstance.getInstanceProcAddr("vkDestroySwapchainKHR"));
        Q_ASSERT(vkDestroySwapchainKHR != nullptr);

        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);
            m_swapchain = VK_NULL_HANDLE;
        }

        if (m_oldSwapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_logicalDevice, m_oldSwapchain, nullptr);
            m_oldSwapchain = VK_NULL_HANDLE;
        }
    }

    // Destroy logical device
    {
        m_pDeviceFunctions->vkDestroyDevice(m_logicalDevice, nullptr);
        m_pDeviceFunctions = VK_NULL_HANDLE;
    }

    // DO NOT destroy m_surface here: — it is owned by Qt's platform window.
    // DO NOT destroy m_vulkanInstance here: QVulkanInstance's destructor handles it;
    // calling it here causes a double-destroy when the member is later destructed.
    qDebug() << "cleaned up";
}

void VulkanRenderer::recreateSwapChain()
{
    printDebugInfo("Recreate SwapChain");

    if (m_window->width() == 0 || m_window->height() == 0) {
        printDebugInfo("Window minimized, skip recreateSwapChain");
        return;
    }

    // Move current swapchain to old swapchain to recreate
    m_oldSwapchain = m_swapchain;
    m_swapchain    = VK_NULL_HANDLE;

    // Destroy swapchain resources
    destroySwapChainResources();

    // Recreate a swapchain
    createSwapChain();

    // Destroy old swapchain
    if (m_oldSwapchain != VK_NULL_HANDLE) {
        auto* vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)m_vulkanInstance.getInstanceProcAddr("vkDestroySwapchainKHR");
        Q_ASSERT(vkDestroySwapchainKHR != nullptr);
        vkDestroySwapchainKHR(m_logicalDevice, m_oldSwapchain, nullptr);
        m_oldSwapchain = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::createInstance()
{
    printDebugInfo("Create Instance");

    m_vulkanInstance.setApiVersion(m_vulkanInstance.supportedApiVersion());
    m_vulkanInstance.setLayers({ "VK_LAYER_KHRONOS_validation" });

    if (!m_vulkanInstance.create()) {
        qCritical() << "Failed to create QVulkanInstance, error code:" << m_vulkanInstance.errorCode();
        return;
    }

    printVulkanInfo("Extensions:");
    for (const auto& extension : m_vulkanInstance.extensions()) {
        printVulkanInfo(extension);
    }

    printVulkanInfo("Vulkan api version: " + m_vulkanInstance.apiVersion().toString());

    m_window->setVulkanInstance(&m_vulkanInstance);

    m_pFunctions = m_vulkanInstance.functions();
    Q_ASSERT(m_pFunctions != VK_NULL_HANDLE);
}

void VulkanRenderer::createSurface()
{
    printDebugInfo("Create Surface");

    // Get VkSurfaceKHR info from QWindow
    m_surface = m_vulkanInstance.surfaceForWindow(m_window);
}

void VulkanRenderer::selectPhysicalDevice()
{
    printDebugInfo("Select physical device");

    Q_ASSERT(m_pFunctions != VK_NULL_HANDLE);

    uint32_t deviceCount = 0;
    m_pFunctions->vkEnumeratePhysicalDevices(m_vulkanInstance.vkInstance(), &deviceCount, nullptr);

    // If no device available, then none support Vulkan!
    if (deviceCount == 0) throw std::runtime_error("Can't find GPUs that support Vulkan Instance!");

    // Get list of physical devices
    std::vector<VkPhysicalDevice> gpuDeviceList(deviceCount);
    m_pFunctions->vkEnumeratePhysicalDevices(m_vulkanInstance.vkInstance(), &deviceCount, gpuDeviceList.data());

    // Pick suitable physical device
    // Always select first index device
    uint32_t selectedGPUIndex = 0;
    m_physicalDevice = gpuDeviceList[selectedGPUIndex];

    // Get Properties of our new physical device
    m_pFunctions->vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties);
    printVulkanInfo("GPU name: " + QString(m_physicalDeviceProperties.deviceName));

    // Get Physical device features
    m_pFunctions->vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_physicalDeviceFeatures);
    printVulkanInfo("Physical device features: ");
    printVulkanInfo("geometryShader: " + QVariant(m_physicalDeviceFeatures.geometryShader).toString());
    printVulkanInfo("tessellationShader: " + QVariant(m_physicalDeviceFeatures.tessellationShader).toString());

    // Get Physical device memory properties
    m_pFunctions->vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_physicalDeviceMemoryProperties);

    // Find Queue Family
    printDebugInfo("Get queue familiy");

    uint32_t queueFamilyCount = 0;
    m_pFunctions->vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
    Q_ASSERT(queueFamilyCount > 0);

    m_queueFamilyProperties.resize(queueFamilyCount);
    m_pFunctions->vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, m_queueFamilyProperties.data());

    printVulkanInfo("Number of Queue Family Properties: " + QString::number(queueFamilyCount));
    for (size_t i = 0; i < m_queueFamilyProperties.size(); ++i) {
        QString queueFamily = "Queue Familiy " + QString::number(i) + ": " + QString::number(m_queueFamilyProperties[i].queueCount) + ", flags: ";
        if (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamily += "VK_QUEUE_GRAPHICS_BIT";
        } else if (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueFamily += "VK_QUEUE_COMPUTE_BIT";
        } else if (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            queueFamily += "VK_QUEUE_TRANSFER_BIT";
        } else if (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
            queueFamily += "VK_QUEUE_SPARSE_BINDING_BIT";
        } else if (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_PROTECTED_BIT) {
            queueFamily += "VK_QUEUE_PROTECTED_BIT";
        } else if (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) {
            queueFamily += "VK_QUEUE_VIDEO_DECODE_BIT_KHR";
        } else if (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) {
            queueFamily += "VK_QUEUE_VIDEO_ENCODE_BIT_KHR";
        } else if (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) {
            queueFamily += "VK_QUEUE_OPTICAL_FLOW_BIT_NV";
        }

        printVulkanInfo(queueFamily);
    }

    // Extensions
    {
        uint32_t extensionCount = 0;
        m_pFunctions->vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);
        if (extensionCount > 0) {
            std::vector<VkExtensionProperties> extensions(extensionCount);
            m_pFunctions->vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, extensions.data());
            for (const VkExtensionProperties& extension : extensions) {
                m_supportedExtensions.push_back(extension.extensionName);
            }
        }
    }

    // Check if Graphics queue supports Presentation queue
    {
        const uint32_t graphicsQueueIndex = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
        VkBool32 presentSupport = VK_FALSE;

        auto* vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)m_vulkanInstance.getInstanceProcAddr("vkGetPhysicalDeviceSurfaceSupportKHR");
        Q_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR != nullptr);
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, graphicsQueueIndex, m_surface, &presentSupport);

        if (!presentSupport) throw std::runtime_error("Graphics queue family does not support presentation!");

        printDebugInfo("Graphics queue supports presentation: OK (index: " + QString::number(graphicsQueueIndex) + ")");
    }
}

void VulkanRenderer::createLogicalDevice()
{
    printDebugInfo("Create logical device");

    const VkQueueFlags requestedQueueTypes = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;

    VkPhysicalDeviceVulkan13Features enabledFeatures13{};
    enabledFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    enabledFeatures13.dynamicRendering = VK_TRUE; // Enable Dynamic Rendering
    enabledFeatures13.synchronization2 = VK_TRUE;

    // QUEUE FAMILIY
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    const float defaultQueueProiority = 0.f;

    // Graphics queue
    if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
        m_queueFaimilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);

        VkDeviceQueueCreateInfo graphicQueueInfo{};
        graphicQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        graphicQueueInfo.queueFamilyIndex = m_queueFaimilyIndices.graphics;
        graphicQueueInfo.queueCount = 1;
        graphicQueueInfo.pQueuePriorities = &defaultQueueProiority;

        queueCreateInfos.push_back(graphicQueueInfo);
    } else {
        m_queueFaimilyIndices.graphics = 0;
    }

    // Compute queue
    if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT) {
        m_queueFaimilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
        if (m_queueFaimilyIndices.compute != m_queueFaimilyIndices.graphics) {

            VkDeviceQueueCreateInfo computeQueueInfo{};
            computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            computeQueueInfo.queueFamilyIndex = m_queueFaimilyIndices.compute;
            computeQueueInfo.queueCount = 1;
            computeQueueInfo.pQueuePriorities = &defaultQueueProiority;

            queueCreateInfos.push_back(computeQueueInfo);
        }
    } else {
        m_queueFaimilyIndices.compute = m_queueFaimilyIndices.graphics;
    }

    // Transfer queue
    if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT) {
        m_queueFaimilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);

        if ((m_queueFaimilyIndices.transfer != m_queueFaimilyIndices.graphics) &&
            (m_queueFaimilyIndices.transfer != m_queueFaimilyIndices.compute)) {

            VkDeviceQueueCreateInfo transferQueueInfo{};
            transferQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            transferQueueInfo.queueFamilyIndex = m_queueFaimilyIndices.transfer;
            transferQueueInfo.queueCount = 1;
            transferQueueInfo.pQueuePriorities = &defaultQueueProiority;

            queueCreateInfos.push_back(transferQueueInfo);
        }
    } else {
        m_queueFaimilyIndices.transfer = m_queueFaimilyIndices.graphics;
    }

    // Enable features
    VkPhysicalDeviceFeatures enabledFeatures{};
    enabledFeatures.samplerAnisotropy = m_physicalDeviceFeatures.samplerAnisotropy;
    /*
    enabledFeatures.depthBiasClamp = m_physicalDeviceFeatures.depthBiasClamp;
    enabledFeatures.depthClamp = m_physicalDeviceFeatures.depthClamp;
    */

    // Check if the physical device supports extensions
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    for (const char* extension : deviceExtensions) {
        if (!extensionSupported(extension)) {
            printDebugInfo("Enabled device extension " + QString(extension) + " isn't present at device level.");
        }
    }

    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
    physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physicalDeviceFeatures2.features = enabledFeatures;
    physicalDeviceFeatures2.pNext = &enabledFeatures13;

    // Information to create logical device
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = nullptr;
    deviceCreateInfo.pNext = &physicalDeviceFeatures2;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Create the logical device for the physical device
    VkResult result =  m_pFunctions->vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_logicalDevice);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create a logical device");

    m_pDeviceFunctions = m_vulkanInstance.deviceFunctions(m_logicalDevice);

    Q_ASSERT(m_pDeviceFunctions != VK_NULL_HANDLE);

    // Queues are created at the same time as the logical device
    createQueues();
}

void VulkanRenderer::createQueues()
{
    printDebugInfo("Create Queues");

    // Validate queue family indices before getting queues
    if (m_queueFaimilyIndices.graphics == -1) {
        throw std::runtime_error("Graphics queue family index is invalid");
    }

    // From given logical device, of given Queue Family, of given Queue Index (0 since only one queue), place reference in given VkQueue
    m_pDeviceFunctions->vkGetDeviceQueue(m_logicalDevice, m_queueFaimilyIndices.graphics, 0, &m_graphicsQueue);

    // For compute queue: either get from dedicated family or use graphics queue
    if ((m_queueFaimilyIndices.compute != m_queueFaimilyIndices.graphics) && (m_queueFaimilyIndices.compute != -1)) {
        m_pDeviceFunctions->vkGetDeviceQueue(m_logicalDevice, m_queueFaimilyIndices.compute, 0, &m_computeQueue);
    } else {
        m_computeQueue = m_graphicsQueue;
    }

    // For transfer queue: either get from dedicated family or use appropriate fallback
    if ((m_queueFaimilyIndices.transfer != m_queueFaimilyIndices.graphics) && (m_queueFaimilyIndices.transfer != m_queueFaimilyIndices.compute) && (m_queueFaimilyIndices.transfer != -1)) {
        m_pDeviceFunctions->vkGetDeviceQueue(m_logicalDevice, m_queueFaimilyIndices.transfer, 0, &m_transferQueue);
    } else if (m_queueFaimilyIndices.transfer == m_queueFaimilyIndices.compute) {
        m_transferQueue = m_computeQueue;
    } else {
        m_transferQueue = m_graphicsQueue;
    }

    // Validate all queues were obtained
    if (m_graphicsQueue == VK_NULL_HANDLE) throw std::runtime_error("Failed to get Graphic queue");
    if (m_computeQueue == VK_NULL_HANDLE) throw std::runtime_error("Failed to get Compute queue");
    if (m_transferQueue == VK_NULL_HANDLE) throw std::runtime_error("Failed to get Transfer queue");
}

void VulkanRenderer::createCommandPools()
{
    printDebugInfo("Create Command Pools");

    // Graphics command pool
    m_graphicsCommandPool = createCommandPool(m_queueFaimilyIndices.graphics);
    printDebugInfo("Created Graphics Command Pool");

    // Compute command pool
    if (m_queueFaimilyIndices.compute != m_queueFaimilyIndices.graphics) {
        m_computeCommandPool = createCommandPool(m_queueFaimilyIndices.compute);
        printDebugInfo("Created Compute Command Pool");
    } else {
        m_computeCommandPool = m_graphicsCommandPool;
    }

    // Transfer command pool
    if (m_queueFaimilyIndices.transfer != m_queueFaimilyIndices.graphics && m_queueFaimilyIndices.transfer != m_queueFaimilyIndices.compute) {
        m_transferCommandPool = createCommandPool(m_queueFaimilyIndices.transfer);
        printDebugInfo("Created Transfer Command Pool");
    } else if (m_queueFaimilyIndices.transfer != m_queueFaimilyIndices.compute) {
        m_transferCommandPool = m_computeCommandPool;
    } else {
        m_transferCommandPool = m_graphicsCommandPool;
    }
}

VkCommandPool VulkanRenderer::createCommandPool(const uint32_t iQueueFamilyIndex)
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = iQueueFamilyIndex;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool commandPool;
    VkResult result = m_pDeviceFunctions->vkCreateCommandPool(m_logicalDevice, &createInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create a Command Pool");

    return commandPool;
}

void VulkanRenderer::createSwapChain()
{
    printDebugInfo("Create SwapChain");

    // Get SwapChainDetails so we can pick best settings
    m_swapchainDetails = getSwapChainDetails(m_physicalDevice);

    // 1. CHOOSE BEST SURFACE FORMAT
    m_surfaceFormat = chooseBestSurfaceFormat(m_swapchainDetails.formats);

    // 2. CHOOSE BEST PRESENTATION MODE
    const VkPresentModeKHR presentMode = chooseBestPresentationMode(m_swapchainDetails.presentationModes);

    // 3. CHOOSE SWAPCHAIN IMAGE RESOLUTION
    m_extent = chooseSwapExtent(m_swapchainDetails.surfaceCapabilities);

    // Number of images in Swapchain. Get 1 more than the mininum to allow triple buffering
    uint32_t imageCount = m_swapchainDetails.surfaceCapabilities.minImageCount + 1;
    // Clamp down the image count if higher than max
    imageCount = std::min(imageCount, m_swapchainDetails.surfaceCapabilities.maxImageCount);
    printDebugInfo("Number of swapchain images: " + QString::number(imageCount));

    // Creation information for Swapchain
    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = m_surface;                                                    // Swapchain surface
    swapchainCreateInfo.imageFormat = m_surfaceFormat.format;                                   // Swapchain format
    swapchainCreateInfo.imageColorSpace = m_surfaceFormat.colorSpace;                           // Swapchain color space
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
    auto* vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)(m_vulkanInstance.getInstanceProcAddr("vkCreateSwapchainKHR"));
    Q_ASSERT(vkCreateSwapchainKHR != nullptr);
    VkResult result = vkCreateSwapchainKHR(m_logicalDevice, &swapchainCreateInfo, nullptr, &m_swapchain);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create a swapchain");
    m_swapchainImages.clear();

    uint32_t swapchainImageCount = 0;
    auto* vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)(m_vulkanInstance.getInstanceProcAddr("vkGetSwapchainImagesKHR"));
    Q_ASSERT(vkGetSwapchainImagesKHR != nullptr);
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &swapchainImageCount, nullptr);
    std::vector<VkImage> images(swapchainImageCount);
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &swapchainImageCount, images.data());

    m_swapchainImages.reserve(images.size());

    // Create image view
    for (const VkImage& image : images) {
        // Store image handle
        SwapchainImage_t swapchainImage{};
        swapchainImage.image = image;
        swapchainImage.imageView = createImageView(swapchainImage.image, m_surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);

        m_swapchainImages.push_back(swapchainImage);
    }
}

void VulkanRenderer::createDescriptorSetLayout()
{
    printDebugInfo("Create Descriptor Set Layout");

    // CREATE UNIFORM BUFFER DESCRIPTOR SET LAYOUT
    // uboModelViewProjection binding info
    VkDescriptorSetLayoutBinding uboModelViewProjectionLayoutBinding{};
    uboModelViewProjectionLayoutBinding.binding = 0;                                         // Binding point in shader (designated by binding number in shader)
    uboModelViewProjectionLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // Type of descriptor (uniform buffer, image sampler, etc)
    uboModelViewProjectionLayoutBinding.descriptorCount = 1;								    // Number of descriptors for binding, only 1 in this case, but could be more with arrays of descriptors in shader
    uboModelViewProjectionLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;			    // Shader stage to bind descriptor to (vertex shader in this case since uboViewProjection matrix is used in vertex shader)
    uboModelViewProjectionLayoutBinding.pImmutableSamplers = nullptr;						// For image sampling, can specify if sampler is immutable (can't be changed) and give a sampler, but not used in this case

    // std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { uboViewProjectionLayoutBinding, uboModelLayoutBinding };
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { uboModelViewProjectionLayoutBinding };

    // Create Descriptor Set Layout with given bindings
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size()); // Number of bindings in the layout
    descriptorSetLayoutCreateInfo.pBindings = layoutBindings.data();                           // List of bindings (only one in this case)
    descriptorSetLayoutCreateInfo.flags = 0;                                                   // Optional flags for the layout (e.g. update after bind pool)
    descriptorSetLayoutCreateInfo.pNext = nullptr;                                             // Optional pointer to extend functionality of layout creation (e.g. for push descriptors)

    // Create Descriptor Set Layout
    Q_ASSERT(m_pDeviceFunctions != nullptr);
    VkResult result = m_pDeviceFunctions->vkCreateDescriptorSetLayout(m_logicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &m_descriptorSetLayout);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create Descriptor Set Layout");
}

void VulkanRenderer::createSynchronization()
{
    printDebugInfo("Create Synchronization");

    Q_ASSERT(m_pDeviceFunctions!= nullptr);

    m_imagesAvailable.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    m_renderFinished.resize(MAX_FRAMES_IN_FLIGHT,  VK_NULL_HANDLE);
    m_fences.resize(MAX_FRAMES_IN_FLIGHT,          VK_NULL_HANDLE);

    // Semaphore creation information
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Fence creation information
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_fences.size(); ++i) {
        if ((m_pDeviceFunctions->vkCreateFence(m_logicalDevice, &fenceCreateInfo, nullptr, &m_fences[i])) != VK_SUCCESS) {
            throw std::runtime_error("Failed to fences");
        }
    }

    for (size_t i = 0; i < m_imagesAvailable.size(); ++i) {
        if ((m_pDeviceFunctions->vkCreateSemaphore(m_logicalDevice, &semaphoreCreateInfo, nullptr, &m_imagesAvailable[i])) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semamphores");
        }
    }

    for (size_t i = 0; i < m_renderFinished.size(); ++i) {
        if ((m_pDeviceFunctions->vkCreateSemaphore(m_logicalDevice, &semaphoreCreateInfo, nullptr, &m_renderFinished[i])) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semamphores");
        }
    }
}

void VulkanRenderer::printVulkanInfo(const QString &iString) const
{
    emit m_window->sendVulkanInfo(iString);
}

void VulkanRenderer::printDebugInfo(const QString &iString) const
{
    emit m_window->sendDebugInfo(iString);
}

const uint32_t VulkanRenderer::getQueueFamilyIndex(const VkQueueFlags iQueueFlags) const
{
    if ((iQueueFlags & VK_QUEUE_COMPUTE_BIT) == iQueueFlags) {
        for (size_t i = 0; i < m_queueFamilyProperties.size(); ++i) {
            if ((m_queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                ((m_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
                return i;
            }
        }
    }

    if ((iQueueFlags & VK_QUEUE_TRANSFER_BIT) == iQueueFlags) {
        for (size_t i = 0; i < m_queueFamilyProperties.size(); ++i) {
            if ((m_queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                ((m_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
                ((m_queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
                return i;
            }
        }
    }

    for (size_t i = 0; i < m_queueFamilyProperties.size(); ++i) {
        if ((m_queueFamilyProperties[i].queueFlags & iQueueFlags) == iQueueFlags) {
            return i;
        }
    }

    printDebugInfo("Couldn't find a queue family that supports the requested queue flags: " + QString::number(iQueueFlags));

    return -1;
}

const SwapChainDetails_t VulkanRenderer::getSwapChainDetails(const VkPhysicalDevice &iDevice)
{
    printDebugInfo("Get Swapchain Details");

    SwapChainDetails_t swapchainDetails{};

    // CAPABILITIES
    // Get the surface capabilities for the given surface on the given physical device
    auto* vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)(m_vulkanInstance.getInstanceProcAddr("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    Q_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR != nullptr);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &swapchainDetails.surfaceCapabilities);

    // FORMATS
    uint32_t formatCount = 0;
    auto* vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)(m_vulkanInstance.getInstanceProcAddr("vkGetPhysicalDeviceSurfaceFormatsKHR"));
    Q_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR != nullptr);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    if (formatCount != 0) {
        swapchainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, swapchainDetails.formats.data());
    }

    // PRESENTATION MODES
    uint32_t presentationCount = 0;
    auto* vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)(m_vulkanInstance.getInstanceProcAddr("vkGetPhysicalDeviceSurfacePresentModesKHR"));
    Q_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR != nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentationCount, nullptr);

    // if presentation modees returned, get list of presentation modes
    if (presentationCount != 0) {
        swapchainDetails.presentationModes.resize(presentationCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentationCount, swapchainDetails.presentationModes.data());
    }

    return swapchainDetails;
}

const VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& iFormats)
{
    // Best format is subjective, but ours will be:
    // Format    : VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM as backup)
    // ColorSpace: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR

    printDebugInfo("Coohse best surface format");

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

const VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& iPresentationModes)
{
    printDebugInfo("Coohse best presentaion mode");

    // Look for Mailbox presentation mode
    for (const VkPresentModeKHR& presentationMode : iPresentationModes) {
        if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentationMode;
        }
    }

    // If can't find Mailbox, use FIFO as Vulkan spec says it must be present;
    return VK_PRESENT_MODE_FIFO_KHR;
}

const VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& iSurfaceCapabilities)
{
    printDebugInfo("Coohse Swap Extent");

    // If current extent is at numeric limits, then extent can vary. Otherwise, it's the size of the window.
    if (iSurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        printDebugInfo("Extent width: " + QString::number(iSurfaceCapabilities.currentExtent.width));
        printDebugInfo("Extent height: " + QString::number(iSurfaceCapabilities.currentExtent.height));
        return iSurfaceCapabilities.currentExtent;
    }

    uint32_t width = static_cast<uint32_t>(m_window->width());
    uint32_t height = static_cast<uint32_t>(m_window->height());

    // Create new extent using window size
    VkExtent2D newExtent{};

    // Surface also defines max and min, so make sure within boundaries by clamping value
    newExtent.width = std::max(iSurfaceCapabilities.minImageExtent.width, std::min(iSurfaceCapabilities.maxImageExtent.width, width));
    newExtent.height = std::max(iSurfaceCapabilities.minImageExtent.height, std::min(iSurfaceCapabilities.maxImageExtent.height, height));

    printDebugInfo("Extent width: " + QString::number(newExtent.width));
    printDebugInfo("Extent height: " + QString::number(newExtent.height));

    return newExtent;
}

const VkImageView VulkanRenderer::createImageView(const VkImage iImage, const VkFormat iFormat, const VkImageAspectFlags iAspectFlags)
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
    Q_ASSERT(m_pDeviceFunctions != nullptr);
    VkResult result = m_pDeviceFunctions->vkCreateImageView(m_logicalDevice, &imageViewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create Image view");

    return imageView;
}

void VulkanRenderer::destroySwapChainResources()
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    // Destroy the old swapchain
    for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
        if (m_swapchainImages[i].imageView != VK_NULL_HANDLE) {
            m_pDeviceFunctions->vkDestroyImageView(m_logicalDevice, m_swapchainImages[i].imageView, nullptr);
            m_swapchainImages[i].imageView = VK_NULL_HANDLE;
        }
    }

    m_swapchainImages.clear();
}

bool VulkanRenderer::extensionSupported(const char* iExtension) const
{
    return (std::find(m_supportedExtensions.begin(), m_supportedExtensions.end(), iExtension) != m_supportedExtensions.end());
}

