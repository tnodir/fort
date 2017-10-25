#ifndef TASKUZONLINE_H
#define TASKUZONLINE_H

#include "tasktasix.h"

class TaskUzonline : public TaskTasix
{
    Q_OBJECT

public:
    explicit TaskUzonline(QObject *parent = nullptr);

    static QString parseUzonlineBufer(const QByteArray &buffer);

protected:
    void startDownloader() const override;

    QString parseBufer(const QByteArray &buffer) const override {
        return parseUzonlineBufer(buffer);
    }
};

#endif // TASKUZONLINE_H
