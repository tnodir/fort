#ifndef OPTIONSPAGE_H
#define OPTIONSPAGE_H

#include <QObject>

class OptionsPage : public QObject
{
    Q_OBJECT
public:
    explicit OptionsPage(QObject *parent = nullptr);

signals:

};

#endif // OPTIONSPAGE_H
