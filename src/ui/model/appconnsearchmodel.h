#ifndef APPCONNSEARCHMODEL_H
#define APPCONNSEARCHMODEL_H

#include "connsearchmodel.h"

class App;

class AppConnSearchModel : public ConnSearchModel
{
    Q_OBJECT

public:
    explicit AppConnSearchModel(QObject *parent = nullptr);

    qint64 confAppId() const { return m_confAppId; }
    const QString &appPath() const { return m_appPath; }

    void setApp(const App &app);

    /* The program has an id or a path */
    static bool canFilterApp(const App &app);

protected:
    /* The program's connections only: by its id or path */
    bool isSearching() const override { return true; }

    qint64 lastConnsIdMin(qint64 idMax) const override;

    QString sqlSearchWhere() const override;
    void fillSearchVars(QVariantList &vars) const override;

private:
    qint64 m_confAppId = 0;

    QString m_appPath;
};

#endif // APPCONNSEARCHMODEL_H
