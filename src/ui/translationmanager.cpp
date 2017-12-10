#include "translationmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QTranslator>

#include "util/stringutil.h"

#define TRANSLATION_FILE_PREFIX     "i18n_"
#define TRANSLATION_FILE_SUFFIX     ".qm"

TranslationManager::TranslationManager(QObject *parent) :
    QObject(parent),
    m_language(0)
{
    setupTranslation();
}

TranslationManager::~TranslationManager()
{
    uninstallAllTranslators();
}

TranslationManager *TranslationManager::instance()
{
    static TranslationManager *g_instanceTranslationManager = nullptr;

    if (!g_instanceTranslationManager) {
        g_instanceTranslationManager = new TranslationManager();
    }
    return g_instanceTranslationManager;
}

void TranslationManager::setupTranslation()
{
    // Collect locales from i18n files
    const int prefixLen = QLatin1String(TRANSLATION_FILE_PREFIX).size();

    m_locales.append(QLocale(QLocale::English, QLocale::UnitedStates));

    foreach (const QFileInfo &fileInfo, QDir(i18nDir())
             .entryInfoList(QStringList() << ("*" TRANSLATION_FILE_SUFFIX))) {
        const QString localeName = fileInfo.completeBaseName().mid(prefixLen);
        const QLocale locale(localeName);

        m_locales.append(locale);
    }

    // Translators will be loaded later when needed
    m_translators.resize(m_locales.size());
    m_translators.fill(nullptr);
}

QStringList TranslationManager::naturalLabels() const
{
    QStringList list;
    list.reserve(m_locales.size());

    foreach (const QLocale &locale, m_locales) {
        list.append(StringUtil::capitalize(
                        locale.nativeLanguageName()));
    }
    return list;
}

int TranslationManager::getLanguageByName(const QString &localeName) const
{
    int index = 0;
    foreach (const QLocale &locale, m_locales) {
        if (localeName == locale.name()) {
            return index;
        }
        ++index;
    }
    return 0;
}

bool TranslationManager::switchLanguage(int language)
{
    if (language < 0 || language >= m_locales.size())
        return false;

    m_locale = m_locales.at(language);

    QLocale::setDefault(m_locale);

    uninstallTranslator(m_language);

    m_language = language;
    emit languageChanged(m_language);

    installTranslator(m_language, m_locale);

    refreshTranslations();

    return true;
}

bool TranslationManager::switchLanguageByName(const QString &localeName)
{
    return switchLanguage(getLanguageByName(localeName));
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
    QTranslator *translator = m_translators.at(language);
    if (translator) {
        qApp->removeTranslator(translator);
    }
}

void TranslationManager::installTranslator(int language, const QLocale &locale)
{
    QTranslator *translator = m_translators.at(language);
    if (!translator) {
        translator = loadTranslator(language, locale);
    }
    if (translator) {
        qApp->installTranslator(translator);
    }
}

QTranslator *TranslationManager::loadTranslator(int language, const QLocale &locale)
{
    if (!language)
        return nullptr;

    // Load .qm file
    QTranslator *translator = new QTranslator(this);
    translator->load(TRANSLATION_FILE_PREFIX + locale.name(), i18nDir());

    m_translators.replace(language, translator);

    return translator;
}

void TranslationManager::refreshTranslations()
{
    emit dummyBoolChanged();
}

QString TranslationManager::i18nDir()
{
    return qApp->applicationDirPath() + "/i18n";
}
