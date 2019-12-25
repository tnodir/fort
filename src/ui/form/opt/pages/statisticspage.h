#ifndef STATISTICSPAGE_H
#define STATISTICSPAGE_H

#include "basepage.h"

QT_FORWARD_DECLARE_CLASS(AppStatModel)
QT_FORWARD_DECLARE_CLASS(CheckTimePeriod)
QT_FORWARD_DECLARE_CLASS(LabelSpinCombo)
QT_FORWARD_DECLARE_CLASS(TrafListModel)

class StatisticsPage : public BasePage
{
    Q_OBJECT

public:
    explicit StatisticsPage(OptionsController *ctrl = nullptr,
                            QWidget *parent = nullptr);

    AppStatModel *appStatModel() const;
    TrafListModel *trafListModel() const { return m_trafListModel; }

protected slots:
    void onEditResetted() override;
    void onSaved() override;

    void onRetranslateUi() override;

private:
    bool graphEdited() const { return m_graphEdited; }
    void setGraphEdited(bool v);

    void retranslateTrafKeepDayNames() const;
    void retranslateTrafKeepMonthNames() const;
    void retranslateQuotaNames() const;
    void retranslateTrafUnitNames() const;

    void setupUi();
    QLayout *setupHeader();
    void setupClearMenu();
    void setupTrafOptionsMenu();
    void setupActivePeriod();
    void setupMonthStart();
    void setupTrafHourKeepDays();
    void setupTrafDayKeepDays();
    void setupTrafMonthKeepMonths();
    void setupQuotaDayMb();
    void setupQuotaMonthMb();
    void refreshPage();
    void setupModels();

    static LabelSpinCombo *createSpinCombo(int min, int max,
                                           const QString &suffix = QString());

    static QString formatQuota(int mbytes);

private:
    bool m_graphEdited = false;

    TrafListModel *m_trafListModel = nullptr;

    QPushButton *m_btRefresh = nullptr;
    QPushButton *m_btClear = nullptr;
    QAction *m_actRemoveApp = nullptr;
    QAction *m_actResetTotal = nullptr;
    QAction *m_actClearAll = nullptr;
    QPushButton *m_btTrafOptions = nullptr;
    CheckTimePeriod *m_ctpActivePeriod = nullptr;
    LabelSpinCombo *m_lscMonthStart = nullptr;
    LabelSpinCombo *m_lscTrafHourKeepDays = nullptr;
    LabelSpinCombo *m_lscTrafDayKeepDays = nullptr;
    LabelSpinCombo *m_lscTrafMonthKeepMonths = nullptr;
    LabelSpinCombo *m_lscQuotaDayMb = nullptr;
    LabelSpinCombo *m_lscQuotaMonthMb = nullptr;
};

#endif // STATISTICSPAGE_H
