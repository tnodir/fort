#ifndef CONTROL_H
#define CONTROL_H

#include <QDebug>

#include "control_types.h"

namespace Control {

RpcManager managerByCommand(Command cmd);

bool commandRequiresValidation(Command cmd);

QDebug operator<<(QDebug debug, Command cmd);
QDebug operator<<(QDebug debug, RpcManager rpcManager);

}

#endif // CONTROL_H
