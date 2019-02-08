#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QLocale>
#include <QObject>
#include <QStringList>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QTranslator)

class TranslationManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool trTrigger READ trTrigger NOTIFY trTriggerChanged)
    Q_PROPERTY(int language READ language WRITE switchLanguage NOTIFY languageChanged)
    Q_PROPERTY(QStringList naturalLabels READ naturalLabels CONSTANT)

protected:
    explicit TranslationManager(QObject *parent = nullptr);
    ~TranslationManager() override;

public:
    static TranslationManager *instance();

    bool trTrigger() const { return true; }

    int language() const { return m_language; }
    QString localeName() const { return m_locale.name(); }

    QStringList naturalLabels() const;

    int getLanguageByName(const QString &localeName) const;

signals:
    void trTriggerChanged();
    void languageChanged(int language);

public slots:
    bool switchLanguage(int language = 0);
    bool switchLanguageByName(const QString &localeName);

    void refreshTranslations();

private:
    void setupTranslation();

    void uninstallAllTranslators();
    void uninstallTranslator(int language);

    void installTranslator(int language, const QLocale &locale);

    QTranslator *loadTranslator(int language, const QLocale &locale);

    static QString i18nDir();

private:
    int m_language;

    QLocale m_locale;

    QList<QLocale> m_locales;
    QVector<QTranslator *> m_translators;
};

#endif // TRANSLATIONMANAGER_H
