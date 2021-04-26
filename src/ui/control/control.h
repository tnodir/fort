#ifndef CONTROL_H
#define CONTROL_H

#include <QObject>

namespace Control {

enum Command : qint8 {
    CommandNone = 0,
    CommandConf,
    CommandProg,
};

}

#endif // CONTROL_H
