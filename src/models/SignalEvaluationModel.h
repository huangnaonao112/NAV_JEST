#pragma once

#include <QObject>
#include <QtQml/qqml.h>

/**
 * @brief 信号评估桥接模型：把 navcore 算法内核（GnssSignalSimulation +
 * VirtualReceiver）暴露给 QML。
 *
 * 干扰/欺骗配置通过 Q_PROPERTY 由评估页快调面板写入；runEvaluation()
 * 一次性完成“信号场景评估 → 星座伪距生成 → 最小二乘定位解算”的闭环，
 * 结果（C/N0、J/S、定位误差、残差 RMS 等）以只读属性通知 QML 刷新。
 *
 * 本模型不含算法逻辑：只做参数搬运、单位换算（km↔m）与结果缓存。
 */
class SignalEvaluationModel : public QObject {
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

    // ---- 场景配置（可由 QML 直接写） ----
    Q_PROPERTY(QString profileId READ profileId WRITE setProfileId NOTIFY configChanged)
    Q_PROPERTY(bool interferenceEnabled READ interferenceEnabled WRITE setInterferenceEnabled NOTIFY configChanged)
    Q_PROPERTY(QString interferenceTypeId READ interferenceTypeId WRITE setInterferenceTypeId NOTIFY configChanged)
    Q_PROPERTY(double interferenceCenterOffsetMHz READ interferenceCenterOffsetMHz WRITE setInterferenceCenterOffsetMHz NOTIFY configChanged)
    Q_PROPERTY(double interferenceBandwidthMHz READ interferenceBandwidthMHz WRITE setInterferenceBandwidthMHz NOTIFY configChanged)
    Q_PROPERTY(double interferencePowerDbm READ interferencePowerDbm WRITE setInterferencePowerDbm NOTIFY configChanged)
    Q_PROPERTY(double interferenceDutyCyclePercent READ interferenceDutyCyclePercent WRITE setInterferenceDutyCyclePercent NOTIFY configChanged)
    Q_PROPERTY(bool spoofingEnabled READ spoofingEnabled WRITE setSpoofingEnabled NOTIFY configChanged)
    Q_PROPERTY(int spoofingPrn READ spoofingPrn WRITE setSpoofingPrn NOTIFY configChanged)
    Q_PROPERTY(double spoofingPowerDbm READ spoofingPowerDbm WRITE setSpoofingPowerDbm NOTIFY configChanged)
    Q_PROPERTY(double spoofingRangeBiasMeters READ spoofingRangeBiasMeters WRITE setSpoofingRangeBiasMeters NOTIFY configChanged)
    Q_PROPERTY(bool measurementNoiseEnabled READ measurementNoiseEnabled WRITE setMeasurementNoiseEnabled NOTIFY configChanged)
    Q_PROPERTY(double truthLatitudeDeg READ truthLatitudeDeg WRITE setTruthLatitudeDeg NOTIFY configChanged)
    Q_PROPERTY(double truthLongitudeDeg READ truthLongitudeDeg WRITE setTruthLongitudeDeg NOTIFY configChanged)
    Q_PROPERTY(double truthAltitudeMeters READ truthAltitudeMeters WRITE setTruthAltitudeMeters NOTIFY configChanged)

    // ---- 评估结果（只读，runEvaluation 后通知） ----
    Q_PROPERTY(QString evaluationStatus READ evaluationStatus NOTIFY evaluationChanged)
    Q_PROPERTY(int enabledChannelCount READ enabledChannelCount NOTIFY evaluationChanged)
    Q_PROPERTY(double nominalCn0DbHz READ nominalCn0DbHz NOTIFY evaluationChanged)
    Q_PROPERTY(double effectiveCn0DbHz READ effectiveCn0DbHz NOTIFY evaluationChanged)
    Q_PROPERTY(double jammingToSignalDb READ jammingToSignalDb NOTIFY evaluationChanged)
    Q_PROPERTY(double spoofingToAuthenticDb READ spoofingToAuthenticDb NOTIFY evaluationChanged)
    Q_PROPERTY(bool interferenceInBand READ interferenceInBand NOTIFY evaluationChanged)
    Q_PROPERTY(bool receiverValid READ receiverValid NOTIFY evaluationChanged)
    Q_PROPERTY(double positionErrorMeters READ positionErrorMeters NOTIFY evaluationChanged)
    Q_PROPERTY(double clockBiasMeters READ clockBiasMeters NOTIFY evaluationChanged)
    Q_PROPERTY(double residualRmsMeters READ residualRmsMeters NOTIFY evaluationChanged)
    Q_PROPERTY(int usedSatellites READ usedSatellites NOTIFY evaluationChanged)
    Q_PROPERTY(int solutionIterations READ solutionIterations NOTIFY evaluationChanged)
    Q_PROPERTY(QString solutionMessage READ solutionMessage NOTIFY evaluationChanged)

public:
    explicit SignalEvaluationModel(QObject *parent = nullptr);

    Q_INVOKABLE void runEvaluation();

    QString profileId() const;
    void setProfileId(const QString &profileId);
    bool interferenceEnabled() const;
    void setInterferenceEnabled(bool enabled);
    QString interferenceTypeId() const;
    void setInterferenceTypeId(const QString &typeId);
    double interferenceCenterOffsetMHz() const;
    void setInterferenceCenterOffsetMHz(double value);
    double interferenceBandwidthMHz() const;
    void setInterferenceBandwidthMHz(double value);
    double interferencePowerDbm() const;
    void setInterferencePowerDbm(double value);
    double interferenceDutyCyclePercent() const;
    void setInterferenceDutyCyclePercent(double value);
    bool spoofingEnabled() const;
    void setSpoofingEnabled(bool enabled);
    int spoofingPrn() const;
    void setSpoofingPrn(int prn);
    double spoofingPowerDbm() const;
    void setSpoofingPowerDbm(double value);
    double spoofingRangeBiasMeters() const;
    void setSpoofingRangeBiasMeters(double value);
    bool measurementNoiseEnabled() const;
    void setMeasurementNoiseEnabled(bool enabled);
    double truthLatitudeDeg() const;
    void setTruthLatitudeDeg(double value);
    double truthLongitudeDeg() const;
    void setTruthLongitudeDeg(double value);
    double truthAltitudeMeters() const;
    void setTruthAltitudeMeters(double value);

    QString evaluationStatus() const;
    int enabledChannelCount() const;
    double nominalCn0DbHz() const;
    double effectiveCn0DbHz() const;
    double jammingToSignalDb() const;
    double spoofingToAuthenticDb() const;
    bool interferenceInBand() const;
    bool receiverValid() const;
    double positionErrorMeters() const;
    double clockBiasMeters() const;
    double residualRmsMeters() const;
    int usedSatellites() const;
    int solutionIterations() const;
    QString solutionMessage() const;

signals:
    void configChanged();
    void evaluationChanged();

private:
    // 场景配置
    QString m_profileId = QStringLiteral("BDS_B1I");
    bool m_interferenceEnabled = false;
    QString m_interferenceTypeId = QStringLiteral("BROADBAND");
    double m_interferenceCenterOffsetMHz = 0.0;
    double m_interferenceBandwidthMHz = 2.0;
    double m_interferencePowerDbm = -110.0;
    double m_interferenceDutyCyclePercent = 100.0;
    bool m_spoofingEnabled = false;
    int m_spoofingPrn = 1;
    double m_spoofingPowerDbm = -125.0;
    double m_spoofingRangeBiasMeters = 50.0;
    // 测量热噪声开关：按有效 C/N0 耦合伪距噪声（确定性固定种子，可回归）
    bool m_measurementNoiseEnabled = true;
    double m_truthLatitudeDeg = 39.9042;
    double m_truthLongitudeDeg = 116.4074;
    double m_truthAltitudeMeters = 50.0;

    // 最近一次评估结果缓存
    QString m_evaluationStatus = QStringLiteral("尚未运行，点击“运行评估”开始");
    int m_enabledChannelCount = 0;
    double m_nominalCn0DbHz = 0.0;
    double m_effectiveCn0DbHz = 0.0;
    double m_jammingToSignalDb = 0.0;
    double m_spoofingToAuthenticDb = 0.0;
    bool m_interferenceInBand = false;
    bool m_receiverValid = false;
    double m_positionErrorMeters = 0.0;
    double m_clockBiasMeters = 0.0;
    double m_residualRmsMeters = 0.0;
    int m_usedSatellites = 0;
    int m_solutionIterations = 0;
    QString m_solutionMessage;
};
