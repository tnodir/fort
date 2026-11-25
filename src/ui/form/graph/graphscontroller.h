#ifndef GRAPHSCONTROLLER_H
#define GRAPHSCONTROLLER_H

#include <form/basecontroller.h>

class IniUser;

class GraphsController : public BaseController
{
    Q_OBJECT

public:
    explicit GraphsController(QObject *parent = nullptr);

public slots:
    void saveGraph(IniUser &ini, int graphId);
    void removeGraph(int graphId);
};

#endif // GRAPHSCONTROLLER_H
