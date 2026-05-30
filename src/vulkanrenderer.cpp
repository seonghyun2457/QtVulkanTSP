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
    } catch (const std::runtime_error& e) {
        printDebugInfo(e.what());
        succeded = false;
    }

    return succeded;
}

void VulkanRenderer::cleanup()
{
    Q_ASSERT(m_pDeviceFunctions != VK_NULL_HANDLE);
    m_pDeviceFunctions->vkDeviceWaitIdle(m_logicalDevice);

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

    // Destroy logical device
    {
        m_pDeviceFunctions->vkDestroyDevice(m_logicalDevice, nullptr);
        m_pDeviceFunctions = VK_NULL_HANDLE;
    }

    // Destroy SwapChain
    {

    }

    // DO NOT destroy m_surface here: — it is owned by Qt's platform window.
    // DO NOT destroy m_vulkanInstance here: QVulkanInstance's destructor handles it;
    // calling it here causes a double-destroy when the member is later destructed.
    qDebug() << "cleaned up";
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
    m_surface = QVulkanInstance::surfaceForWindow(m_window);
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

void VulkanRenderer::createLogicalDevice()
{
    printDebugInfo("Create logical device");

    const VkQueueFlags requestedQueueTypes = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;

    VkPhysicalDeviceVulkan13Features enabledFeatures13{};
    enabledFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    enabledFeatures13.dynamicRendering = VK_TRUE;
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
    printDebugInfo("Create Swap Chain");

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

bool VulkanRenderer::extensionSupported(const char *iExtension) const
{
    return (std::find(m_supportedExtensions.begin(), m_supportedExtensions.end(), iExtension) != m_supportedExtensions.end());
}

