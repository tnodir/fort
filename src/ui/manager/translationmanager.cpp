#include "translationmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QTranslator>

#include <conf/confmanager.h>
#include <user/iniuser.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/stringutil.h>

namespace {

const QLatin1String translationFilePrefix("i18n_");
const QLatin1String translationFileSuffix(".qm");

}

TranslationManager::TranslationManager(QObject *parent) : QObject(parent)
{
    setupTranslation();
}

TranslationManager::~TranslationManager()
{
    uninstallAllTranslators();
}

QString TranslationManager::languageName() const
{
    return isSystemLanguage() ? QString() : m_locale.name();
}

void TranslationManager::setUp()
{
    auto confManager = IoCDependency<ConfManager>();

    connect(confManager, &ConfManager::iniUserChanged, this, &TranslationManager::setupByIniUser);

    setupByIniUser(confManager->iniUser());
}

void TranslationManager::setupTranslation()
{
    setupLocales();

    // Translators will be loaded later when needed
    m_translators.resize(m_locales.size());
    m_translators.fill(nullptr);
}

void TranslationManager::setupLocales()
{
    // Collect locales from i18n files
    const auto i18nFileInfos = QDir(i18nDir()).entryInfoList({ ('*' + translationFileSuffix) });

    const int prefixLen = translationFilePrefix.size();

    const QLocale systemLocale = QLocale::system();
    const QStringList uiLanguages = systemLocale.uiLanguages();

    m_locales.append(!uiLanguages.isEmpty() ? QLocale(uiLanguages.first()) : systemLocale);

    m_locales.append(QLocale(QLocale::English, QLocale::UnitedStates));
    constexpr int preLocalesCount = 2;

    int localeBit = (1 << preLocalesCount);
    for (const QFileInfo &fileInfo : i18nFileInfos) {
        const QString localeName = fileInfo.completeBaseName().mid(prefixLen);
        const QLocale locale(localeName);

        m_locales.append(locale);
        m_localesWithCountry |= localeName.size() > 2 ? localeBit : 0;
        localeBit <<= 1;
    }
}

QStringList TranslationManager::displayLabels() const
{
    QStringList list;
    list.reserve(m_locales.size());

    list.append(systemLocaleDisplay());

    int localeBit = (1 << 1);
    int index = 1; // skip System locale
    const int localesSize = m_locales.size();
    for (; index < localesSize; ++index) {
        const QLocale &locale = m_locales[index];
        const bool isWithCountry = (m_localesWithCountry & localeBit) != 0;
        localeBit <<= 1;

        const QString label = localeDisplay(locale, isWithCountry);

        list.append(label);
    }

    return list;
}

int TranslationManager::getLanguageByName(const QString &langName) const
{
    if (langName.isEmpty())
        return 0;

    int index = 1; // skip System locale
    const int localesSize = m_locales.size();
    for (; index < localesSize; ++index) {
        const QLocale &locale = m_locales[index];
        if (langName == locale.name() || langName == locale.bcp47Name()) {
            return index;
        }
    }
    return 0;
}

void TranslationManager::setLanguage(int language)
{
    if (m_language == language)
        return;

    uninstallTranslator(m_language);
    installTranslator(language, m_locale);

    m_language = language;
    emit languageChanged();
}

bool TranslationManager::switchLanguage(int language)
{
    if (language < 0 || language >= m_locales.size())
        return false;

    m_locale = m_locales.at(language);

    setupDefaultLocale();

    setLanguage(language);

    return true;
}

bool TranslationManager::switchLanguageByName(const QString &languageName)
{
    return switchLanguage(getLanguageByName(languageName));
}

void TranslationManager::uninstallAllTranslators()
{
    const int translatorsCount = m_translators.size();
    for (int i = 0; i < translatorsCount; ++i) {
        uninstallTranslator(i);
    }
}

void TranslationManager::uninstallTranslator(int language)
{
    if (language < 0 || language >= m_translators.size())
        return;

    QTranslator *translator = m_translators.at(language);
    if (translator) {
        QCoreApplication::removeTranslator(translator);
    }
}

void TranslationManager::installTranslator(int language, const QLocale &locale)
{
    QTranslator *translator = m_translators.at(language);
    if (!translator) {
        translator = loadTranslator(language, locale);
        m_translators.replace(language, translator);
    }
    if (translator) {
        QCoreApplication::installTranslator(translator);
    }
}

QTranslator *TranslationManager::loadTranslator(int language, const QLocale &locale)
{
    if (language == 1)
        return nullptr; // English

    // Load .qm file
    auto translator = new QTranslator(this);
    if (!translator->load(translationFilePrefix + locale.name(), i18nDir())) {
        delete translator;
        return nullptr;
    }

    return translator;
}

void TranslationManager::setupDefaultLocale()
{
    QLocale::setDefault(m_useSystemLocale ? QLocale::system() : m_locale);
}

void TranslationManager::setupByIniUser(const IniUser &ini)
{
    m_useSystemLocale = ini.useSystemLocale();

    switchLanguageByName(ini.language());
}

QString TranslationManager::systemLocaleDisplay()
{
    return "System, " + tr("System Language");
}

QString TranslationManager::localeDisplay(const QLocale &locale, bool isWithCountry)
{
    QString label = QLocale::languageToString(locale.language());
    QString nativeLabel = StringUtil::capitalize(locale.nativeLanguageName());

    if (isWithCountry) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        label += " (" + QLocale::territoryToString(locale.territory()) + ")";
        nativeLabel += " (" + StringUtil::capitalize(locale.nativeTerritoryName()) + ")";
#else
        label += " (" + QLocale::countryToString(locale.country()) + ")";
        nativeLabel += " (" + StringUtil::capitalize(locale.nativeCountryName()) + ")";
#endif
    }

    const QString langName = isWithCountry ? locale.name() : locale.bcp47Name();

    return label + ", " + nativeLabel + " - " + langName;
}

QString TranslationManager::i18nDir()
{
    return FileUtil::appBinLocation() + "/i18n";
}
