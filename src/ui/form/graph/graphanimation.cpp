#include "graphanimation.h"

namespace {

inline constexpr int frameMsecs = 33; // ~30 frames per second

}

GraphAnimation::GraphAnimation(QObject *parent) : QObject(parent)
{
    m_frameTimer.setInterval(frameMsecs);

    connect(&m_frameTimer, &QTimer::timeout, this, &GraphAnimation::updateValue);
}

void GraphAnimation::start(double startValue, double endValue)
{
    m_startValue = startValue;
    m_endValue = endValue;

    m_elapsedTimer.start();
    m_frameTimer.start();

    setCurrentValue(startValue);
}

void GraphAnimation::stop()
{
    m_frameTimer.stop();
}

void GraphAnimation::setCurrentValue(double v)
{
    if (m_currentValue == v)
        return;

    m_currentValue = v;

    emit valueChanged(v);
}

void GraphAnimation::updateValue()
{
    const double progress = qMin(double(m_elapsedTimer.elapsed()) / qMax(m_duration, 1), 1.0);

    if (progress >= 1.0) {
        m_frameTimer.stop();
    }

    const double ratio = m_easingCurve.valueForProgress(progress);

    setCurrentValue(m_startValue + (m_endValue - m_startValue) * ratio);
}
