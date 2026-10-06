#ifndef GRAPHANIMATION_H
#define GRAPHANIMATION_H

#include <QEasingCurve>
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

// Animates a value by the easing curve with less frames than QVariantAnimation's ~60 per second
class GraphAnimation : public QObject
{
    Q_OBJECT

public:
    explicit GraphAnimation(QObject *parent = nullptr);

    int duration() const { return m_duration; }
    void setDuration(int v) { m_duration = v; }

    const QEasingCurve &easingCurve() const { return m_easingCurve; }
    void setEasingCurve(const QEasingCurve &v) { m_easingCurve = v; }

    double currentValue() const { return m_currentValue; }

    void start(double startValue, double endValue);
    void stop();

signals:
    void valueChanged(double value);

private:
    void setCurrentValue(double v);
    void updateValue();

private:
    int m_duration = 250;

    double m_startValue = 0;
    double m_endValue = 0;
    double m_currentValue = 0;

    QEasingCurve m_easingCurve;
    QElapsedTimer m_elapsedTimer;
    QTimer m_frameTimer;
};

#endif // GRAPHANIMATION_H
