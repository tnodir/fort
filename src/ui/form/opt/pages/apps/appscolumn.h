#ifndef APPSCOLUMN_H
#define APPSCOLUMN_H

#include <QObject>

class AppsColumn : public QObject
{
    Q_OBJECT

public:
    explicit AppsColumn(QObject *parent = nullptr);

};

#endif // APPSCOLUMN_H
