#ifndef TASKUZONLINE_H
#define TASKUZONLINE_H

#include "tasktasix.h"

class TaskUzonline : public TaskTasix
{
    Q_OBJECT

public:
    explicit TaskUzonline(QObject *parent = nullptr);

    static QString parseUzonlineBuffer(const QByteArray &buffer);

protected:
    void startDownloader() const override;

    QString parseBuffer(const QByteArray &buffer) const override {
        return parseUzonlineBuffer(buffer);
    }
};

#endif // TASKUZONLINE_H
