#ifndef GRAPHPOPUP_H
#define GRAPHPOPUP_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QGridLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)

class GraphBaseWindow;

struct ActivityAppSpeed;
struct TrafBytes;

// The traffic of today and of the month and the fastest Programs, shown on hover over the graph
class GraphPopup : public QWidget
{
    Q_OBJECT

public:
    explicit GraphPopup(GraphBaseWindow *graphWindow);

    void showPopup();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateTraffic();
    void updateTopApps();

    void updatePlacement();

private:
    struct Row
    {
        QLabel *iconLabel = nullptr;
        QLabel *nameLabel = nullptr;
        QLabel *inLabel = nullptr;
        QLabel *outLabel = nullptr;
    };

    void setupUi();
    void setupHeader(QGridLayout *layout);
    void setupRows(QGridLayout *layout);
    void setupRefresh();

    void fillAppRow(const Row &row, const ActivityAppSpeed &appSpeed);
    void setRowSpeeds(const Row &row, qint64 inBits, qint64 outBits);
    void updateOtherSpeed();

    void updateSize();

    static Row createRow(QGridLayout *layout, int rowIndex, bool withIcon);
    static void setRowBytes(const Row &row, const TrafBytes &bytes);
    static void setRowVisible(const Row &row, bool visible);

private:
    qint64 m_topInBits = 0; // the sum of the top Programs' speeds
    qint64 m_topOutBits = 0;

    GraphBaseWindow *m_graphWindow = nullptr;

    Row m_todayRow;
    Row m_monthRow;
    QList<Row> m_appRows;
    Row m_otherRow;
};

#endif // GRAPHPOPUP_H
