#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <conf/addressgroup.h>
#include <conf/appgroup.h>
#include <conf/firewallconf.h>
#include <driver/drivercommon.h>
#include <log/logentryblockedip.h>
#include <manager/envmanager.h>
#include <util/conf/confappswalker.h>
#include <util/conf/confbuffer.h>
#include <util/fileutil.h>
#include <util/net/netutil.h>
#include <util/stringutil.h>

class ConfUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void ConfUtilTest::SetUp() { }

void ConfUtilTest::TearDown() { }

TEST_F(ConfUtilTest, confWriteRead)
{
    EnvManager envManager;
    FirewallConf conf;

    AddressGroup *inetGroup = conf.inetAddressGroup();

    inetGroup->setIncludeAll(true);
    inetGroup->setExcludeAll(false);

    inetGroup->setIncludeText(QString());
    inetGroup->setExcludeText(NetUtil::localIpNetworksText());

    conf.setAppBlockAll(true);
    conf.setAppAllowAll(false);

    AppGroup *appGroup1 = new AppGroup();
    appGroup1->setName("Base");
    appGroup1->setEnabled(true);
    appGroup1->setPeriodEnabled(true);
    appGroup1->setPeriodFrom("00:00");
    appGroup1->setPeriodTo("12:00");
    appGroup1->setBlockText("System");
    appGroup1->setAllowText("C:\\Program Files\\Skype\\Phone\\Skype.exe\n"
                            "?:\\Utils\\Dev\\Git\\**\n"
                            "D:\\**\\Programs\\**\n");

    AppGroup *appGroup2 = new AppGroup();
    appGroup2->setName("Browser");
    appGroup2->setEnabled(false);
    appGroup2->setAllowText("C:\\Utils\\Firefox\\Bin\\firefox.exe");
    appGroup2->setLimitInEnabled(true);
    appGroup2->setSpeedLimitIn(1024);

    conf.addAppGroup(appGroup1);
    conf.addAppGroup(appGroup2);

    conf.resetEdited(FirewallConf::AllEdited);
    conf.prepareToSave();

    ConfBuffer confBuf;

    if (!confBuf.write(conf, nullptr, envManager)) {
        qCritical() << "Error:" << confBuf.errorMessage();
        Q_UNREACHABLE();
    }

    // Check the buffer
    const char *data = confBuf.data() + DriverCommon::confIoConfOff();

    ASSERT_FALSE(DriverCommon::confIp4InRange(data, 0, true));
    ASSERT_FALSE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("9.255.255.255")));
    ASSERT_FALSE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("11.0.0.0")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("10.0.0.0")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("169.254.100.100")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("192.168.255.255")));
    ASSERT_FALSE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("193.0.0.0")));
    ASSERT_TRUE(DriverCommon::confIp4InRange(data, NetUtil::textToIp4("239.255.255.250")));
    ASSERT_TRUE(DriverCommon::confIp6InRange(data, NetUtil::textToIp6("::1")));
    ASSERT_TRUE(DriverCommon::confIp6InRange(data, NetUtil::textToIp6("::2")));
    ASSERT_TRUE(DriverCommon::confIp6InRange(data, NetUtil::textToIp6("::ffff:0:2")));
    ASSERT_FALSE(DriverCommon::confIp6InRange(data, NetUtil::textToIp6("65::")));

    ASSERT_TRUE(DriverCommon::confAppFind(data, "System").found);

    ASSERT_TRUE(DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("C:\\Program Files\\Skype\\Phone\\Skype.exe"))
                    .found);
    ASSERT_TRUE(DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("C:\\Utils\\Dev\\Git\\git.exe"))
                    .found);
    ASSERT_TRUE(DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("D:\\Utils\\Dev\\Git\\bin\\git.exe"))
                    .found);
    ASSERT_TRUE(DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("D:\\My\\Programs\\Test.exe"))
                    .found);

    ASSERT_FALSE(DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("C:\\Program Files\\Test.exe"))
                    .found);

    const auto firefoxData = DriverCommon::confAppFind(
            data, FileUtil::pathToKernelPath("C:\\Utils\\Firefox\\Bin\\firefox.exe"));
    ASSERT_EQ(int(firefoxData.flags.group_index), 1);
}

TEST_F(ConfUtilTest, checkEnvManager)
{
    EnvManager envManager;

    envManager.setCachedEnvVar("a", "a");
    envManager.setCachedEnvVar("b", "b");
    envManager.setCachedEnvVar("c", "c");

    ASSERT_EQ(envManager.expandString("%%%a%%b%%c%-%c%%b%%a%%%"), "%abc-cba%");

    envManager.setCachedEnvVar("d", "%e%");
    envManager.setCachedEnvVar("e", "%f%");
    envManager.setCachedEnvVar("f", "%d%");

    ASSERT_EQ(envManager.expandString("%d%"), QString());

    envManager.setCachedEnvVar("d", "%e%");
    envManager.setCachedEnvVar("e", "%f%");
    envManager.setCachedEnvVar("f", "%a%");

    ASSERT_EQ(envManager.expandString("%d%"), "a");

    ASSERT_NE(envManager.expandString("%HOME%"), QString());
}

TEST_F(ConfUtilTest, serviceSidsWriteRead)
{
    const auto SVC = [](const char *name) -> ServiceInfo {
        ServiceInfo si;
        si.serviceName = si.realServiceName = name;
        return si;
    };

    const QVector<ServiceInfo> services = { SVC("AppIDSvc"), SVC("Appinfo"), SVC("AppMgmt"),
        SVC("AppReadiness"), SVC("AppXSvc"), SVC("AudioEndpointBuilder"), SVC("AxInstSV"),
        SVC("BFE"), SVC("BITS"), SVC("BrokerInfrastructure"), SVC("BTAGService"),
        SVC("BthAvctpSvc"), SVC("bthserv"), SVC("camsvc"), SVC("CDPSvc"), SVC("CertPropSvc"),
        SVC("ClipSVC"), SVC("CoreMessagingRegistrar"), SVC("DcomLaunch"),
        SVC("DeviceAssociationService"), SVC("DeviceInstall"), SVC("DevQueryBroker"), SVC("Dhcp"),
        SVC("DialogBlockingService"), SVC("DispBrokerDesktopSvc"), SVC("DisplayEnhancementService"),
        SVC("dot3svc"), SVC("DPS"), SVC("DsmSvc"), SVC("DsSvc"), SVC("EapHost"),
        SVC("embeddedmode"), SVC("EntAppSvc"), SVC("EventLog"), SVC("EventSystem"), SVC("fdPHost"),
        SVC("FDResPub"), SVC("fhsvc"), SVC("FontCache"), SVC("FrameServer"), SVC("hidserv"),
        SVC("icssvc"), SVC("IKEEXT"), SVC("iphlpsvc"), SVC("IpxlatCfgSvc"), SVC("KtmRm"),
        SVC("LanmanServer"), SVC("LanmanWorkstation"), SVC("lfsvc"), SVC("LicenseManager"),
        SVC("lltdsvc"), SVC("lmhosts"), SVC("LSM"), SVC("LxpSvc"), SVC("mpssvc"), SVC("MSiSCSI"),
        SVC("MsKeyboardFilter"), SVC("NaturalAuthentication"), SVC("NcaSvc"), SVC("NcbService"),
        SVC("NcdAutoSetup"), SVC("Netman"), SVC("netprofm"), SVC("NetSetupSvc"), SVC("NgcCtnrSvc"),
        SVC("NgcSvc"), SVC("NlaSvc"), SVC("nsi"), SVC("PcaSvc"), SVC("PhoneSvc"), SVC("pla"),
        SVC("PlugPlay"), SVC("PolicyAgent"), SVC("Power"), SVC("PrintDeviceConfigurationService"),
        SVC("PrintScanBrokerService"), SVC("ProfSvc"), SVC("PushToInstall"), SVC("QWAVE"),
        SVC("RasAuto"), SVC("RasMan"), SVC("RemoteAccess"), SVC("RemoteRegistry"), SVC("RmSvc"),
        SVC("RpcEptMapper"), SVC("RpcSs"), SVC("SCardSvr"), SVC("ScDeviceEnum"), SVC("Schedule"),
        SVC("SCPolicySvc"), SVC("seclogon"), SVC("SENS"), SVC("SensorService"), SVC("SensrSvc"),
        SVC("SessionEnv"), SVC("SharedAccess"), SVC("ShellHWDetection"), SVC("shpamsvc"),
        SVC("SmsRouter"), SVC("SSDPSRV"), SVC("SstpSvc"), SVC("StateRepository"), SVC("StorSvc"),
        SVC("svsvc"), SVC("SysMain"), SVC("SystemEventsBroker"), SVC("TapiSrv"), SVC("TermService"),
        SVC("TextInputManagementService"), SVC("Themes"), SVC("TimeBrokerSvc"), SVC("TokenBroker"),
        SVC("TrkWks"), SVC("TroubleshootingSvc"), SVC("tzautoupdate"), SVC("UmRdpService"),
        SVC("upnphost"), SVC("UserManager"), SVC("UsoSvc"), SVC("vmicguestinterface"),
        SVC("vmicheartbeat"), SVC("vmickvpexchange"), SVC("vmicrdv"), SVC("vmicshutdown"),
        SVC("vmictimesync"), SVC("vmicvmsession"), SVC("W32Time"), SVC("WaaSMedicSvc"),
        SVC("WbioSrvc"), SVC("wcncsvc"), SVC("WdiServiceHost"), SVC("WdiSystemHost"),
        SVC("WebClient"), SVC("webthreatdefsvc"), SVC("Wecsvc"), SVC("WEPHOSTSVC"),
        SVC("wercplsupport"), SVC("WFDSConMgrSvc"), SVC("WiaRpc"), SVC("WinHttpAutoProxySvc"),
        SVC("Winmgmt"), SVC("WinRM"), SVC("wisvc"), SVC("wlidsvc"), SVC("wlpasvc"), SVC("WManSvc"),
        SVC("WPDBusEnum"), SVC("WpnService"), SVC("wuauserv"), SVC("WwanSvc"),
        SVC("XblAuthManager"), SVC("XblGameSave"), SVC("XboxGipSvc"), SVC("XboxNetApiSvc"),
        SVC("AarSvc"), SVC("BcastDVRUserService"), SVC("CaptureService"), SVC("cbdhsvc"),
        SVC("CDPUserSvc"), SVC("ConsentUxUserSvc"), SVC("DeviceAssociationBrokerSvc"),
        SVC("DevicePickerUserSvc"), SVC("DevicesFlowUserSvc"), SVC("MessagingService"),
        SVC("NPSMSvc"), SVC("PimIndexMaintenanceSvc"), SVC("PrintWorkflowUserSvc"),
        SVC("UdkUserSvc"), SVC("UnistoreSvc"), SVC("UserDataSvc"), SVC("webthreatdefusersvc"),
        SVC("WpnUserService") };

    ConfBuffer confBuf;

    confBuf.writeServiceSids(services);

    // Check the buffer
    const auto buffer = confBuf.buffer();

    ASSERT_NE(buffer.size(), 0);

    PCFORT_SERVICE_SID_LIST service_sids = PCFORT_SERVICE_SID_LIST(buffer.data());

    // Find Service Name by SID
    for (const auto &si : services) {
        const auto sid = StringUtil::serviceSid(si.realServiceName);
        const auto name = DriverCommon::confServiceSidNameFind(service_sids, sid);

        ASSERT_EQ(name, si.serviceName.toLower());
    }

    // Find Invalid Service Name by SID
    {
        const auto sid = StringUtil::serviceSid("Bad");

        ASSERT_EQ(sid.size(), FORT_SERVICE_SID_SIZE);
        ASSERT_EQ(DriverCommon::confServiceSidNameFind(service_sids, sid), QString());
    }
}
