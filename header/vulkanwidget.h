#ifndef VULKANWIDGET_H
#define VULKANWIDGET_H

#include <QWindow>

#include "vulkanrenderer.h"

class VulkanWidget : public QWindow
{
    Q_OBJECT
public:
    VulkanWidget();
    virtual ~VulkanWidget();

signals:
    void sendVulkanInfo(const QString& iVulkanInfoString);
    void sendDebugInfo(const QString& iDebugString);

protected:
    virtual void exposeEvent(QExposeEvent* event) override;
    virtual bool event(QEvent* e) override;
    virtual void resizeEvent(QResizeEvent* event) override;


private:
    void initializeRenderer();
    void draw();

private:
    std::unique_ptr<VulkanRenderer> m_pVulkanRenderer;
    bool m_initisialized{false};
};

#endif // VULKANWIDGET_H
