#include "translationmanager.h"

#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

TranslationManager::TranslationManager(QObject *parent) :
    QObject(parent),
    m_language(English),
    m_i18nDir(":/i18n")
{
}

TranslationManager::~TranslationManager()
{
    switchLanguage(English);
}

TranslationManager *TranslationManager::instance()
{
    static TranslationManager *g_instanceTranslationManager = 0;

    if (!g_instanceTranslationManager) {
        g_instanceTranslationManager = new TranslationManager();
    }
    return g_instanceTranslationManager;
}

void TranslationManager::switchLanguage(TranslationManager::Language language)
{
    QLocale::setDefault(getLocale(language));

    uninstallTranslator(m_language);
    installTranslator(language);

    m_language = language;
    emit languageChanged(m_language);

    refreshTranslations();
}

void TranslationManager::switchLanguageByName(const QString &langName)
{
    return switchLanguage(getLanguageByName(langName));
}

void TranslationManager::installTranslator(Language language)
{
    QTranslator *translator = m_translators.value(language);
    if (!translator) {
        translator = loadTranslator(language);
    }
    if (translator) {
        qApp->installTranslator(translator);
    }
}

void TranslationManager::uninstallTranslator(Language language)
{
    QTranslator *translator = m_translators.value(language);
    if (translator) {
        qApp->removeTranslator(translator);
    }
}

QTranslator *TranslationManager::loadTranslator(Language language)
{
    const QString langName = getLangName(language);

    // Load .qm file
    QTranslator *translator = new QTranslator(this);
    translator->load(QLatin1String("i18n_") + langName, m_i18nDir);

    m_translators.insert(language, translator);

    return translator;
}

void TranslationManager::refreshTranslations()
{
    emit dummyBoolChanged();
}

QString TranslationManager::langName() const
{
    return getLangName(m_language);
}

void TranslationManager::setLangName(const QString &langName)
{
    const int i = getLangNames().indexOf(langName);
    if (i >= 0 && i < LanguageCount)
        switchLanguage(static_cast<Language>(i));
}

TranslationManager::Language TranslationManager::getLanguageByName(const QString &langName)
{
    return static_cast<Language>(getLangNames().indexOf(langName));
}

QString TranslationManager::getLangName(TranslationManager::Language language) const
{
    return getLangNames().at(language != NoneLanguage
            ? language : m_language);
}

QString TranslationManager::getLabel(TranslationManager::Language language) const
{
    return getLabels().at(language != NoneLanguage
            ? language : m_language);
}

QString TranslationManager::getNaturalLabel(TranslationManager::Language language) const
{
    return getNaturalLabels().at(language != NoneLanguage
            ? language : m_language);
}

QStringList TranslationManager::getLangNames()
{
    static QStringList g_langNames = QStringList()
            << "en" << "ru" << "uz";
    return g_langNames;
}

QStringList TranslationManager::getLabels()
{
    static QStringList g_labels = QStringList()
            << "American" << "Russian" << "Uzbek";
    return g_labels;
}

QStringList TranslationManager::getNaturalLabels()
{
    static QStringList g_naturalLabels = QStringList()
            << "English language" << "Русский язык" << "Ўзбек тили";
    return g_naturalLabels;
}

QLocale TranslationManager::getLocale(Language language)
{
    switch (language) {
    case English:
        return QLocale(QLocale::English, QLocale::UnitedStates);
    case Russian:
        return QLocale(QLocale::Russian);
    case Uzbek:
        return QLocale(QLocale::Uzbek);
    default:
        Q_UNREACHABLE();
        return QLocale();
    }
}
