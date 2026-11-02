// SignalEvaluationModelTests - 桥接层回归测试（无 UI、毫秒级）。
// 断言基准与 2026-11-02 界面实测三场景一致（基线/干扰/欺骗），
// 数值为确定性计算（无随机噪声），可作精确回归锚点。
#include "SignalEvaluationModel.h"

#include <QtTest/QtTest>

class SignalEvaluationModelTests : public QObject {
    Q_OBJECT

private slots:
    void baselineScenario();
    void jammingScenario();
    void spoofingScenario();
    void spoofingPrnOutOfRange();
    void invalidProfileFallsBack();
    void noiseOnBaselineScenario();
    void jammingNoiseCoupling();
};

namespace
{
// 复位到基线场景（与评估页默认值一致）。
// 锚点用例显式关闭测量噪声：界面实测的 0.000 m / 11.135 m 等数值
// 均为无噪声确定性解，保持可精确回归。
void resetBaseline(SignalEvaluationModel &model)
{
    model.setProfileId(QStringLiteral("BDS_B1I"));
    model.setInterferenceEnabled(false);
    model.setSpoofingEnabled(false);
    model.setSpoofingPrn(1);
    model.setSpoofingRangeBiasMeters(50.0);
    model.setMeasurementNoiseEnabled(false);
    model.setTruthLatitudeDeg(39.9042);
    model.setTruthLongitudeDeg(116.4074);
    model.setTruthAltitudeMeters(50.0);
}
} // namespace

void SignalEvaluationModelTests::baselineScenario()
{
    SignalEvaluationModel model;
    resetBaseline(model);
    model.runEvaluation();

    QVERIFY(model.receiverValid());
    QVERIFY(model.evaluationStatus().contains(QStringLiteral("评估完成")));
    QCOMPARE(model.enabledChannelCount(), 24);
    QVERIFY2(qAbs(model.nominalCn0DbHz() - 42.0) < 0.05,
             qPrintable(QStringLiteral("名义C/N0=%1").arg(model.nominalCn0DbHz())));
    QVERIFY2(qAbs(model.effectiveCn0DbHz() - 42.0) < 0.05,
             qPrintable(QStringLiteral("有效C/N0=%1").arg(model.effectiveCn0DbHz())));
    QVERIFY(model.jammingToSignalDb() < -100.0); // 未开干扰≈-180
    QVERIFY(model.spoofingToAuthenticDb() < -100.0);
    QVERIFY(!model.interferenceInBand());
    QCOMPARE(model.usedSatellites(), 8);
    // 无噪声无偏差：解算应收敛到真值（界面实测 0.000 m）
    QVERIFY2(model.positionErrorMeters() < 1e-3,
             qPrintable(QStringLiteral("定位误差=%1").arg(model.positionErrorMeters())));
    // 场景注入的 65 m 钟差应被精确还原（界面实测 65.0 m）
    QVERIFY2(qAbs(model.clockBiasMeters() - 65.0) < 0.05,
             qPrintable(QStringLiteral("钟差=%1").arg(model.clockBiasMeters())));
    QVERIFY2(model.residualRmsMeters() < 1e-3,
             qPrintable(QStringLiteral("残差RMS=%1").arg(model.residualRmsMeters())));
}

void SignalEvaluationModelTests::jammingScenario()
{
    SignalEvaluationModel model;
    resetBaseline(model);
    model.setInterferenceEnabled(true);
    model.setInterferenceTypeId(QStringLiteral("BROADBAND"));
    model.setInterferencePowerDbm(-110.0);
    model.setInterferenceBandwidthMHz(2.0);
    model.runEvaluation();

    // 界面实测：有效 C/N0 42→39.5，J/S=20.0 dB，带内=是
    QVERIFY2(qAbs(model.effectiveCn0DbHz() - 39.5) < 0.1,
             qPrintable(QStringLiteral("有效C/N0=%1").arg(model.effectiveCn0DbHz())));
    QVERIFY2(qAbs(model.jammingToSignalDb() - 20.0) < 0.1,
             qPrintable(QStringLiteral("J/S=%1").arg(model.jammingToSignalDb())));
    QVERIFY(model.interferenceInBand());
    QVERIFY(model.receiverValid());
}

void SignalEvaluationModelTests::spoofingScenario()
{
    SignalEvaluationModel model;
    resetBaseline(model);
    model.setSpoofingEnabled(true);
    model.setSpoofingPrn(1);
    model.setSpoofingRangeBiasMeters(50.0);
    model.runEvaluation();

    // 界面实测：欺骗比 5.0 dB，误差 11.135 m，残差 15.183 m，钟差 68.3 m
    QVERIFY2(qAbs(model.spoofingToAuthenticDb() - 5.0) < 0.05,
             qPrintable(QStringLiteral("欺骗比=%1").arg(model.spoofingToAuthenticDb())));
    QVERIFY2(qAbs(model.positionErrorMeters() - 11.135) < 0.05,
             qPrintable(QStringLiteral("定位误差=%1").arg(model.positionErrorMeters())));
    QVERIFY2(qAbs(model.residualRmsMeters() - 15.183) < 0.05,
             qPrintable(QStringLiteral("残差RMS=%1").arg(model.residualRmsMeters())));
    QVERIFY2(qAbs(model.clockBiasMeters() - 68.3) < 0.1,
             qPrintable(QStringLiteral("钟差=%1").arg(model.clockBiasMeters())));
    QVERIFY(model.nominalCn0DbHz() > 41.0); // 无干扰，C/N0 不受影响
    QVERIFY(!model.interferenceInBand());
}

void SignalEvaluationModelTests::spoofingPrnOutOfRange()
{
    SignalEvaluationModel model;
    resetBaseline(model);
    model.setSpoofingEnabled(true);
    model.setSpoofingPrn(99); // 超出引擎合法范围 [1,63]，会被算法层拒绝
    model.runEvaluation();

    // 引擎拒绝非法 PRN → 欺骗未生效（比值保持未激活的 -180），
    // 但评估本身必须安全完成，定位不受影响，状态里给出拒绝提示
    QVERIFY2(model.spoofingToAuthenticDb() < -100.0,
             qPrintable(QStringLiteral("欺骗比=%1").arg(model.spoofingToAuthenticDb())));
    QVERIFY(model.evaluationStatus().contains(QStringLiteral("拒绝")));
    QVERIFY2(model.positionErrorMeters() < 1e-3,
             qPrintable(QStringLiteral("定位误差=%1").arg(model.positionErrorMeters())));
    QVERIFY(model.receiverValid());
    QCOMPARE(model.usedSatellites(), 8);
}

void SignalEvaluationModelTests::invalidProfileFallsBack()
{
    SignalEvaluationModel model;
    resetBaseline(model);
    model.setProfileId(QStringLiteral("NOT_A_PROFILE"));
    model.runEvaluation();

    QVERIFY(model.evaluationStatus().contains(QStringLiteral("回退")));
    QVERIFY(model.receiverValid()); // 回退 BDS_B1I 后仍应正常评估
    QVERIFY(model.enabledChannelCount() == 24);
}

void SignalEvaluationModelTests::noiseOnBaselineScenario()
{
    SignalEvaluationModel model;
    resetBaseline(model);
    model.setMeasurementNoiseEnabled(true); // 参考点 42 dB-Hz，σ=1.5 m
    model.runEvaluation();

    // 开噪声后解算应偏离真值（误差>0）但保持收敛，残差反映噪声水平
    QVERIFY(model.receiverValid());
    QVERIFY2(model.positionErrorMeters() > 1e-3,
             qPrintable(QStringLiteral("定位误差=%1").arg(model.positionErrorMeters())));
    QVERIFY2(model.positionErrorMeters() < 15.0,
             qPrintable(QStringLiteral("定位误差=%1").arg(model.positionErrorMeters())));
    QVERIFY2(model.residualRmsMeters() > 1e-3,
             qPrintable(QStringLiteral("残差RMS=%1").arg(model.residualRmsMeters())));
    // 确定性：两次运行结果必须逐位一致
    const double firstError = model.positionErrorMeters();
    model.runEvaluation();
    QVERIFY2(qAbs(model.positionErrorMeters() - firstError) < 1e-12,
             qPrintable(QStringLiteral("两次评估误差不一致: %1 vs %2")
                            .arg(firstError)
                            .arg(model.positionErrorMeters())));
}

void SignalEvaluationModelTests::jammingNoiseCoupling()
{
    SignalEvaluationModel model;
    resetBaseline(model);
    model.setMeasurementNoiseEnabled(true);
    model.runEvaluation();
    const double cleanError = model.positionErrorMeters();

    // 强干扰：宽带噪声 -90 dBm → 有效 C/N0 大幅下降 → 伪距噪声放大 → 误差显著增大
    model.setInterferenceEnabled(true);
    model.setInterferenceTypeId(QStringLiteral("BROADBAND"));
    model.setInterferencePowerDbm(-90.0);
    model.setInterferenceBandwidthMHz(2.0);
    model.runEvaluation();

    QVERIFY2(model.effectiveCn0DbHz() < 35.0,
             qPrintable(QStringLiteral("强干扰有效C/N0=%1").arg(model.effectiveCn0DbHz())));
    QVERIFY2(model.positionErrorMeters() > 2.0 * cleanError,
             qPrintable(QStringLiteral("干扰误差=%1 未超过基线%2的2倍")
                            .arg(model.positionErrorMeters())
                            .arg(cleanError)));
    QVERIFY(model.interferenceInBand());
    QVERIFY(model.receiverValid());
}

QTEST_GUILESS_MAIN(SignalEvaluationModelTests)
#include "SignalEvaluationModelTests.moc"
