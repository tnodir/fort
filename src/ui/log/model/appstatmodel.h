#ifndef APPSTATMODEL_H
#define APPSTATMODEL_H

#include "stringlistmodel.h"

QT_FORWARD_DECLARE_CLASS(DatabaseManager)
QT_FORWARD_DECLARE_CLASS(TrafListModel)

class AppStatModel : public StringListModel
{
    Q_OBJECT

public:
    explicit AppStatModel(DatabaseManager *databaseManager,
                          QObject *parent = nullptr);

    void initialize();

    Q_INVOKABLE TrafListModel *trafListModel(int trafType, int row) const;

    void handleProcNew(const QString &appPath);
    void handleStatTraf(quint16 procCount, const quint8 *procBits,
                        const quint32 *trafBytes);

signals:

public slots:
    void clear() override;

private:
    void updateList();

private:
    DatabaseManager *m_databaseManager;

    TrafListModel *m_trafListModel;

    QVector<qint64> m_appIds;
};

#endif // APPSTATMODEL_H
