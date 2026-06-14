#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "vulkanrenderer.h"

#include "vulkanwidget.h"

#include <QDir>
#include <QtAssert>
#include <QVariant>

#include <exception>
#include <QDebug>
#include <fstream>

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

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

        // Create via Qt framework
        {
            createInstance();
            createSurface();
        }

        // Must create them before to create Graphics pipeline
        {
            selectPhysicalDevice();
            createLogicalDevice();
            createSwapChain();
            createDescriptorSetLayout();
            createPushConstantRange();
            createGraphicsPipeline();
        }

        // Create after Graphics pipeline
        {
            createQueryPools();
            createCommandPools();
            createCommandBuffers();
            createUniformBuffers();
            createDescriptorPool();
            createDescriptorSets();
            createSynchronization();
        }

        // Set UBOModelViewProjection buffer
        {

            m_uboModelViewProjection.model = glm::mat4(1.f);

            m_uboModelViewProjection.view = glm::lookAt(glm::vec3(0.f, 0.f, 10.f),
                                                        glm::vec3(0.f, 0.f, 0.f),
                                                        glm::vec3(0.f, 1.f, 0.f));

            /*
            const float aspect = m_extent.width / (float)m_extent.height;
            m_uboModelViewProjection.projection = glm::perspective(glm::radians(45.f),
                                                                   aspect,
                                                                   0.1f,
                                                                   100.f);
            */

            // use Orthographic projection
             m_uboModelViewProjection.projection = glm::ortho(-1.f, 1.f, -1.f, 1.f, 0.1f, 100.f);

            // GLM was designed for OpenGL, where Y coordinate of the clip coordinates is inverted.
            // The easist way to fix this is to flip the sign on the scaling factor of the Y axis in the projection matrix.
            m_uboModelViewProjection.projection[1][1] *= -1.f;
        }

        /*
        {
            std::vector<Vertex> vertices1 = {
                                                Vertex(glm::vec3(-0.9f, 0.4f, 0.f),  glm::vec3(1.f, 0.f, 0.f)),  // 0
                                                Vertex(glm::vec3(-1.0f, -0.4f, 0.f), glm::vec3(1.f, 0.f, 0.f)),  // 1
                                                Vertex(glm::vec3(-0.4f, -0.4f, 0.f),  glm::vec3(1.f, 0.f, 0.f)),  // 2
                                                Vertex(glm::vec3(-0.4f, 0.4f, 0.f),   glm::vec3(1.f, 0.f, 0.f))   // 3
                                            };

            std::vector<Vertex> vertices2 = {
                                                Vertex(glm::vec3(0.4f, 0.4f, 0.f),  glm::vec3(1.f, 0.f, 0.f)),  // 0
                                                Vertex(glm::vec3(0.4f, -0.4f, 0.f), glm::vec3(1.f, 0.f, 0.f)),  // 1
                                                Vertex(glm::vec3(0.8f, -0.4f, 0.f),  glm::vec3(1.f, 0.f, 0.f)),  // 2
                                                Vertex(glm::vec3(0.8f, 0.4f, 0.f),   glm::vec3(1.f, 0.f, 0.f))   // 3
                                            };

            m_objects.emplace_back(Rectangle(this, vertices1));
            m_objects.emplace_back(Rectangle(this, vertices2));

        }
*/
    } catch (const std::runtime_error& e) {
        printDebugInfo(e.what());
        succeded = false;
    }

    return succeded;
}

void VulkanRenderer::cleanup(std::vector<Node>& iNodes)
{
    Q_ASSERT(m_pDeviceFunctions != VK_NULL_HANDLE);

    waitDeviceIdle();

    iNodes.clear();

    // Destroy descriptor pool
    if (m_descriptorPool != VK_NULL_HANDLE) {
        m_pDeviceFunctions->vkDestroyDescriptorPool(m_logicalDevice, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    // Destroy uniform buffers
    {
        // Destroy buffer
        for (size_t i = 0; i < m_uboModelViewProjectionBuffers.size(); ++i) {
            destroyBuffer(m_uboModelViewProjectionBuffers[i]);
        }

        // Free buffer memory
        for (size_t i = 0; i < m_uboModelViewProjectionBuffersMemory.size(); ++i) {
            destroyBufferMemory(m_uboModelViewProjectionBuffersMemory[i]);
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

    // Destroy Query pools
    {
        for (size_t i = 0; i < m_queryPools.size(); ++i) {
            if (m_queryPools[i] != VK_NULL_HANDLE) {
                m_pDeviceFunctions->vkDestroyQueryPool(m_logicalDevice, m_queryPools[i], nullptr);
                m_queryPools[i] = VK_NULL_HANDLE;
            }
        }
    }

    // Destroy Graphics pipeline and layout
    {
        auto* vkDestroyPipeline = (PFN_vkDestroyPipeline)(m_vulkanInstance.getInstanceProcAddr("vkDestroyPipeline"));
        Q_ASSERT(vkDestroyPipeline != nullptr);
        if (m_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_logicalDevice, m_pipeline, nullptr);
            m_pipeline = VK_NULL_HANDLE;
        }

        auto* vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)(m_vulkanInstance.getInstanceProcAddr("vkDestroyPipelineLayout"));
        Q_ASSERT(vkDestroyPipelineLayout != nullptr);
        if (m_pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
            m_pipelineLayout = VK_NULL_HANDLE;
        }
    }


    // Destroy descriptor set layout
    {
        auto* vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)(m_vulkanInstance.getInstanceProcAddr("vkDestroyDescriptorSetLayout"));
        Q_ASSERT(vkDestroyDescriptorSetLayout != nullptr);
        if (m_descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_logicalDevice, m_descriptorSetLayout, nullptr);
            m_descriptorSetLayout = VK_NULL_HANDLE;
        }
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

    // Wait until Idle status
    m_pDeviceFunctions->vkDeviceWaitIdle(m_logicalDevice);

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

void VulkanRenderer::draw(const std::vector<Node>& iNodes)
{

    /*
    for (size_t i = 0; i < m_objects.size(); ++i) {
        glm::mat4 modelMat = glm::rotate(glm::mat4(1.f), glm::radians(60.f), glm::vec3(0.0f, 0.0f, 1.0f));
        m_uboModelViewProjection.model = modelMat;
    }
    */

    Q_ASSERT(m_pDeviceFunctions != nullptr);

    // GET NEXT IMAGE
    // Wait for give fence to signal (open) from last draw before continuing
    m_pDeviceFunctions->vkWaitForFences(m_logicalDevice, 1, &m_fences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // Manually reset (close) fences
    m_pDeviceFunctions->vkResetFences(m_logicalDevice, 1, &m_fences[m_currentFrame]);

    // Get index of next image to be drawn to, and signal semaphore then ready to be drwan to
    uint32_t imageIndex = 0;
    auto* vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)m_vulkanInstance.getInstanceProcAddr("vkAcquireNextImageKHR");
    Q_ASSERT(vkAcquireNextImageKHR != nullptr);

    vkAcquireNextImageKHR(m_logicalDevice, m_swapchain, std::numeric_limits<uint64_t>::max(), m_imagesAvailable[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    // Update Uniform buffers
    recordCommands(imageIndex, iNodes);
    updateUniformBuffers(imageIndex);

    // SUBMIT COMMAND BUFFER TO RENDERER
    {
        // Queue submission information
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;                                                     // Number of semaphores to wait on
        submitInfo.pWaitSemaphores = &m_imagesAvailable[m_currentFrame];                       // list of semaphores to wait on
        VkPipelineStageFlags stageFlags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.pWaitDstStageMask = stageFlags;                                             // Stages to check semaphores at
        submitInfo.commandBufferCount = 1;                                                     // Number of command buffers to submit
        submitInfo.pCommandBuffers = &m_graphicsCommandBuffers[imageIndex];                    // Command buffer to submit
        submitInfo.signalSemaphoreCount = 1;                                                   // Number of semaphores to signal
        submitInfo.pSignalSemaphores = &m_renderFinished[imageIndex];                          // Semaphores to signal when command buffer finishes submitting

        // Submit command buffer to Graphics queue
        VkResult result = m_pDeviceFunctions->vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_fences[m_currentFrame]);
        if (result != VK_SUCCESS) throw std::runtime_error("Failed to submit Command buffer to Graphics queue");
    }


    // PRESENT RENDERED IMAGE TO SCREEN
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_renderFinished[imageIndex];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapchain;
        presentInfo.pImageIndices = &imageIndex;

        // Present rendered image to screen
        auto* vkQueuePresentKHR = (PFN_vkQueuePresentKHR)m_vulkanInstance.getInstanceProcAddr("vkQueuePresentKHR");
        Q_ASSERT(vkQueuePresentKHR != nullptr);
        VkResult result = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
        if (result != VK_SUCCESS) throw std::runtime_error("Failed to present the rendered image to screen");
    }

    // Save previous frame for GPU time measurement
    m_prevFrame = m_currentFrame;

    // GET NEXT FRAME
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::resetNodes(const uint32_t iRowSize, const uint32_t iColSize, const glm::vec3 iColor, std::vector<Node>& oNodes)
{
    waitDeviceIdle();

    oNodes.clear();

    const float rectangleWidth = m_extent.width / static_cast<float>(iColSize);
    const float rectangleHeight = m_extent.height / static_cast<float>(iRowSize);

    const float normalizedRectangleHalfWidth = 1.f / static_cast<float>(iColSize);
    const float normalizedRectangleHalfHeight = 1.f / static_cast<float>(iRowSize);

    float halfWidth = 0.5f * m_extent.width;
    float halfHeight = 0.5f * m_extent.height;

    for (size_t i = 0; i < iRowSize; ++i) {
        for (size_t j = 0; j < iColSize; ++j) {

            float normalizedXPos = (((j * rectangleWidth) - halfWidth) / static_cast<float>(halfWidth)) + normalizedRectangleHalfWidth;
            float normalizedYPos = -((((i * rectangleHeight) - halfHeight) / static_cast<float>(halfHeight)) + normalizedRectangleHalfHeight);

            glm::vec2 recPos{normalizedXPos, normalizedYPos};


            oNodes.emplace_back(Node(this, recPos, normalizedRectangleHalfWidth, normalizedRectangleHalfHeight, iColor));
        }
    }

}

void VulkanRenderer::waitDeviceIdle()
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    // Wait until Idle status
    m_pDeviceFunctions->vkDeviceWaitIdle(m_logicalDevice);
}


void VulkanRenderer::createVertexBuffer(const std::vector<Vertex>& iVertices,VkBuffer& oBuffer, VkDeviceMemory& oBufferMemory)
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    const VkDeviceSize bufferSize = sizeof(Vertex) * iVertices.size();

    // Temporary buffer to "stage" vertex data before transferring to GPU
    VkBuffer stagingBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};

    // Create Buffer and Allocate memory
    createBuffer(m_physicalDevice,
                 m_logicalDevice,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);

    // Map memory to vertex buffer (CPU is writing to the mapping process)
    void* pData = nullptr;                                                                                // 1. Cretae pointer  to a pointer in normal memory
    m_pDeviceFunctions->vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &pData);      // 2. Map the vertex buffer memory to the pointer
    memcpy(pData, iVertices.data(), static_cast<size_t>(bufferSize));                                     // 3. Copy memory from vertices vector to the pointer
    m_pDeviceFunctions->vkUnmapMemory(m_logicalDevice, stagingBufferMemory);                              // 4. Unmap the vertex buffer memory

    // Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
    // Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it  and not CPU (host)
    createBuffer(m_physicalDevice,
                 m_logicalDevice,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // Only visible to GPU
                 oBuffer,
                 oBufferMemory);

    // Copy stagaing bufeer to vertex buffer on GPU
    copyBuffer(bufferSize, stagingBuffer, oBuffer);

    // Clean up staging buffer parts
    destroyBuffer(stagingBuffer);
    destroyBufferMemory(stagingBufferMemory);
}

void VulkanRenderer::createIndexBuffer(const std::vector<uint32_t>& iIndices, VkBuffer& oBuffer, VkDeviceMemory& oBufferMemory)
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    const VkDeviceSize bufferSize = sizeof(uint32_t) * iIndices.size();

    // Temporary buffer to "stage" index data before transferring to GPU
    VkBuffer stagingBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};

    // Create Buffer and Allocate memory
    createBuffer(m_physicalDevice,
                 m_logicalDevice,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);

    // Map memory to index buffer (CPU is writing to the mapping process)
    void* pData = nullptr;                                                                                // 1. Cretae pointer  to a pointer in normal memory
    m_pDeviceFunctions->vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &pData);      // 2. Map the index buffer memory to the pointer
    memcpy(pData, iIndices.data(), static_cast<size_t>(bufferSize));                                      // 3. Copy memory from vertices vector to the pointer
    m_pDeviceFunctions->vkUnmapMemory(m_logicalDevice, stagingBufferMemory);                              // 4. Unmap the index buffer memory

    // Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
    // Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it  and not CPU (host)
    createBuffer(m_physicalDevice,
                 m_logicalDevice,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // Only visible to GPU
                 oBuffer,
                 oBufferMemory);

    // Copy stagaing bufeer to vertex buffer on GPU
    copyBuffer(bufferSize, stagingBuffer, oBuffer);

    // Clean up staging buffer parts
    destroyBuffer(stagingBuffer);
    destroyBufferMemory(stagingBufferMemory);
}

void VulkanRenderer::destroyBuffer(VkBuffer& ioBuffer)
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    if (ioBuffer != VK_NULL_HANDLE) {
        m_pDeviceFunctions->vkDestroyBuffer(m_logicalDevice, ioBuffer, nullptr);
        ioBuffer = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::destroyBufferMemory(VkDeviceMemory& ioBufferMemory)
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    if (ioBufferMemory != VK_NULL_HANDLE) {
        m_pDeviceFunctions->vkFreeMemory(m_logicalDevice, ioBufferMemory, nullptr);
        ioBufferMemory = VK_NULL_HANDLE;
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

    VkPhysicalDeviceVulkan12Features enabledFeatures12{};
    enabledFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    enabledFeatures12.hostQueryReset = VK_TRUE; // Enable CPU to call vkResetQueryPool
    enabledFeatures12.pNext = &enabledFeatures13; // Connect VkPhysicalDeviceVulkan13Features

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
    physicalDeviceFeatures2.pNext = &enabledFeatures12;

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

void VulkanRenderer::createCommandBuffers()
{
    printDebugInfo("Create Command buffers");

    Q_ASSERT(m_pDeviceFunctions != nullptr);

    // Resize command buffer count to have one for each frame buffer
    m_graphicsCommandBuffers.resize(m_swapchainImages.size());

    // Command buffer allocation information
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = m_graphicsCommandPool;
    // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Buffer you submit directly to queue. Can't be called by other buffers.
    // VK_COMMAND_BUFFER_LEVEL_SECONDARY: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer.
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_graphicsCommandBuffers.size());

    // Allocate command buffers and place handles in array of buffers
    VkResult result = m_pDeviceFunctions->vkAllocateCommandBuffers(m_logicalDevice, &commandBufferAllocateInfo, m_graphicsCommandBuffers.data());
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to allocate Command Buffers");
}

void VulkanRenderer::createUniformBuffers()
{
    printDebugInfo("Create uniform buffers");

    // ModelViewProjection buffer size
    VkDeviceSize modelViewProjection = sizeof(UboModelViewProjection);

    // Create ModelViewProjection uniform buffer for each swapchain image
    m_uboModelViewProjectionBuffers.resize(m_swapchainImages.size());
    m_uboModelViewProjectionBuffersMemory.resize(m_swapchainImages.size());

    // Create ModelViewProjection uniform buffers
    for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
        createBuffer(m_physicalDevice,
                    m_logicalDevice,
                    modelViewProjection,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    m_uboModelViewProjectionBuffers[i],
                    m_uboModelViewProjectionBuffersMemory[i]);
    }
}

void VulkanRenderer::createBuffer(const VkPhysicalDevice iPhysicalDevice,
                                  const VkDevice iDevice,
                                  const VkDeviceSize iBufferSize,
                                  const VkBufferUsageFlags iBufferUsageFlags,
                                  const VkMemoryPropertyFlags iBufferProperties,
                                  VkBuffer& oBuffer,
                                  VkDeviceMemory& oBufferMemory)
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    // Create vertex buffer
    // Buffer creation information
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = iBufferSize;                       // Buffer size
    bufferCreateInfo.usage = iBufferUsageFlags;                // Multiple type of buffer possible
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // Similar to swapchain images, can share vertex buffers

    VkResult result = m_pDeviceFunctions->vkCreateBuffer(m_logicalDevice, &bufferCreateInfo, nullptr, &oBuffer);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create Vertex buffer");

    // Get buffer memory requirements
    VkMemoryRequirements memRequirements{};
    m_pDeviceFunctions->vkGetBufferMemoryRequirements(m_logicalDevice, oBuffer, &memRequirements);

    // Malloc information
    VkMemoryAllocateInfo mallocInfo{};
    mallocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mallocInfo.allocationSize = memRequirements.size;
    mallocInfo.memoryTypeIndex = findMemoryTypeIndex(iPhysicalDevice, memRequirements.memoryTypeBits, iBufferProperties);

    // Allocate memory to buffer
    result= m_pDeviceFunctions->vkAllocateMemory(m_logicalDevice, &mallocInfo, nullptr, &oBufferMemory);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to allocate Vertex buffer memroy");

    // Allocate memory to given vertex buffer
    m_pDeviceFunctions->vkBindBufferMemory(m_logicalDevice, oBuffer, oBufferMemory, 0);
}

const uint32_t VulkanRenderer::findMemoryTypeIndex(const VkPhysicalDevice iPhysicalDevice, const uint32_t allowdedTypes, const VkMemoryPropertyFlags properties)
{
    Q_ASSERT(m_pFunctions != nullptr);

    // Get properties of physical device memory
    VkPhysicalDeviceMemoryProperties memProperties{};
    m_pFunctions->vkGetPhysicalDeviceMemoryProperties(iPhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((allowdedTypes & (1 << i)) &&                                                // Index of memory type must match corresponding bit in allowedTypes
            ((memProperties.memoryTypes[i].propertyFlags & properties) == properties))   // Desired property bit flags are part of memory type's property flags
        {
            // This memory type is valide, so return its index
            return i;
        }
    }

    return 0;
}

float VulkanRenderer::readGPUFrameTime()
{
    return readGPUFrameTime(m_prevFrame);
}

float VulkanRenderer::readGPUFrameTime(const size_t iFrameIndex)
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    uint64_t timestamps[FPS_QUERY_COUNT] = {0, 0};

    VkResult result = m_pDeviceFunctions->vkGetQueryPoolResults(m_logicalDevice,
                                                                m_queryPools[iFrameIndex],
                                                                0,
                                                                FPS_QUERY_COUNT,
                                                                sizeof(timestamps), timestamps,
                                                                sizeof(uint64_t),
                                                                VK_QUERY_RESULT_64_BIT); // Removed WAIT_BIT

    // if GPU is still VK_NOT_READY or send abnormal values
    if (result != VK_SUCCESS || timestamps[1] <= timestamps[0]) {
        return m_gpuTimesMs[iFrameIndex];
    }

    // Time diff in milliseconds
    m_gpuTimesMs[iFrameIndex] = (timestamps[1] - timestamps[0]) * m_physicalDeviceProperties.limits.timestampPeriod * 1e-6f;

    return m_gpuTimesMs[iFrameIndex];
}

void VulkanRenderer::createDescriptorPool()
{
    printDebugInfo("Create Descriptor pool");

    Q_ASSERT(m_pDeviceFunctions != nullptr);

    // CREATE UNIFORM BUFFER DESCRIPTOR POOL
    // Type of descriptor + how many Descriptoprs, not Descriptor Sets (combined makes the pool size)
    VkDescriptorPoolSize modelViewProjectionDescriptorPoolSize{};
    modelViewProjectionDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;                                         // Type of descriptor (uniform buffer, image sampler, etc)
    modelViewProjectionDescriptorPoolSize.descriptorCount = static_cast<uint32_t>(m_uboModelViewProjectionBuffers.size());  // Number of descriptor of this type

    // List of descriptor pool sizes
    std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {modelViewProjectionDescriptorPoolSize};

    // Descriptor pool creation information
    VkDescriptorPoolCreateInfo modelViewProjectionDescriptorPoolCreateInfo{};
    modelViewProjectionDescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;              // Type of the structures
    modelViewProjectionDescriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(m_swapchainImages.size());          // Maximum number of descriptor sets that can be allocated from pool
    modelViewProjectionDescriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());  // Amount of pool sizes being passed
    modelViewProjectionDescriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();                            // Pool sizes to create pool with

    // Create Descriptor pool
    VkResult result = m_pDeviceFunctions->vkCreateDescriptorPool(m_logicalDevice, &modelViewProjectionDescriptorPoolCreateInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create Descriptor pool");
}

void VulkanRenderer::createDescriptorSets()
{
    printDebugInfo("Create Descriptor sets");

    Q_ASSERT(m_pDeviceFunctions != nullptr);

    // Resize descriptor sets so one for every unifrom buffer
    m_descriptorSets.resize(m_swapchainImages.size());

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(m_swapchainImages.size(), m_descriptorSetLayout);

    // Descriptor set allocation information
    VkDescriptorSetAllocateInfo descriptorSetAllocateinfo{};
    descriptorSetAllocateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateinfo.descriptorPool = m_descriptorPool;                                     // Pool to allocate Descriptor sets from
    descriptorSetAllocateinfo.descriptorSetCount = static_cast<uint32_t>(m_swapchainImages.size());  // Number of descriptor sets to allocate
    descriptorSetAllocateinfo.pSetLayouts = descriptorSetLayouts.data();                             // Layouts to use for each allocated set

    VkResult result = m_pDeviceFunctions->vkAllocateDescriptorSets(m_logicalDevice, &descriptorSetAllocateinfo, m_descriptorSets.data());
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create Descriptor sets");

    // Update descritor sets with buffer bindings
    for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
        // UBO modelViewProejction descriptor
        VkDescriptorBufferInfo modelViewProjectionUniformBufferInfo{};

        modelViewProjectionUniformBufferInfo.buffer = m_uboModelViewProjectionBuffers[i];  // Buffer to bind to descriptor set
        modelViewProjectionUniformBufferInfo.offset = 0;                                   // Offset in buffer to start of data
        modelViewProjectionUniformBufferInfo.range = sizeof(UboModelViewProjection);       // Size of data to bind

        // Data about connection between buffer and binding in shader
        VkWriteDescriptorSet modelViewProjectionUniformBufferWriteDescriptorSet{};
        modelViewProjectionUniformBufferWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        modelViewProjectionUniformBufferWriteDescriptorSet.dstSet = m_descriptorSets[i];                        // Descriptor set to write to
        modelViewProjectionUniformBufferWriteDescriptorSet.dstBinding = 0;                                      // Binding in shader where data will be read
        modelViewProjectionUniformBufferWriteDescriptorSet.dstArrayElement = 0;                                 // Index in array to write to
        modelViewProjectionUniformBufferWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // Type of descriptor
        modelViewProjectionUniformBufferWriteDescriptorSet.descriptorCount = 1;                                 // Number of descriptors t write
        modelViewProjectionUniformBufferWriteDescriptorSet.pBufferInfo = &modelViewProjectionUniformBufferInfo; // Buffer info

        // List of write descriptor sets
        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {modelViewProjectionUniformBufferWriteDescriptorSet};

        // Update the descriptor set with new buffer/binding info
        m_pDeviceFunctions->vkUpdateDescriptorSets(m_logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }
}

void VulkanRenderer::recordCommands(const uint32_t iImageIndex, const std::vector<Node>& iNodes)
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    // Reset Command buffer
    VkCommandBuffer commandBuffer = m_graphicsCommandBuffers[iImageIndex];
    m_pDeviceFunctions->vkResetCommandBuffer(commandBuffer, 0);

    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // Start recording commands
    VkResult result = m_pDeviceFunctions->vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to start recording Command buffer");

    // Reset query pool
    m_pDeviceFunctions->vkCmdResetQueryPool(commandBuffer, m_queryPools[m_currentFrame], 0, FPS_QUERY_COUNT);

    // Frame start timestamp
    m_pDeviceFunctions->vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_queryPools[m_currentFrame], 0);

    // RECORD COMMANDS
    {
        // Image layout transformation (UNDEFINED -> COLOR_ATTACHMENT)
        VkImageMemoryBarrier imageMemoryBarrierToRender{};
        imageMemoryBarrierToRender.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrierToRender.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrierToRender.newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imageMemoryBarrierToRender.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrierToRender.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrierToRender.image               = m_swapchainImages[iImageIndex].image;
        imageMemoryBarrierToRender.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        imageMemoryBarrierToRender.srcAccessMask       = 0;
        imageMemoryBarrierToRender.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        m_pDeviceFunctions->vkCmdPipelineBarrier(commandBuffer,
                                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                 0, 0, nullptr, 0, nullptr,
                                                 1, &imageMemoryBarrierToRender);

        // Rendering attachment information for Dynamic rendering
        VkRenderingAttachmentInfo renderingAttachmentInfo{};
        renderingAttachmentInfo.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        renderingAttachmentInfo.imageView   = m_swapchainImages[iImageIndex].imageView;
        renderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        renderingAttachmentInfo.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        renderingAttachmentInfo.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        renderingAttachmentInfo.clearValue  = {{{0.f, 0.f, 0.f, 1.f}}};

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset    = {0, 0};
        renderingInfo.renderArea.extent    = m_extent;
        renderingInfo.layerCount           = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments    = &renderingAttachmentInfo;

        auto* vkCmdBeginRendering = (PFN_vkCmdBeginRendering)(m_vulkanInstance.getInstanceProcAddr("vkCmdBeginRendering"));
        Q_ASSERT(vkCmdBeginRendering != nullptr);
        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        // RECORD RENDERING COMMANDS
        {
            // Bind Graphic pipeline
            m_pDeviceFunctions->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
            // Set Primitive Topology
            auto* vkCmdSetPrimitiveTopology = (PFN_vkCmdSetPrimitiveTopology)(m_vulkanInstance.getInstanceProcAddr("vkCmdSetPrimitiveTopology"));
            Q_ASSERT(vkCmdSetPrimitiveTopology != nullptr);
            vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

            // Viewport
            VkViewport viewport{};
            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.width    = static_cast<float>(m_extent.width);
            viewport.height   = static_cast<float>(m_extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            m_pDeviceFunctions->vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            // Scissor
            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = m_extent;

            m_pDeviceFunctions->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            for (size_t i = 0; i < iNodes.size(); ++i) {

                // Bind mesh vertex buffer with 0 offset
                std::array<VkBuffer, 1> vertexBuffer = { iNodes[i].getVertexBuffer() }; // Buffers to bind
                std::array<VkDeviceSize, 1> offsets = { 0 };                              // Offsets into buffers being bound
                m_pDeviceFunctions->vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.data(), offsets.data()); // Command to bind vertex buffer before drawing with that

                // Bind mesh index buffer with 0 offset and using the uint32 type
                m_pDeviceFunctions->vkCmdBindIndexBuffer(commandBuffer, iNodes[i].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

                // Bind Descriptor sets (for Uniform Buffer ModelViewProjection)
                m_pDeviceFunctions->vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
                                                        0, 1,
                                                        &m_descriptorSets[iImageIndex],
                                                        0, nullptr);

                // Transfer pushConstantInfo_t to Fragment shader via PushConstant
                pushConstantInfo_t pushConstInfo{};
                pushConstInfo.color = glm::vec4(iNodes[i].getColor(), 1.f);
                pushConstInfo.borderColor = glm::vec4(0.3f, 0.3f, 0.3f, 0.5f);
                pushConstInfo.borderWidth = 0.05f;

                m_pDeviceFunctions->vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstantInfo_t), &pushConstInfo);

                // Draw by index
                m_pDeviceFunctions->vkCmdDrawIndexed(commandBuffer, iNodes[i].getIndexCount(), 1, 0, 0, 0);
            }
        }

        auto* vkCmdEndRendering = (PFN_vkCmdEndRendering)(m_vulkanInstance.getInstanceProcAddr("vkCmdEndRendering"));
        Q_ASSERT(vkCmdBeginRendering != nullptr);
        vkCmdEndRendering(commandBuffer);


        // Image layout transformation (COLOR_ATTACHMENT -> PRESENT)
        VkImageMemoryBarrier imageMemoryBarrierToPresent{};
        imageMemoryBarrierToPresent.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrierToPresent.oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imageMemoryBarrierToPresent.newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageMemoryBarrierToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrierToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrierToPresent.image               = m_swapchainImages[iImageIndex].image;
        imageMemoryBarrierToPresent.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        imageMemoryBarrierToPresent.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imageMemoryBarrierToPresent.dstAccessMask       = 0;

        m_pDeviceFunctions->vkCmdPipelineBarrier(commandBuffer,
                                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                                0, 0, nullptr, 0, nullptr,
                                                1, &imageMemoryBarrierToPresent);
    }

    // Frame end timestamp
    m_pDeviceFunctions->vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPools[m_currentFrame], 1);

    // Stop recording commands
    result = m_pDeviceFunctions->vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to stop recording Command buffer");
}

void VulkanRenderer::updateUniformBuffers(const uint32_t iImageIndex)
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    // Copy ModelViewProjection data to uniform buffer
    void* pData = nullptr;
    m_pDeviceFunctions->vkMapMemory(m_logicalDevice, m_uboModelViewProjectionBuffersMemory[iImageIndex], 0, sizeof(m_uboModelViewProjection), 0, &pData);
    memcpy(pData, &m_uboModelViewProjection, sizeof(m_uboModelViewProjection));
    m_pDeviceFunctions->vkUnmapMemory(m_logicalDevice, m_uboModelViewProjectionBuffersMemory[iImageIndex]);
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

void VulkanRenderer::createPushConstantRange()
{
    m_pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Send PushConstant to fragment shader
    m_pushConstantRange.offset = 0;
    m_pushConstantRange.size = sizeof(pushConstantInfo_t);
}

void VulkanRenderer::createGraphicsPipeline()
{
    printDebugInfo("Create Graphics pipeline");

    Q_ASSERT(m_pDeviceFunctions != nullptr);

    // Graphics pipeline creation info requres the array of shader stage creation info
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    // HANDLE SHADERS
    // Read in SPIR-V code of shaders
    std::vector<char> vertexShaderCode = readFile("C:/Users/ssh24/Documents/QtVulkanTSP/shaders/vert.spv");
    std::vector<char> fragmentShaderCode = readFile("C:/Users/ssh24/Documents/QtVulkanTSP/shaders/frag.spv");

    // Build shader modules to link to Graphics pipeline
    VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    // Shader stage creation information
    // Vertex stage creation information
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo{};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;           // Shader stage name
    vertexShaderCreateInfo.module = vertexShaderModule;                  // Shader module to be used by stage
    vertexShaderCreateInfo.pName = "main";                               // Entry point to shader

    // Fragment stage creation information
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo{};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;       // Shader stage name
    fragmentShaderCreateInfo.module = fragmentShaderModule;              // Shader module to be used by stage
    fragmentShaderCreateInfo.pName = "main";                             // Entry point to shader

    // Put shader stage creation info to vector
    shaderStages.push_back(vertexShaderCreateInfo);
    shaderStages.push_back(fragmentShaderCreateInfo);

    // CREATE GRAPHICS PIPELINE
    {
        // How the data for a single vertex (including info such as position, color, texture coords, normals, etc) is as a whole
        VkVertexInputBindingDescription vertexbindingDescption{};
        vertexbindingDescption.binding = 0;
        vertexbindingDescption.stride = sizeof(Vertex);
        vertexbindingDescption.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // How to move between data after each vertex

        // How the data for an attribute is defined within a vertex
        std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributesDescripotions;

        // Position attribute
        // layout(location = 0) in vec3 pos in Vertex Shader
        vertexInputAttributesDescripotions[0].binding = 0;                         // Which binding the data is at (should be same as above)
        vertexInputAttributesDescripotions[0].location = 0;                        // Location in shader where data will be read from
        vertexInputAttributesDescripotions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // Formate the data will take (also helps to define the size of data). 12 bytes.
        vertexInputAttributesDescripotions[0].offset = offsetof(Vertex, pos);      // Where this attribute is defined in the data for a single vertex

        // Color attribute
        // layout(location = 1) in vec3 pos in Vertex Shader
        vertexInputAttributesDescripotions[1].binding = 0;                         // Which binding the data is at (should be same as above)
        vertexInputAttributesDescripotions[1].location = 1;                        // Location in shader where data will be read from
        vertexInputAttributesDescripotions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // Formate the data will take (also helps to define the size of data). 12 bytes.
        vertexInputAttributesDescripotions[1].offset = offsetof(Vertex, col);      // Where this attribute is defined in the data for a single vertex

        // UV attribute
        // layout(location = 2) in vec2 uv in Vertex Shader
        vertexInputAttributesDescripotions[2].binding = 0;
        vertexInputAttributesDescripotions[2].location = 2;
        vertexInputAttributesDescripotions[2].format = VK_FORMAT_R32G32_SFLOAT; // 8 bytes for UV
        vertexInputAttributesDescripotions[2].offset = offsetof(Vertex, uv);

        // VERTEX INPUT STATE
        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
        vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexbindingDescption;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributesDescripotions.size());
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributesDescripotions.data();

        // INPUT ASSEMBLY
        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
        pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;    // Default primitive type to assemble vertices as
        pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;                 // Allow overriding of "strip" topology to start new primitives

        // VIEWPORT AND SCISSOR
        // Create a viewport info struct
        VkViewport viewPort{};
        viewPort.x = 0.f;                                       // x start coordinate
        viewPort.y = 0.f;                                       // y start coordinate
        viewPort.width = static_cast<float>(m_extent.width);    // Width of viewport
        viewPort.height = static_cast<float>(m_extent.height);  // Height of viewport
        viewPort.minDepth = 0.f;                                // Min framebuffer depth
        viewPort.maxDepth = 1.f;                                // Max framebuffer depth

        // Create a scissor info struct
        VkRect2D scissor{};
        scissor.offset = {0, 0};    // Offset to use region from
        scissor.extent = m_extent;  // Extent to describe region to use, starting at offset

        VkPipelineViewportStateCreateInfo pipelineViewportSateCreateInfo{};
        pipelineViewportSateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipelineViewportSateCreateInfo.viewportCount = 1;
        pipelineViewportSateCreateInfo.pViewports = &viewPort;
        pipelineViewportSateCreateInfo.scissorCount = 1;
        pipelineViewportSateCreateInfo.pScissors = &scissor;

        // DYNAMIC STATES
        // Dynamic states to enable
        std::vector<VkDynamicState> dynamicStates ={VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY};

        // Dynamic state creation information
        VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
        pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        // RASTERIZER
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
        pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;                    // Change if fragments beyond near/far planes are clipped(default) or clamped to plane
        pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;             // Whether to discard data and skip rasterizer. Never creates fragments, Only suitable for pipeline without framebuffer output
        pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;             // how to handle filling points between vertices
        pipelineRasterizationStateCreateInfo.lineWidth = 1.f;                                // How thick lines should be when drawn
        pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;               // Which face of a triangle to cull
        pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;    // Determine which side is front
        pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;                     // Whether to add depth bias to fragments (good for stoppoing "shadow acne" in shadow mapping)

        // MULTI-SAMPLING
        VkPipelineMultisampleStateCreateInfo pipelineMultiSampleStateCreateInfo{};
        pipelineMultiSampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipelineMultiSampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        pipelineMultiSampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


        // BLENDING
        // Blending decides how to blend a new color being written to a fragment, with the old value
        // Color blend attachment State
        VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
        pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        pipelineColorBlendAttachmentState.blendEnable = VK_TRUE; // Enable blending

        // Blending uses equation: (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)
        pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        // Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
        //             (new color alpha * new color) + ((1 - new color alpha)  * old color)

        pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        // Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

        // Color blend state create information
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
        pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;                       // Alternative to calculations is to use logical opeations
        pipelineColorBlendStateCreateInfo.attachmentCount = 1;
        pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;

        // PIPELINE LAYOUT
        // Pipeline layout create information
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &m_pushConstantRange;

        VkResult result = m_pDeviceFunctions->vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
        if (result != VK_SUCCESS) throw std::runtime_error("Failed to create pipeline layout");

        // DEPTH STENCIL TESTING
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
        pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;            // Enable depth testing (closer fragments should be rendered in front of farther ones)
        pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;           // Whether to wrtie to depth buffer
        pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;  // Comparison operation to use to compare new depth value with existing one in depth buffer
        pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;     // Depth bounds test: check if depth value exist within a min/max range
        pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;         // Optional stencil test

        // Rendering create Info for Dynamic rendering
        VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.pNext = nullptr;
        pipelineRenderingCreateInfo.viewMask = 0;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        VkFormat colorFormat = m_surfaceFormat.format;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
        pipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
        pipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;


        // GRAPHICS PIPELINE CREATION
        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
        graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        graphicsPipelineCreateInfo.pStages = shaderStages.data();
        graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
        graphicsPipelineCreateInfo.pViewportState = &pipelineViewportSateCreateInfo;
        graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
        graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
        graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultiSampleStateCreateInfo;
        graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
        graphicsPipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
        graphicsPipelineCreateInfo.layout = m_pipelineLayout;
        graphicsPipelineCreateInfo.renderPass = VK_NULL_HANDLE;                                    // No need Renderpass here as we use Dynamic rendering
        graphicsPipelineCreateInfo.subpass = 0;
        graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;                           // Use Dynamic rendering

        // Pipeline derivatives: Can create multiple pipelines that derive from one another for optimization
        graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;                            // Existing pipeline to derive from
        graphicsPipelineCreateInfo.basePipelineIndex = -1;                                         // or index of pipeline being created to derive from (in case creating multiple at once)

        result = m_pDeviceFunctions->vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_pipeline);
        if (result != VK_SUCCESS) throw std::runtime_error("Failed to create pipeline");
    }

    m_pDeviceFunctions->vkDestroyShaderModule(m_logicalDevice, fragmentShaderModule, nullptr);
    m_pDeviceFunctions->vkDestroyShaderModule(m_logicalDevice, vertexShaderModule, nullptr);
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char> &iShaderCode)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = iShaderCode.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(iShaderCode.data());

    VkShaderModule shaderModule{VK_NULL_HANDLE};
    VkResult result = m_pDeviceFunctions->vkCreateShaderModule(m_logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create Shader module");

    return shaderModule;
}

void VulkanRenderer::createQueryPools()
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    m_queryPools.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    m_gpuTimesMs.resize(MAX_FRAMES_IN_FLIGHT, 0.f);

    // Query pool creation info for FPS
    VkQueryPoolCreateInfo queryPoolCreateInfo{};
    queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolCreateInfo.queryCount = FPS_QUERY_COUNT; // Start-End

    for (size_t i = 0; i < m_queryPools.size(); ++i) {
        VkResult result = m_pDeviceFunctions->vkCreateQueryPool(m_logicalDevice, &queryPoolCreateInfo, nullptr, &m_queryPools[i]);
        if (result != VK_SUCCESS) throw std::runtime_error("Failed to create Query pool");

        auto* vkResetQueryPool = (PFN_vkResetQueryPool)m_vulkanInstance.getInstanceProcAddr("vkResetQueryPool");
        Q_ASSERT(vkResetQueryPool != nullptr);
        vkResetQueryPool(m_logicalDevice, m_queryPools[i], 0, FPS_QUERY_COUNT);
    }
}

void VulkanRenderer::createSynchronization()
{
    printDebugInfo("Create Synchronization");

    Q_ASSERT(m_pDeviceFunctions!= nullptr);

    m_imagesAvailable.resize(MAX_FRAMES_IN_FLIGHT,     VK_NULL_HANDLE);
    m_renderFinished.resize(m_swapchainImages.size(),  VK_NULL_HANDLE);
    m_fences.resize(MAX_FRAMES_IN_FLIGHT,              VK_NULL_HANDLE);

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

const VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& iPresentationModes)
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

const VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& iSurfaceCapabilities)
{
    printDebugInfo("Choose Swap Extent");

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

void VulkanRenderer::copyBuffer(const VkDeviceSize iBufferSize, VkBuffer &iSrcBuffer, VkBuffer &iDstBuffer)
{
    Q_ASSERT(m_pDeviceFunctions != nullptr);

    // Command buffer to hold transfer commands
    VkCommandBuffer commandBuffer{};

    // Command buffer allocate information
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandPool = m_graphicsCommandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;

    // Allocate command buffer from pool
    m_pDeviceFunctions->vkAllocateCommandBuffers(m_logicalDevice, &commandBufferAllocateInfo, &commandBuffer);

    // Information to begin the command buffer record
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We only use the command buffer once

    // Begin recording commands
    m_pDeviceFunctions->vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    // Region of data to copy from and to
    VkBufferCopy bufferCopyRegion{};
    bufferCopyRegion.srcOffset = 0;
    bufferCopyRegion.dstOffset = 0;
    bufferCopyRegion.size = iBufferSize;

    // Command to copy src buffer to dst buffer
    m_pDeviceFunctions->vkCmdCopyBuffer(commandBuffer, iSrcBuffer, iDstBuffer, 1, &bufferCopyRegion);

    // End commands
    m_pDeviceFunctions->vkEndCommandBuffer(commandBuffer);

    // Queue submission information
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Submit command to queue and wait until it finishes
    m_pDeviceFunctions->vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    m_pDeviceFunctions->vkQueueWaitIdle(m_graphicsQueue);

    // Free temporary command buffer back to pool
    m_pDeviceFunctions->vkFreeCommandBuffers(m_logicalDevice, m_graphicsCommandPool, 1, &commandBuffer);
}

bool VulkanRenderer::extensionSupported(const char* iExtension) const
{
    return (std::find(m_supportedExtensions.begin(), m_supportedExtensions.end(), iExtension) != m_supportedExtensions.end());
}

std::vector<char> VulkanRenderer::readFile(const std::string &iFilePath)
{
    // Open stream from given file
    // std::ios::binary tells stream to read file as binary
    // std::ios::ate tells stream to start reading from end of file
    std::ifstream file(iFilePath, std::ios_base::binary | std::ios::ate);

    // Check if file stream successfully opened
    if (!file.is_open()) throw std::runtime_error("Failed to open a file");

    // Get current read position and use to resize file buffer
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> fileBuffer(fileSize);

    // Move read position (seek to) the start of the file
    file.seekg(0);

    // Read the file data into the buffer (stream "fileSize" in total)
    file.read(fileBuffer.data(), fileSize);

    return fileBuffer;
}

