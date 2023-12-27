#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QLocale>
#include <QObject>
#include <QStringList>
#include <QVector>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>

QT_FORWARD_DECLARE_CLASS(QTranslator)

class IniUser;

class TranslationManager : public QObject, public IocService
{
    Q_OBJECT
    Q_PROPERTY(int language READ language WRITE switchLanguage NOTIFY languageChanged)

public:
    explicit TranslationManager(QObject *parent = nullptr);
    ~TranslationManager() override;
    CLASS_DELETE_COPY_MOVE(TranslationManager)

    int language() const { return m_language; }
    bool isSystemLanguage() const { return m_language == 0; }

    QString languageName() const;

    void setUp() override;

    QStringList displayLabels() const;

    int getLanguageByName(const QString &langName) const;

signals:
    void languageChanged();

public slots:
    bool switchLanguage(int language = 0);
    bool switchLanguageByName(const QString &languageName);

private:
    void setupTranslation();
    void setupLocales();

    void setLanguage(int language);

    void uninstallAllTranslators();
    void uninstallTranslator(int language);

    void installTranslator(int language, const QLocale &locale);

    QTranslator *loadTranslator(int language, const QLocale &locale);

    void setupDefaultLocale();

    void setupByIniUser(const IniUser &ini);

    static QString systemLocaleDisplay();
    static QString localeDisplay(const QLocale &locale, bool isWithCountry);

    static QString i18nDir();

private:
    bool m_useSystemLocale = false;

    int m_language = -1;

    QLocale m_locale;

    quint32 m_localesWithCountry = 0;
    QVector<QLocale> m_locales;
    QVector<QTranslator *> m_translators;
};

#endif // TRANSLATIONMANAGER_H
