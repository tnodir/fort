#ifndef INIGRAPH_H
#define INIGRAPH_H

#include <QColor>
#include <QRect>
#include <QVariant>

#include <form/graph/graph_types.h>

class MapSettings;

class IniGraph
{
public:
    static constexpr int maxCount = 8;

    explicit IniGraph(MapSettings &ini, int graphId = 0);

    int graphId() const { return m_graphId; }
    MapSettings &ini() const { return m_ini; }
    const QString &group() const { return m_group; }

    static QString groupName(int graphId);

    // Removes the graph's options
    void remove();

    QString name() const { return value("name").toString(); }
    void setName(const QString &v) { setValue("name", v); }

    static constexpr bool hideOnCloseDefault() { return false; }
    bool hideOnClose() const { return valueBool("hideOnClose", hideOnCloseDefault()); }
    void setHideOnClose(bool on) { setValue("hideOnClose", on); }

    bool visible() const { return valueBool("visible"); }
    void setVisible(bool on) { setValue("visible", on); }

    QRect geometry() const { return value("geometry").toRect(); }
    void setGeometry(const QRect &v) { setValue("geometry", v); }

    bool maximized() const { return valueBool("maximized"); }
    void setMaximized(bool on) { setValue("maximized", on); }

    static constexpr bool alwaysOnTopDefault() { return true; }
    bool alwaysOnTop() const { return valueBool("alwaysOnTop", alwaysOnTopDefault()); }
    void setAlwaysOnTop(bool on) { setValue("alwaysOnTop", on); }

    static constexpr bool framelessDefault() { return false; }
    bool frameless() const { return valueBool("frameless", framelessDefault()); }
    void setFrameless(bool on) { setValue("frameless", on); }

    static constexpr bool clickThroughDefault() { return false; }
    bool clickThrough() const { return valueBool("clickThrough", clickThroughDefault()); }
    void setClickThrough(bool on) { setValue("clickThrough", on); }

    static constexpr bool hideOnHoverDefault() { return false; }
    bool hideOnHover() const { return valueBool("hideOnHover", hideOnHoverDefault()); }
    void setHideOnHover(bool on) { setValue("hideOnHover", on); }

    static constexpr bool hideYAxisLabelsDefault() { return false; }
    bool hideYAxisLabels() const { return valueBool("hideYAxisLabels", hideYAxisLabelsDefault()); }
    void setHideYAxisLabels(bool on) { setValue("hideYAxisLabels", on); }

    static constexpr bool showSpeedDefault() { return false; }
    bool showSpeed() const { return valueBool("showSpeed", showSpeedDefault()); }
    void setShowSpeed(bool on) { setValue("showSpeed", on); }

    static constexpr bool animationDefault() { return false; }
    bool animation() const { return valueBool("animation", animationDefault()); }
    void setAnimation(bool on) { setValue("animation", on); }

    static constexpr bool axisTicksDefault() { return false; }
    bool axisTicks() const { return valueBool("axisTicks", axisTicksDefault()); }
    void setAxisTicks(bool on) { setValue("axisTicks", on); }

    static constexpr int opacityDefault() { return 90; }
    int opacity() const { return valueInt("opacity", opacityDefault()); }
    void setOpacity(int v) { setValue("opacity", v); }

    static constexpr int hoverOpacityDefault() { return 95; }
    int hoverOpacity() const { return valueInt("hoverOpacity", hoverOpacityDefault()); }
    void setHoverOpacity(int v) { setValue("hoverOpacity", v); }

    static constexpr bool showInTaskbarDefault() { return false; }
    bool showInTaskbar() const { return valueBool("showInTaskbar", showInTaskbarDefault()); }
    void setShowInTaskbar(bool on) { setValue("showInTaskbar", on); }

    // At 100% scale, px
    static constexpr int taskbarWidthDefault() { return 200; }
    int taskbarWidth() const { return valueInt("taskbarWidth", taskbarWidthDefault()); }
    void setTaskbarWidth(int v) { setValue("taskbarWidth", v); }

    static constexpr int maxSecondsDefault() { return 500; }
    int maxSeconds() const { return valueInt("maxSeconds", maxSecondsDefault()); }
    void setMaxSeconds(int v) { setValue("maxSeconds", v); }

    static constexpr int fixedSpeedDefault() { return 0; }
    int fixedSpeed() const { return valueInt("fixedSpeed", fixedSpeedDefault()); }
    void setFixedSpeed(int v) { setValue("fixedSpeed", v); }

    static constexpr int trafUnitDefault() { return 0; }
    int trafUnit() const { return valueInt("trafUnit", trafUnitDefault()); }
    void setTrafUnit(int v) { setValue("trafUnit", v); }

    static const char *colorKey(GraphColorType type, bool dark);

    static QColor colorDefault(GraphColorType type, bool dark);
    QColor color(GraphColorType type, bool dark) const;
    void setColor(GraphColorType type, bool dark, const QColor &v);

    static constexpr int tickLabelSizeDefault() { return 9; }
    int tickLabelSize() const { return valueInt("tickLabelSize", tickLabelSizeDefault()); }
    void setTickLabelSize(int v) { setValue("tickLabelSize", v); }

    static constexpr int speedTextSizeDefault() { return 10; }
    int speedTextSize() const { return valueInt("speedTextSize", speedTextSizeDefault()); }
    void setSpeedTextSize(int v) { setValue("speedTextSize", v); }

    static constexpr bool showGraphDefault() { return true; }
    bool showGraph() const { return valueBool("showGraph", showGraphDefault()); }
    void setShowGraph(bool on) { setValue("showGraph", on); }

    static constexpr int graphTypeDefault() { return 0; }
    int graphType() const { return valueInt("graphType", graphTypeDefault()); }
    void setGraphType(int v) { setValue("graphType", v); }

    static constexpr int barWidthDefault() { return 5; }
    int barWidth() const { return valueInt("barWidth", barWidthDefault()); }
    void setBarWidth(int v) { setValue("barWidth", v); }

    static constexpr bool showTotalDefault() { return false; }
    bool showTotal() const { return valueBool("showTotal", showTotalDefault()); }
    void setShowTotal(bool on) { setValue("showTotal", on); }

    static constexpr bool showInDefault() { return true; }
    bool showIn() const { return valueBool("showIn", showInDefault()); }
    void setShowIn(bool on) { setValue("showIn", on); }

    static constexpr bool showOutDefault() { return true; }
    bool showOut() const { return valueBool("showOut", showOutDefault()); }
    void setShowOut(bool on) { setValue("showOut", on); }

private:
    QString key(const char *name) const;

    QVariant value(const char *name) const;
    void setValue(const char *name, const QVariant &v);

    int valueInt(const char *name, int defaultValue) const;
    bool valueBool(const char *name, bool defaultValue = false) const;

    QColor valueColor(const char *name, const QColor &defaultValue) const;
    void setColorValue(const char *name, const QColor &v);

private:
    int m_graphId = 0;

    MapSettings &m_ini;

    QString m_group;
};

#endif // INIGRAPH_H
