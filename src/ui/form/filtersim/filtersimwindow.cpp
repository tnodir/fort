#include "filtersimwindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/app.h>
#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <conf/confrulemanager.h>
#include <conf/confzonemanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/dialog/dialogutil.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/connlistmodel.h>
#include <user/iniuser.h>
#include <util/fileutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/net/netformatutil.h>
#include <util/net/netutil.h>
#include <util/stringutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "filtersimcontroller.h"

using namespace Fort;

namespace {

inline constexpr int PORT_MAX = 65535;
inline constexpr int IP_PROTO_MAX = 255;

inline constexpr int SIMULATE_DELAY_MSEC = 150;

const QSize resultIconSize(16, 16);

QComboBox *createFixedComboBox(const QStringList &texts = {})
{
    auto c = ControlUtil::createComboBox(texts);
    c->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return c;
}

QSpinBox *createPortSpinBox()
{
    auto c = ControlUtil::createSpinBox();
    c->setRange(0, PORT_MAX);
    return c;
}

QLayout *createIpPortLayout(QWidget *editIp, QWidget *labelPort, QWidget *spinPort)
{
    editIp->setMaximumWidth(200);

    auto layout = new QHBoxLayout();
    layout->addWidget(editIp, 1);
    layout->addSpacing(10);
    layout->addWidget(labelPort);
    layout->addSpacing(10);
    layout->addWidget(spinPort);
    layout->addStretch();

    return layout;
}

QLabel *createResultLabel()
{
    auto c = ControlUtil::createLabel();
    c->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return c;
}

QString trimIpText(const QString &text)
{
    const QString ipText = text.trimmed();

    // IPv6 address in brackets
    if (ipText.startsWith('[') && ipText.endsWith(']')) {
        return ipText.mid(1, ipText.size() - 2);
    }

    return ipText;
}

bool textToIp(const QString &text, ip_addr_t &ip, bool &isIPv6)
{
    const QString ipText = trimIpText(text);

    isIPv6 = ipText.contains(':');

    bool ok = false;

    if (isIPv6) {
        ip.v6 = NetFormatUtil::textToIp6(ipText, &ok);
    } else {
        ip.v4 = NetFormatUtil::textToIp4(ipText, &ok);
    }

    return ok;
}

bool isLoopbackIp(const ip_addr_t &ip, bool isIPv6)
{
    if (isIPv6) {
        static const ip6_addr_t loopbackIp6 = NetFormatUtil::textToIp6("::1");

        return memcmp(&ip.v6, &loopbackIp6, sizeof(ip6_addr_t)) == 0;
    }

    return (ip.v4 >> 24) == 127; // 127.0.0.0/8
}

bool textToProtocol(const QString &text, quint8 &ipProto)
{
    const QString name = text.trimmed();

    bool ok = false;

    ipProto = NetUtil::protocolNumber(name, ok);
    if (ok)
        return true;

    const uint v = name.toUInt(&ok);
    ipProto = quint8(v);

    return ok && v <= IP_PROTO_MAX;
}

QString resultIconPath(DriverCommon::ConnFilterResult result)
{
    // Sync with enum DriverCommon::ConnFilterResult
    static const char *const iconPaths[] = {
        ":/icons/accept.png",
        ":/icons/deny.png",
        ":/icons/help.png",
        ":/icons/road_sign.png",
    };

    return iconPaths[result];
}

QString resultText(DriverCommon::ConnFilterResult result)
{
    // Sync with enum DriverCommon::ConnFilterResult
    const QStringList texts = {
        ConnListModel::actionText(/*blocked=*/false),
        ConnListModel::actionText(/*blocked=*/true),
        FilterSimWindow::tr("Ask to Connect"),
        FilterSimWindow::tr("Ignored: other firewalls decide"),
    };

    return texts.value(result);
}

QString ruleText(quint16 ruleId)
{
    return (ruleId != 0) ? confRuleManager()->ruleNameById(ruleId) : QString();
}

QString zoneText(quint8 zoneId)
{
    return (zoneId != 0) ? confZoneManager()->zoneNameById(zoneId) : QString();
}

QString appText(const FORT_APP_DATA &appData)
{
    if (!appData.flags.found)
        return FilterSimWindow::tr("Not found");

    const App app = confAppManager()->appById(appData.app_id);

    // The name may be empty, e.g. till the App's info is looked up
    return !app.appName.isEmpty() ? app.appName : StringUtil::firstLine(app.appOriginPath);
}

}

FilterSimWindow::FilterSimWindow(QWidget *parent) :
    FormWindow(parent), m_ctrl(new FilterSimController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::filterSimWindowGroup());
}

void FilterSimWindow::initialize(const FilterSimConn &simConn)
{
    const FORT_CONF_META_CONN &conn = simConn.conn;

    m_editAppPath->setText(simConn.appPath);
    m_comboDirection->setCurrentIndex(conn.inbound ? 1 : 0);
    m_comboProtocol->setCurrentText(NetUtil::protocolName(conn.ip_proto));
    m_editRemoteIp->setText(NetFormatUtil::ipToText(conn.remote_ip, conn.isIPv6));
    m_spinRemotePort->setValue(conn.remote_port);
    m_editLocalIp->setText(NetFormatUtil::ipToText(conn.local_ip, conn.isIPv6));
    m_spinLocalPort->setValue(conn.local_port);
    m_cbLoopback->setChecked(conn.is_loopback);

    simulateConn();
}

void FilterSimWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setFilterSimWindowGeometry(stateWatcher()->geometry());
    iniUser.setFilterSimWindowMaximized(stateWatcher()->maximized());

    confManager()->saveIniUser();
}

void FilterSimWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    stateWatcher()->restore(this, QSize(600, 500), iniUser.filterSimWindowGeometry(),
            iniUser.filterSimWindowMaximized());
}

void FilterSimWindow::setupController()
{
    connect(ctrl(), &FilterSimController::retranslateUi, this, &FilterSimWindow::retranslateUi);

    emit ctrl() -> retranslateUi();
}

void FilterSimWindow::retranslateUi()
{
    this->unsetLocale();

    m_gbConn->setTitle(tr("Connection"));
    m_labelAppPath->setText(tr("Program Path:"));
    m_btSelectFile->setToolTip(tr("Select File"));
    m_labelDirection->setText(tr("Direction:"));
    retranslateComboDirection();
    m_labelProtocol->setText(tr("Protocol:"));
    m_labelRemoteIp->setText(tr("Remote IP:"));
    m_labelRemotePort->setText(tr("Port:"));
    m_labelLocalIp->setText(tr("Local IP:"));
    m_editLocalIp->setPlaceholderText(tr("Any"));
    m_labelLocalPort->setText(tr("Port:"));
    m_cbLoopback->setText(tr("Loopback"));
    m_cbLoopback->setToolTip(tr("The connection to an address of this computer."
                                " The addresses 127.0.0.0/8 and ::1 are always loopback."));
    m_labelProfile->setText(tr("Network Profile:"));
    retranslateComboProfile();
    m_btSimulate->setText(tr("Simulate"));
    m_gbResult->setTitle(tr("Result"));
    m_labelAction->setText(tr("Action:"));
    m_labelReason->setText(tr("Reason:"));
    m_labelRule->setText(tr("Rule:"));
    m_labelZone->setText(tr("Zone:"));
    m_labelProgram->setText(tr("Program:"));
    m_labelNote->setText(tr("The program is checked by its path: the settings propagated from"
                            " a parent process aren't taken into account."));

    updateResult();

    this->setWindowTitle(tr("Filter Simulator"));
}

void FilterSimWindow::retranslateComboDirection()
{
    const QStringList list = {
        ConnListModel::directionText(/*inbound=*/false),
        ConnListModel::directionText(/*inbound=*/true),
    };

    const int currentIndex = qMax(m_comboDirection->currentIndex(), 0);

    ControlUtil::setComboBoxTexts(m_comboDirection, list, currentIndex);

    ControlUtil::setComboBoxIcons(m_comboDirection,
            { ConnListModel::directionIconPath(/*inbound=*/false),
                    ConnListModel::directionIconPath(/*inbound=*/true) });
}

void FilterSimWindow::retranslateComboProfile()
{
    // Sync with the profile ids: 1 - Public, 2 - Private, 3 - Domain
    const QStringList list = { tr("Public"), tr("Private"), tr("Domain") };

    const int currentIndex = qMax(m_comboProfile->currentIndex(), 0);

    ControlUtil::setComboBoxTexts(m_comboProfile, list, currentIndex);
}

void FilterSimWindow::setupUi()
{
    // Connection
    m_gbConn = new QGroupBox();
    m_gbConn->setLayout(setupConnLayout());

    // Simulate
    m_btSimulate = ControlUtil::createButton(":/icons/play.png", [&] { simulateConn(); });

    // Result
    m_gbResult = new QGroupBox();
    m_gbResult->setLayout(setupResultLayout());

    // Note
    m_labelNote = ControlUtil::createLabel();
    m_labelNote->setWordWrap(true);
    m_labelNote->setForegroundRole(QPalette::PlaceholderText);

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addWidget(m_gbConn);
    layout->addWidget(m_btSimulate, 0, Qt::AlignCenter);
    layout->addWidget(m_gbResult);
    layout->addStretch();
    layout->addWidget(m_labelNote);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *FilterSimWindow::setupConnLayout()
{
    auto layout = new QFormLayout();
    layout->setHorizontalSpacing(10);

    // Program Path
    auto appPathLayout = setupAppPathLayout();

    layout->addRow("Program Path:", appPathLayout);
    m_labelAppPath = ControlUtil::formRowLabel(layout, appPathLayout);

    // Direction
    m_comboDirection = createFixedComboBox();

    layout->addRow("Direction:", m_comboDirection);
    m_labelDirection = ControlUtil::formRowLabel(layout, m_comboDirection);

    // Protocol
    m_comboProtocol = createFixedComboBox({ "TCP", "UDP", "ICMP", "ICMPv6" });
    m_comboProtocol->setEditable(true);

    layout->addRow("Protocol:", m_comboProtocol);
    m_labelProtocol = ControlUtil::formRowLabel(layout, m_comboProtocol);

    // Remote IP & Port
    auto remoteLayout = setupRemoteLayout();

    layout->addRow("Remote IP:", remoteLayout);
    m_labelRemoteIp = ControlUtil::formRowLabel(layout, remoteLayout);

    // Local IP & Port
    auto localLayout = setupLocalLayout();

    layout->addRow("Local IP:", localLayout);
    m_labelLocalIp = ControlUtil::formRowLabel(layout, localLayout);

    // Loopback
    m_cbLoopback = new QCheckBox();

    layout->addRow(QString(), m_cbLoopback);

    // Network Profile
    m_comboProfile = createFixedComboBox();

    layout->addRow("Network Profile:", m_comboProfile);
    m_labelProfile = ControlUtil::formRowLabel(layout, m_comboProfile);

    return layout;
}

QLayout *FilterSimWindow::setupAppPathLayout()
{
    // Path
    m_editAppPath = new LineEdit();
    m_editAppPath->setMaxLength(1024);

    connect(m_editAppPath, &QLineEdit::returnPressed, this, &FilterSimWindow::simulateConn);

    // Select File
    m_btSelectFile = ControlUtil::createIconToolButton(":/icons/folder.png", [&] {
        const auto filePath = DialogUtil::getOpenFileName(
                m_labelAppPath->text(), tr("Programs (*.exe);;All files (*.*)"));

        if (!filePath.isEmpty()) {
            m_editAppPath->setText(FileUtil::toNativeSeparators(filePath));
        }
    });

    auto layout = new QHBoxLayout();
    layout->addWidget(m_editAppPath);
    layout->addWidget(m_btSelectFile);

    return layout;
}

QLayout *FilterSimWindow::setupRemoteLayout()
{
    // IP
    m_editRemoteIp = new LineEdit();
    m_editRemoteIp->setMaxLength(64);

    connect(m_editRemoteIp, &QLineEdit::returnPressed, this, &FilterSimWindow::simulateConn);

    // Port
    m_labelRemotePort = ControlUtil::createLabel();

    m_spinRemotePort = createPortSpinBox();
    m_spinRemotePort->setValue(443);

    return createIpPortLayout(m_editRemoteIp, m_labelRemotePort, m_spinRemotePort);
}

QLayout *FilterSimWindow::setupLocalLayout()
{
    // IP
    m_editLocalIp = new LineEdit();
    m_editLocalIp->setMaxLength(64);

    connect(m_editLocalIp, &QLineEdit::returnPressed, this, &FilterSimWindow::simulateConn);

    // Port
    m_labelLocalPort = ControlUtil::createLabel();

    m_spinLocalPort = createPortSpinBox();

    return createIpPortLayout(m_editLocalIp, m_labelLocalPort, m_spinLocalPort);
}

QLayout *FilterSimWindow::setupResultLayout()
{
    auto layout = new QFormLayout();
    layout->setHorizontalSpacing(10);

    // Action
    auto actionLayout = setupResultActionLayout();

    layout->addRow("Action:", actionLayout);
    m_labelAction = ControlUtil::formRowLabel(layout, actionLayout);

    // Reason
    m_resultReason = createResultLabel();

    layout->addRow("Reason:", m_resultReason);
    m_labelReason = ControlUtil::formRowLabel(layout, m_resultReason);

    // Rule
    m_resultRule = createResultLabel();

    layout->addRow("Rule:", m_resultRule);
    m_labelRule = ControlUtil::formRowLabel(layout, m_resultRule);

    // Zone
    m_resultZone = createResultLabel();

    layout->addRow("Zone:", m_resultZone);
    m_labelZone = ControlUtil::formRowLabel(layout, m_resultZone);

    // Program
    m_resultProgram = createResultLabel();

    layout->addRow("Program:", m_resultProgram);
    m_labelProgram = ControlUtil::formRowLabel(layout, m_resultProgram);

    return layout;
}

QLayout *FilterSimWindow::setupResultActionLayout()
{
    m_iconAction = ControlUtil::createLabel();
    m_iconAction->setFixedSize(resultIconSize);

    m_resultAction = createResultLabel();
    m_resultAction->setFont(GuiUtil::fontBold());

    auto layout = new QHBoxLayout();
    layout->addWidget(m_iconAction);
    layout->addWidget(m_resultAction, 1);

    return layout;
}

void FilterSimWindow::simulateConn()
{
    // Clear the result for a moment to show that the simulation is run again
    m_simulated = false;
    clearResult();

    QTimer::singleShot(SIMULATE_DELAY_MSEC, this, &FilterSimWindow::simulateConnNow);
}

void FilterSimWindow::simulateConnNow()
{
    FilterSimConn simConn;

    m_simulated = fillSimConn(simConn) && ctrl()->simulateConn(simConn);
    m_simConn = simConn;

    updateResult();
}

bool FilterSimWindow::fillSimConn(FilterSimConn &simConn) const
{
    FORT_CONF_META_CONN &conn = simConn.conn;

    if (!fillConnAddresses(conn) || !fillConnProtocol(conn))
        return false;

    conn.inbound = (m_comboDirection->currentIndex() == 1);
    conn.profile_id = m_comboProfile->currentIndex() + 1;
    conn.local_port = m_spinLocalPort->value();
    conn.remote_port = m_spinRemotePort->value();

    simConn.appPath = m_editAppPath->text().trimmed();

    return true;
}

bool FilterSimWindow::fillConnAddresses(FORT_CONF_META_CONN &conn) const
{
    bool isIPv6 = false;

    if (!textToIp(m_editRemoteIp->text(), conn.remote_ip, isIPv6)) {
        showInputError(tr("Invalid remote IP address"));
        return false;
    }

    conn.isIPv6 = isIPv6;
    conn.is_loopback = m_cbLoopback->isChecked() || isLoopbackIp(conn.remote_ip, isIPv6);

    const QString localIpText = m_editLocalIp->text().trimmed();
    if (localIpText.isEmpty())
        return true;

    bool isLocalIPv6 = false;

    if (!textToIp(localIpText, conn.local_ip, isLocalIPv6) || isLocalIPv6 != isIPv6) {
        showInputError(tr("Invalid local IP address"));
        return false;
    }

    return true;
}

bool FilterSimWindow::fillConnProtocol(FORT_CONF_META_CONN &conn) const
{
    quint8 ipProto = 0;

    if (!textToProtocol(m_comboProtocol->currentText(), ipProto)) {
        showInputError(tr("Invalid protocol"));
        return false;
    }

    conn.ip_proto = ipProto;

    return true;
}

void FilterSimWindow::updateResult()
{
    if (!m_simulated) {
        clearResult();
        return;
    }

    const FORT_CONF_META_CONN &conn = m_simConn.conn;

    m_iconAction->setPixmap(IconCache::pixmap(resultIconPath(m_simConn.result), resultIconSize));
    m_resultAction->setText(resultText(m_simConn.result));
    m_resultReason->setText(ConnListModel::reasonText(FortConnReason(conn.reason)));
    m_resultRule->setText(ruleText(conn.rule_id));
    m_resultZone->setText(zoneText(conn.act.zone_id));
    m_resultProgram->setText(appText(conn.app_data));
}

void FilterSimWindow::clearResult()
{
    m_iconAction->clear();
    m_resultAction->clear();
    m_resultReason->clear();
    m_resultRule->clear();
    m_resultZone->clear();
    m_resultProgram->clear();
}

void FilterSimWindow::showInputError(const QString &text) const
{
    windowManager()->showErrorBox(text, tr("Filter Simulator"));
}
