#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QObject>

using namespace testing;

QT_BEGIN_NAMESPACE
inline void PrintTo(const QString &qString, ::std::ostream *os)
{
    *os << qPrintable(qString);
}
QT_END_NAMESPACE
