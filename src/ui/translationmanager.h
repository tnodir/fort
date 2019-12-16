#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QLocale>
#include <QObject>
#include <QStringList>
#include <QVector>

#include "util/classhelpers.h"

QT_FORWARD_DECLARE_CLASS(QTranslator)

class TranslationManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int language READ language WRITE switchLanguage NOTIFY languageChanged)
    Q_PROPERTY(QStringList naturalLabels READ naturalLabels CONSTANT)

protected:
    explicit TranslationManager(QObject *parent = nullptr);
    ~TranslationManager() override;
    CLASS_DELETE_COPY_MOVE(TranslationManager)

public:
    static TranslationManager *instance();

    int language() const { return m_language; }
    QString localeName() const { return m_locale.name(); }

    QStringList naturalLabels() const;

    int getLanguageByName(const QString &localeName) const;

signals:
    void languageChanged();

public slots:
    bool switchLanguage(int language = 0);
    bool switchLanguageByName(const QString &localeName);

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

    QVector<QLocale> m_locales;
    QVector<QTranslator *> m_translators;
};

#endif // TRANSLATIONMANAGER_H
