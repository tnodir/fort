#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QObject>
#include <QStringList>
#include <QHash>

class QTranslator;

class TranslationManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dummyBool READ dummyBool NOTIFY dummyBoolChanged)
    Q_PROPERTY(Language language READ language WRITE switchLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString langName READ langName WRITE setLangName NOTIFY languageChanged)

protected:
    explicit TranslationManager(QObject *parent = 0);
    virtual ~TranslationManager();

public:
    enum Language {
        NoneLanguage = -1,
        English = 0,
        Russian,
        Uzbek,
        LanguageCount
    };
    Q_ENUM(Language)

    static TranslationManager *instance();

    bool dummyBool() const { return true; }

    TranslationManager::Language language() const { return m_language; }

    QString langName() const;
    void setLangName(const QString &langName);

    static TranslationManager::Language getLanguageByName(const QString &langName);

    Q_INVOKABLE QString getLangName(TranslationManager::Language language = NoneLanguage) const;
    Q_INVOKABLE QString getLabel(TranslationManager::Language language = NoneLanguage) const;
    Q_INVOKABLE QString getNaturalLabel(TranslationManager::Language language = NoneLanguage) const;

    Q_INVOKABLE static QStringList getLangNames();
    Q_INVOKABLE static QStringList getLabels();
    Q_INVOKABLE static QStringList getNaturalLabels();

    static QString defaultLangName() { return QLatin1String("en"); }

signals:
    void dummyBoolChanged();
    void languageChanged(TranslationManager::Language language);

public slots:
    void switchLanguage(TranslationManager::Language language = English);
    void switchLanguageByName(const QString &langName);

    void refreshTranslations();

private:
    void installTranslator(Language language);
    void uninstallTranslator(Language language);

    QTranslator *loadTranslator(Language language);

    static QLocale getLocale(Language language);

private:
    Language m_language;

    QString m_i18nDir;

    QHash<Language, QTranslator *> m_translators;
};

#endif // TRANSLATIONMANAGER_H
