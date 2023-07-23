#include "translationmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QTranslator>

#include <conf/confmanager.h>
#include <user/iniuser.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/stringutil.h>

#define TRANSLATION_FILE_PREFIX "i18n_"
#define TRANSLATION_FILE_SUFFIX ".qm"

TranslationManager::TranslationManager(QObject *parent) : QObject(parent)
{
    setupTranslation();
}

TranslationManager::~TranslationManager()
{
    uninstallAllTranslators();
}

void TranslationManager::setUp()
{
    auto confManager = IoC()->setUpDependency<ConfManager>();

    connect(confManager, &ConfManager::iniUserChanged, this, &TranslationManager::setupByIniUser);
}

void TranslationManager::setupTranslation()
{
    // Collect locales from i18n files
    const int prefixLen = QLatin1String(TRANSLATION_FILE_PREFIX).size();

    m_locales.append(QLocale(QLocale::English, QLocale::UnitedStates));

    const auto i18nFileInfos =
            QDir(i18nDir()).entryInfoList(QStringList() << ("*" TRANSLATION_FILE_SUFFIX));

    int localeBit = 2;
    for (const QFileInfo &fileInfo : i18nFileInfos) {
        const QString localeName = fileInfo.completeBaseName().mid(prefixLen);
        const QLocale locale(localeName);

        m_locales.append(locale);
        m_localesWithCountry |= localeName.size() > 2 ? localeBit : 0;
        localeBit <<= 1;
    }

    // Translators will be loaded later when needed
    m_translators.resize(m_locales.size());
    m_translators.fill(nullptr);
}

QStringList TranslationManager::displayLabels() const
{
    QStringList list;
    list.reserve(m_locales.size());

    int localeBit = 1;
    for (const QLocale &locale : m_locales) {
        const bool isWithCountry = (m_localesWithCountry & localeBit) != 0;
        localeBit <<= 1;

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

        list.append(label + ", " + nativeLabel + " - " + langName);
    }
    return list;
}

int TranslationManager::getLanguageByName(const QString &langName) const
{
    int index = 0;
    for (const QLocale &locale : m_locales) {
        if (langName == locale.name() || langName == locale.bcp47Name()) {
            return index;
        }
        ++index;
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

bool TranslationManager::switchLanguageByName(const QString &langName)
{
    return switchLanguage(getLanguageByName(langName));
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
    if (language == 0)
        return nullptr;

    // Load .qm file
    auto translator = new QTranslator(this);
    if (!translator->load(TRANSLATION_FILE_PREFIX + locale.name(), i18nDir())) {
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

QString TranslationManager::i18nDir()
{
    return FileUtil::appBinLocation() + "/i18n";
}
