#include "textarea2splitter.h"

#include <QResizeEvent>

#include "../../../fortmanager.h"
#include "../../../fortsettings.h"
#include "../optionscontroller.h"
#include "textarea2splitterhandle.h"

TextArea2Splitter::TextArea2Splitter(OptionsController *ctrl,
                                     QWidget *parent) :
    QSplitter(parent),
    m_ctrl(ctrl)
{
    setupUi();
}

FortManager *TextArea2Splitter::fortManager() const
{
    return ctrl()->fortManager();
}

FortSettings *TextArea2Splitter::settings() const
{
    return fortManager()->settings();
}

void TextArea2Splitter::setupUi()
{
    setHandleWidth(32);

    setupState();
}

void TextArea2Splitter::setupState()
{
    connect(fortManager(), &FortManager::afterSaveWindowState, this, [&] {
        settings()->setProperty(settingsPropName(), saveState());
    });
    connect(fortManager(), &FortManager::afterRestoreWindowState, this, [&] {
        restoreState(settings()->property(settingsPropName()).toByteArray());
    });
}

TextArea2SplitterHandle *TextArea2Splitter::handle() const
{
    return static_cast<TextArea2SplitterHandle *>(QSplitter::handle(1));
}

QSplitterHandle *TextArea2Splitter::createHandle()
{
    auto handle = new TextArea2SplitterHandle(
                selectFileEnabled(), orientation(), this);
    return handle;
}
