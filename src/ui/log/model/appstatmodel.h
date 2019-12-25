#ifndef APPSTATMODEL_H
#define APPSTATMODEL_H

#include "stringlistmodel.h"

QT_FORWARD_DECLARE_CLASS(LogEntryProcNew)
QT_FORWARD_DECLARE_CLASS(LogEntryStatTraf)
QT_FORWARD_DECLARE_CLASS(StatManager)
QT_FORWARD_DECLARE_CLASS(TrafListModel)

class AppStatModel : public StringListModel
{
    Q_OBJECT

public:
    explicit AppStatModel(StatManager *statManager,
                          QObject *parent = nullptr);

    void initialize();

    TrafListModel *trafListModel() const { return m_trafListModel; }

    void handleProcNew(const LogEntryProcNew &procNewEntry);
    void handleStatTraf(const LogEntryStatTraf &statTrafEntry);

    qint64 appIdByRow(int row) const;

public slots:
    void clear() override;

    void remove(int row = -1) override;

private slots:
    void handleCreatedApp(qint64 appId, const QString &appPath);

private:
    void updateList();

private:
    StatManager *m_statManager;
    TrafListModel *m_trafListModel;

    QVector<qint64> m_appIds;
};

#endif // APPSTATMODEL_H
