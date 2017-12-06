#ifndef APPSTATMODEL_H
#define APPSTATMODEL_H

#include "stringlistmodel.h"
#include "traflistmodel.h"

QT_FORWARD_DECLARE_CLASS(DatabaseManager)

class AppStatModel : public StringListModel
{
    Q_OBJECT

public:
    explicit AppStatModel(DatabaseManager *databaseManager,
                          QObject *parent = nullptr);

    void initialize();

    Q_INVOKABLE TrafListModel *trafListModel(TrafListModel::TrafType type,
                                             const QString &appPath) const;

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
};

#endif // APPSTATMODEL_H
