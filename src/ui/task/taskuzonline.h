#ifndef TASKUZONLINE_H
#define TASKUZONLINE_H

#include "tasktasix.h"

class TaskUzonline : public TaskTasix
{
    Q_OBJECT

public:
    explicit TaskUzonline(QObject *parent = nullptr);

    static QStringList parseUzonlineBuffer(const QByteArray &buffer);

protected:
    void setupDownloader() override;

    virtual QStringList parseCustomBuffer(const QByteArray &buffer) const override {
        return parseUzonlineBuffer(buffer);
    }

    QString successMessage() const override {
        return tr("UzOnline addresses updated!");
    }
};

#endif // TASKUZONLINE_H
