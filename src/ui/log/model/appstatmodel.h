#ifndef APPSTATMODEL_H
#define APPSTATMODEL_H

#include "stringlistmodel.h"

QT_FORWARD_DECLARE_CLASS(DatabaseManager)
QT_FORWARD_DECLARE_CLASS(LogEntryProcNew)
QT_FORWARD_DECLARE_CLASS(LogEntryStatTraf)
QT_FORWARD_DECLARE_CLASS(TrafListModel)

class AppStatModel : public StringListModel
{
    Q_OBJECT

public:
    explicit AppStatModel(DatabaseManager *databaseManager,
                          QObject *parent = nullptr);

    void initialize();

    Q_INVOKABLE TrafListModel *trafListModel(int trafType, int row,
                                             const QString &appPath) const;

    void handleProcNew(const LogEntryProcNew &procNewEntry);
    void handleStatTraf(const LogEntryStatTraf &statTrafEntry);

signals:

public slots:
    void clear() override;

    void remove(int row = -1) override;

private slots:
    void handleCreatedApp(qint64 appId, const QString &appPath);

private:
    void updateList();

private:
    DatabaseManager *m_databaseManager;

    TrafListModel *m_trafListModel;

    QVector<qint64> m_appIds;
};

#endif // APPSTATMODEL_H
