#ifndef INIGRAPH_H
#define INIGRAPH_H

#include <QColor>
#include <QRect>
#include <QVariant>

class MapSettings;

class IniGraph
{
public:
    explicit IniGraph(MapSettings &ini, int graphId = 0);

    int graphId() const { return m_graphId; }
    MapSettings &ini() const { return m_ini; }
    const QString &group() const { return m_group; }

    static QString groupName(int graphId);

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

    static constexpr int maxSecondsDefault() { return 500; }
    int maxSeconds() const { return valueInt("maxSeconds", maxSecondsDefault()); }
    void setMaxSeconds(int v) { setValue("maxSeconds", v); }

    static constexpr int fixedSpeedDefault() { return 0; }
    int fixedSpeed() const { return valueInt("fixedSpeed", fixedSpeedDefault()); }
    void setFixedSpeed(int v) { setValue("fixedSpeed", v); }

    static constexpr int trafUnitDefault() { return 0; }
    int trafUnit() const { return valueInt("trafUnit", trafUnitDefault()); }
    void setTrafUnit(int v) { setValue("trafUnit", v); }

    static constexpr QColor colorDefault() { return QColor(255, 255, 255); }
    QColor color() const { return valueColor("color", colorDefault()); }
    void setColor(const QColor &v) { setColorValue("color", v); }

    static constexpr QColor borderColorDefault() { return QColor(108, 108, 108); }
    QColor borderColor() const { return valueColor("borderColor", borderColorDefault()); }
    void setBorderColor(const QColor &v) { setColorValue("borderColor", v); }

    static constexpr QColor colorInDefault() { return QColor(52, 196, 84); }
    QColor colorIn() const { return valueColor("colorIn", colorInDefault()); }
    void setColorIn(const QColor &v) { setColorValue("colorIn", v); }

    static constexpr QColor colorOutDefault() { return QColor(235, 71, 63); }
    QColor colorOut() const { return valueColor("colorOut", colorOutDefault()); }
    void setColorOut(const QColor &v) { setColorValue("colorOut", v); }

    static constexpr QColor totalColorDefault() { return QColor(245, 184, 0); }
    QColor totalColor() const { return valueColor("totalColor", totalColorDefault()); }
    void setTotalColor(const QColor &v) { setColorValue("totalColor", v); }

    static constexpr QColor axisColorDefault() { return QColor(108, 108, 108); }
    QColor axisColor() const { return valueColor("axisColor", axisColorDefault()); }
    void setAxisColor(const QColor &v) { setColorValue("axisColor", v); }

    static constexpr QColor tickLabelColorDefault() { return QColor(0, 0, 0); }
    QColor tickLabelColor() const { return valueColor("tickLabelColor", tickLabelColorDefault()); }
    void setTickLabelColor(const QColor &v) { setColorValue("tickLabelColor", v); }

    static constexpr QColor labelColorDefault() { return QColor(33, 33, 33); }
    QColor labelColor() const { return valueColor("labelColor", labelColorDefault()); }
    void setLabelColor(const QColor &v) { setColorValue("labelColor", v); }

    static constexpr QColor gridColorDefault() { return QColor(141, 141, 141); }
    QColor gridColor() const { return valueColor("gridColor", gridColorDefault()); }
    void setGridColor(const QColor &v) { setColorValue("gridColor", v); }

    static constexpr QColor darkColorDefault() { return QColor(56, 56, 56); }
    QColor darkColor() const { return valueColor("darkColor", darkColorDefault()); }
    void setDarkColor(const QColor &v) { setColorValue("darkColor", v); }

    static constexpr QColor darkBorderColorDefault() { return QColor(141, 141, 141); }
    QColor darkBorderColor() const
    {
        return valueColor("darkBorderColor", darkBorderColorDefault());
    }
    void setDarkBorderColor(const QColor &v) { setColorValue("darkBorderColor", v); }

    static constexpr QColor darkColorInDefault() { return QColor(52, 196, 84); }
    QColor darkColorIn() const { return valueColor("darkColorIn", darkColorInDefault()); }
    void setDarkColorIn(const QColor &v) { setColorValue("darkColorIn", v); }

    static constexpr QColor darkColorOutDefault() { return QColor(235, 71, 63); }
    QColor darkColorOut() const { return valueColor("darkColorOut", darkColorOutDefault()); }
    void setDarkColorOut(const QColor &v) { setColorValue("darkColorOut", v); }

    static constexpr QColor darkTotalColorDefault() { return QColor(245, 184, 0); }
    QColor darkTotalColor() const { return valueColor("darkTotalColor", darkTotalColorDefault()); }
    void setDarkTotalColor(const QColor &v) { setColorValue("darkTotalColor", v); }

    static constexpr QColor darkAxisColorDefault() { return QColor(141, 141, 141); }
    QColor darkAxisColor() const { return valueColor("darkAxisColor", darkAxisColorDefault()); }
    void setDarkAxisColor(const QColor &v) { setColorValue("darkAxisColor", v); }

    static constexpr QColor darkTickLabelColorDefault() { return QColor(248, 248, 248); }
    QColor darkTickLabelColor() const
    {
        return valueColor("darkTickLabelColor", darkTickLabelColorDefault());
    }
    void setDarkTickLabelColor(const QColor &v) { setColorValue("darkTickLabelColor", v); }

    static constexpr QColor darkLabelColorDefault() { return QColor(248, 248, 248); }
    QColor darkLabelColor() const { return valueColor("darkLabelColor", darkLabelColorDefault()); }
    void setDarkLabelColor(const QColor &v) { setColorValue("darkLabelColor", v); }

    static constexpr QColor darkGridColorDefault() { return QColor(108, 108, 108); }
    QColor darkGridColor() const { return valueColor("darkGridColor", darkGridColorDefault()); }
    void setDarkGridColor(const QColor &v) { setColorValue("darkGridColor", v); }

    static constexpr int tickLabelSizeDefault() { return 9; }
    int tickLabelSize() const { return valueInt("tickLabelSize", tickLabelSizeDefault()); }
    void setTickLabelSize(int v) { setValue("tickLabelSize", v); }

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
