// SceneDailMapper 的数据库步骤映射（旧版兼容接口）：mapToDbSteps / dbStepsToMap
#include "SceneDetailMapper.h"
#include "utils/SceneLogger.h"

#include <QVariantList>
#include <QDebug>


// ========== 旧版函数（保留向后兼容） ==========

QMap<int, QJsonObject> SceneDetailMapper::mapToDbSteps(const QVariantMap &detailData)
{
    QMap<int, QJsonObject> result;

    QString testType = detailData.value("testType").toString();
    QString sceneType = detailData.value("sceneType").toString();

    // 场景类型判断
    bool isMediumOrbit = (sceneType == "中高轨导航终端性能测试");
    bool isAntiJamming = (sceneType == "导航用户装备抗干扰性能试验");
    bool isSecurityAuth = (sceneType == "安全认证测试终端试验");

    // ★ 计算默认场景存储路径
    QString defaultScenePath;
    {
        QString appDir = QCoreApplication::applicationDirPath();
        QString configDir = appDir + "/config";
        QDir dir(configDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        defaultScenePath = QDir::cleanPath(configDir);
    }

    // ===== OPT_INDEX=0: 场景配置 (原仿真开始) =====
    QJsonObject simStart;
    simStart["仿真场景文件名"] = detailData.value("filePath", "").toString();
    simStart["等待模拟器稳定时间"] = 10;
    simStart["通道总数"] = isMediumOrbit ? 36 : 24;
    simStart["场景类型"] = sceneType;
    simStart["开始时间"] = detailData.value("startTime", "2025/12/16 12:00:00").toString();
    simStart["结束时间"] = detailData.value("endTime", "2025/12/16 14:00:00").toString();

    // ★ 如果scenePath为空，使用默认路径
    QString scenePath = detailData.value("scenePath", "").toString();
    if (scenePath.trimmed().isEmpty()) {
        scenePath = defaultScenePath;
    }
    simStart["场景存储路径"] = scenePath;

    result[0] = simStart;

    // ===== OPT_INDEX=1: 单频点信号设置 =====
    // 对应测试软件的"单频点信号设置"步骤
    if (!isMediumOrbit) {
        QJsonObject signalSet;

        // 根据场景类型从独立的数据键获取频点，确保场景间数据隔离
        QString freq;
        if (isAntiJamming) {
            freq = detailData.value("antiJammingFreq", detailData.value("globalFreq", "GPS_L1")).toString();
        } else if (isSecurityAuth) {
            freq = detailData.value("securityAuthFreq", detailData.value("globalFreq", "GPS_L1")).toString();
        } else if (sceneType == "低轨导航终端性能测试") {
            freq = detailData.value("lowOrbitFreq", detailData.value("globalFreq", "GPS_L1")).toString();
        } else if (sceneType == "地月空间终端设备性能测试") {
            freq = detailData.value("lunarSpaceFreq", detailData.value("globalFreq", "GPS_L1")).toString();
        } else {
            freq = detailData.value("globalFreq", "GPS_L1").toString();
        }
        signalSet["频点选择"] = freq;

        // 根据场景类型从独立的数据键获取通道配置，确保场景间数据隔离
        QVariantList channels;
        if (isAntiJamming) {
            channels = detailData.value("antiJammingChannels", detailData.value("channels")).toList();
        } else if (isSecurityAuth) {
            channels = detailData.value("securityAuthChannels", detailData.value("channels")).toList();
        } else if (sceneType == "低轨导航终端性能测试") {
            channels = detailData.value("lowOrbitChannels", detailData.value("channels")).toList();
        } else if (sceneType == "地月空间终端设备性能测试") {
            channels = detailData.value("lunarSpaceChannels", detailData.value("channels")).toList();
        } else {
            channels = detailData.value("channels").toList();
        }

        // 24通道对象数组格式（参考满天星信号设置）
        QJsonArray channelConfig;
        for (int i = 0; i < 24; ++i) {
            QJsonObject chObj;
            if (i < channels.size()) {
                QVariantMap ch = channels[i].toMap();
                QString sw = ch.value("switchType", "关闭").toString();
                chObj["通道开关"] = (sw == "开启" || sw == "打开" || sw == "true") ? "1-打开" : "0-关闭";
                chObj["通道功率"] = ch.value("powerValue", -127.6).toDouble();
            } else {
                chObj["通道开关"] = "0-关闭";
                chObj["通道功率"] = -127.6;
            }
            channelConfig.append(chObj);
        }
        signalSet["通道配置"] = channelConfig;

        result[1] = signalSet;
    }

    // ===== OPT_INDEX=2: 满天星配置 (仅中高轨) =====
    if (isMediumOrbit) {
        QJsonObject mediumOrbit;

        // ★ 如果scenePath为空，使用默认路径
        QString scenePath = detailData.value("scenePath", "").toString();
        if (scenePath.trimmed().isEmpty()) {
            scenePath = defaultScenePath;
        }
        mediumOrbit["场景路径"] = scenePath;

        mediumOrbit["频点选择"] = detailData.value("freqSelect", "GPS_L1").toString();
        mediumOrbit["轨迹输出"] = detailData.value("trackOutput", "停止输出").toString();

        // 36通道配置
        QVariantList channels = detailData.value("channels").toList();
        QJsonArray channelConfig;
        for (int i = 0; i < 36; ++i) {
            QJsonObject chObj;
            if (i < channels.size()) {
                QVariantMap ch = channels[i].toMap();
                chObj["调制类型"] = ch.value("modulation", "0-关闭").toString();
                chObj["I 功率"] = ch.value("powerI", -127.6).toDouble();
                chObj["Q 功率"] = ch.value("powerQ", -127.6).toDouble();
                chObj["通道开关"] = ch.value("chSwitch", ch.value("channelSwitch", "0-关闭")).toString();
                chObj["频点开关"] = ch.value("freqSwitch", "0-关闭").toString();
            } else {
                chObj["调制类型"] = "0-关闭";
                chObj["I 功率"] = -127.6;
                chObj["Q 功率"] = -127.6;
                chObj["通道开关"] = "0-关闭";
                chObj["频点开关"] = "0-关闭";
            }
            channelConfig.append(chObj);
        }
        mediumOrbit["通道配置"] = channelConfig;

        result[2] = mediumOrbit;
    }

    // ===== OPT_INDEX=3: 安全认证转发源配置 (仅安全认证) =====
    if (isSecurityAuth) {
        QJsonObject securityAuth;

        // 欺骗参数
        QJsonObject deceptionParams;
        deceptionParams["欺骗信号"] = detailData.value("deceptionSignal", "BDS_B1I").toString();
        deceptionParams["功率模式"] = detailData.value("powerMode", "自动模式").toString();
        deceptionParams["初始功率"] = detailData.value("initPower", -130.0).toDouble();
        deceptionParams["星历模式"] = detailData.value("ephemerisMode", "对天接收").toString();
        deceptionParams["欺骗模式"] = detailData.value("deceptionMode", "轨迹欺骗").toString();
        securityAuth["欺骗参数"] = deceptionParams;

        // 欺骗轨迹
        auto round2 = [](double v) -> double {
            return qRound(v * 100.0) / 100.0;
        };

        QJsonObject deceptionTrack;
        deceptionTrack["欺骗轨迹类型"] = detailData.value("trackType", "静态轨迹").toString();
        deceptionTrack["动态轨迹类型"] = detailData.value("dynamicTrackType", "初始状态").toString();
        deceptionTrack["初始状态参数"] = QJsonObject{
            {"经度", round2(detailData.value("initialLon", 116.4074).toDouble())},
            {"纬度", round2(detailData.value("initialLat", 39.9042).toDouble())},
            {"高程", round2(detailData.value("initialAlt", 50.0).toDouble())},
            {"初始速度", round2(detailData.value("initialSpeed", 0.0).toDouble())},
            {"初始方位角", round2(detailData.value("initialAzimuth", 0.0).toDouble())}
        };
        deceptionTrack["匀速直线运动"] = QJsonObject{
            {"运动时长", round2(detailData.value("uniformDuration", 0.0).toDouble())}
        };
        deceptionTrack["匀加速直线运动"] = QJsonObject{
            {"运动时长", round2(detailData.value("accelDuration", 0.0).toDouble())},
            {"匀加速末速度", round2(detailData.value("accelEndSpeed", 0.0).toDouble())}
        };
        deceptionTrack["拐弯参数"] = QJsonObject{
            {"方向改变量", round2(detailData.value("turnDirectionChange", 0.0).toDouble())},
            {"转弯角速度", round2(detailData.value("turnAngularSpeed", 0.0).toDouble())}
        };
        deceptionTrack["俯冲参数"] = QJsonObject{
            {"俯仰角", round2(detailData.value("divePitchAngle", 0.0).toDouble())},
            {"高度改变量", round2(detailData.value("diveAltChange", 0.0).toDouble())}
        };
        deceptionTrack["制动参数"] = QJsonObject{
            {"制动时长", round2(detailData.value("brakeDuration", 0.0).toDouble())}
        };
        deceptionTrack["静态轨迹参数"] = QJsonObject{
            {"经度", round2(detailData.value("longitude", 116.38456).toDouble())},
            {"纬度", round2(detailData.value("latitude", 39.69394).toDouble())},
            {"高程", round2(detailData.value("altitude", 50.0).toDouble())}
        };
        securityAuth["欺骗轨迹"] = deceptionTrack;

        result[3] = securityAuth;
    }

    // ===== OPT_INDEX=4: 压制干扰源配置 (仅抗干扰) =====
    if (isAntiJamming) {
        QJsonObject jamming;
        jamming["连续波设置"] = jammingChannelsToDb(detailData.value("continuousWave").toList(), "连续波设置");
        jamming["脉冲信号设置"] = jammingChannelsToDb(detailData.value("pulseSignal").toList(), "脉冲信号设置");
        jamming["白噪声设置"] = jammingChannelsToDb(detailData.value("whiteNoise").toList(), "白噪声设置");
        jamming["调幅信号设置"] = jammingChannelsToDb(detailData.value("amSignal").toList(), "调幅信号设置");
        jamming["扩频信号设置"] = jammingChannelsToDb(detailData.value("spreadSpectrum").toList(), "扩频信号设置");
        jamming["扫频信号设置"] = jammingChannelsToDb(detailData.value("sweepFreq").toList(), "扫频信号设置");
        jamming["调频信号设置"] = jammingChannelsToDb(detailData.value("fmSignal").toList(), "调频信号设置");
        result[4] = jamming;
    }

    // ===== OPT_INDEX=5: 诱骗干扰源配置 (仅抗干扰) =====
    if (isAntiJamming) {
        QJsonObject deception;
        deception["诱骗位置"] = QJsonObject{
            {"经度", detailData.value("deceptionLon", 0.0).toDouble()},
            {"纬度", detailData.value("deceptionLat", 0.0).toDouble()},
            {"高程", detailData.value("deceptionAlt", 0.0).toDouble()}
        };

        // 13个频点配置
        QJsonObject deceptionParams;
        QVariantList freqList = detailData.value("deceptionFreq").toList();
        QStringList freqNames = {"GPS_L1", "GLO_G2", "GPS_L5", "BDS_B2I", "BDS_B3I",
                                 "BDS_B1C", "GAL_E5A", "GAL_E5B", "BDS_B2A", "BDS_B1I",
                                 "GLO_G1", "GAL_E1", "GPS_L2"};
        for (int i = 0; i < 13; ++i) {
            if (i < freqList.size()) {
                QVariantMap freqData = freqList[i].toMap();
                QJsonObject freqObj;
                freqObj["发射开关"] = freqData.value("switch", "打开").toString();
                freqObj["功率"] = freqData.value("power", -127.6).toDouble();
                freqObj["延迟"] = freqData.value("delay", -127.6).toDouble();
                deceptionParams[freqNames[i]] = freqObj;
            }
        }
        deception["诱骗参数"] = deceptionParams;
        deception["诱骗重启"] = detailData.value("deceptionReboot", "REBOOT").toString();

        result[5] = deception;
    }

    // ===== RMO =====
    QString openMode = detailData.value("rmoOpenMode", "CLOSE").toString();
    if (openMode == "打开指令" || openMode == "打开全部" || openMode == "OPEN") {
        openMode = "OPEN";
    } else {
        openMode = "CLOSE";
    }
    QJsonObject rmo;
    rmo["打开模式"] = openMode;
    rmo["目标语句"] = detailData.value("rmoTargetStatement", "GGA").toString();
    rmo["目标语句输出频度"] = QString::number(detailData.value("rmoOutputFreq", 1).toInt());
    result[6] = rmo;

    // ===== 评估 =====
    QJsonObject evaluation;
    evaluation["GNSS定位精度及定位数据更新率"] = true;
    QJsonObject threshold;
    threshold["圆概率"] = 95;
    threshold["水平"] = detailData.value("thEastCm", 10.0).toDouble();
    threshold["垂直"] = detailData.value("thUpCm", 10.0).toDouble();
    evaluation["指标"] = QJsonArray{QJsonObject{{"参数", threshold}, {"名称", "GNSS定位精度"}}};
    result[7] = evaluation;

    return result;
}

QString SceneDetailMapper::getStepName(int optIndex, const QString &testType, const QString &sceneType)
{
    bool isMediumOrbit = (sceneType == "中高轨导航终端性能测试");
    bool isAntiJamming = (sceneType == "导航用户装备抗干扰性能试验");
    bool isSecurityAuth = (sceneType == "安全认证测试终端试验");

    switch (optIndex) {
        case 0: return "场景配置";  // 原仿真开始
        case 1: return isMediumOrbit ? "" : "差分信号设置";  // 非中高轨使用差分信号设置
        case 2: return isMediumOrbit ? "满天星配置" : "";
        case 3: return isSecurityAuth ? "安全认证转发源配置" : "";
        case 4: return isAntiJamming ? "压制干扰源配置" : "";
        case 5: return isAntiJamming ? "诱骗干扰源配置" : "";
        case 6: return "RMO";
        case 7: return "评估";
        default: return "";
    }
}

QString SceneDetailMapper::getStepType(int optIndex, const QString &testType, const QString &sceneType)
{
    bool isMediumOrbit = (sceneType == "中高轨导航终端性能测试");
    bool isAntiJamming = (sceneType == "导航用户装备抗干扰性能试验");
    bool isSecurityAuth = (sceneType == "安全认证测试终端试验");

    switch (optIndex) {
        case 0: return "信号源控制";
        case 1: return isMediumOrbit ? "" : "信号源控制";
        case 2: return isMediumOrbit ? "信号源控制" : "";
        case 3: return isSecurityAuth ? "信号源控制" : "";
        case 4: return isAntiJamming ? "信号源控制" : "";
        case 5: return isAntiJamming ? "信号源控制" : "";
        case 6: return "待测设备";
        case 7: return "流程逻辑控制";
        default: return "";
    }
}

QVariantMap SceneDetailMapper::dbStepsToMap(const QVector<SceneStep> &steps, const QString &testType)
{
    QVariantMap result;

    SCENE_LOG_CAT("DB_TO_MAP", QString("steps=%1, testType=%2").arg(steps.size()).arg(testType));

    if (steps.isEmpty()) {
        SCENE_LOG_ERROR("DB_TO_MAP", "steps is empty!");
        return result;
    }

    result["testType"] = testType;

    // 从步骤列表推断场景类型（用于数据隔离）
    QString sceneType;

    // 1. 首先从步骤内容中读取场景类型（保存时明确写入的，最可靠）
    for (const auto &step : steps) {
        if (step.optContent.contains("场景类型")) {
            sceneType = step.optContent.value("场景类型").toString();
            SCENE_LOG_CAT("SCENE_TYPE", QString("found in step content: %1").arg(sceneType));
            break;
        }
        // 兼容旧数据
        if (step.optContent.contains("sceneType")) {
            sceneType = step.optContent.value("sceneType").toString();
            SCENE_LOG_CAT("SCENE_TYPE", QString("found in step content (legacy): %1").arg(sceneType));
            break;
        }
    }

    // 2. 如果步骤内容中没有场景类型，才使用步骤名推断作为回退
    if (sceneType.isEmpty()) {
        for (const auto &step : steps) {
            if (step.optName == "轨迹输出") { sceneType = "中高轨导航终端性能测试"; break; }
            if (step.optName == "欺骗参数" || step.optName == "轨迹配置") { sceneType = "安全认证测试终端试验"; break; }
            if (step.optName == "连续波设置" || step.optName == "诱骗位置" || step.optName == "诱骗参数") {
                sceneType = "导航用户装备抗干扰性能试验"; break;
            }
        }
        if (!sceneType.isEmpty()) {
            SCENE_LOG_CAT("SCENE_TYPE", QString("inferred from step name: %1").arg(sceneType));
        }
    }

    if (!sceneType.isEmpty()) {
        result["sceneType"] = sceneType;
    }

    SCENE_LOG_CAT("INFERRED_TYPE", QString("sceneType=%1").arg(sceneType));

    // 场景类型判断
    bool isMediumOrbit = (sceneType == "中高轨导航终端性能测试");
    bool isAntiJamming = (sceneType == "导航用户装备抗干扰性能试验");
    bool isSecurityAuth = (sceneType == "安全认证测试终端试验");
    bool isLowOrbit = (sceneType == "低轨导航终端性能测试");
    bool isLunarSpace = (sceneType == "地月空间终端设备性能测试");

    for (const auto &step : steps) {
        const QJsonObject &content = step.optContent;
        SCENE_LOG_CAT("STEP_PROCESS", QString("step: %1, index: %2, content keys: %3")
                  .arg(step.optName).arg(step.optIndex)
                  .arg(QStringList(content.keys()).join(",")));

        // 兼容新旧步骤名：场景配置（新）或 仿真开始（旧）
        if (step.optName == "场景配置" || step.optName == "仿真开始") {
            result["filePath"] = content.value("仿真场景文件名").toString();
            result["startTime"] = content.value("开始时间").toString("2025/12/16 12:00:00");
            result["endTime"] = content.value("结束时间").toString("2025/12/16 14:00:00");
            result["scenePath"] = content.value("场景存储路径").toString();
        }
        else if (step.optName == "信号设置" || step.optName == "满天星信号设置" || step.optName == "差分信号设置") {
            if (isMediumOrbit || step.optName == "满天星信号设置") {
                // 中高轨信号设置
                result["freqSelect"] = content.value("频点选择").toString();

                if (content.contains("场景路径")) {
                    result["scenePath"] = content.value("场景路径").toString();
                }

                SCENE_LOG_CAT("MEDIUM_ORBIT_LOAD", QString("freqSelect=%1, scenePath=%2")
                    .arg(result["freqSelect"].toString())
                    .arg(result["scenePath"].toString()));

                QJsonArray chArray = content.value("通道配置").toArray();
                SCENE_LOG_CAT("MEDIUM_ORBIT_LOAD", QString("chArray size: %1").arg(chArray.size()));

                if (chArray.size() > 0) {
                    QJsonObject firstCh = chArray[0].toObject();
                    SCENE_LOG_CAT("MEDIUM_ORBIT_RAW", QString("ch[0] raw: modulation=%1, powerI=%2, chSwitch=%3, freqSwitch=%4")
                        .arg(firstCh.value("调制类型").toString())
                        .arg(firstCh.value("I 功率").toDouble())
                        .arg(firstCh.value("通道开关").toString())
                        .arg(firstCh.value("频点开关").toString()));
                }

                QVariantList channels = channelsFromDb_MediumOrbit(chArray);
                SCENE_LOG_CAT("MEDIUM_ORBIT_LOAD", QString("parsed channels size: %1").arg(channels.size()));

                result["channels"] = channels;

                for (int i = 0; i < qMin(3, channels.size()); ++i) {
                    QVariantMap ch = channels[i].toMap();
                    SCENE_LOG_CAT("MEDIUM_ORBIT_CHANNEL", QString("ch[%1]: modulation=%2, powerI=%3, chSwitch=%4")
                        .arg(i)
                        .arg(ch.value("modulation").toString())
                        .arg(ch.value("powerI").toDouble())
                        .arg(ch.value("chSwitch").toString()));
                }
            } else {
                // 单频点信号设置（24通道）
                if (content.contains("通道配置")) {
                    // 新格式：对象数组
                    QString freq = content.value("频点选择").toString();
                    if (isLowOrbit) {
                        result["lowOrbitFreq"] = freq;
                    } else if (isLunarSpace) {
                        result["lunarSpaceFreq"] = freq;
                    } else if (isSecurityAuth) {
                        result["securityAuthFreq"] = freq;
                    } else if (isAntiJamming) {
                        result["antiJammingFreq"] = freq;
                    } else {
                        result["globalFreq"] = freq;
                    }
                    result["globalFreq"] = freq;

                    QJsonArray chArray = content.value("通道配置").toArray();
                    QVariantList channels;
                    for (int i = 0; i < 24; ++i) {
                        QVariantMap ch;
                        if (i < chArray.size()) {
                            QJsonObject chObj = chArray[i].toObject();
                            ch["switchType"] = chObj.value("通道开关").toString() == "1-打开" ? "开启" : "关闭";
                            ch["powerValue"] = chObj.value("通道功率").toDouble(-127.6);
                        } else {
                            ch["switchType"] = "关闭";
                            ch["powerValue"] = -127.6;
                        }
                        ch["powerQ"] = ch["powerValue"].toDouble();
                        ch["modulation"] = "QPSK";
                        channels.append(ch);
                    }

                    if (isLowOrbit) {
                        result["lowOrbitChannels"] = channels;
                    } else if (isLunarSpace) {
                        result["lunarSpaceChannels"] = channels;
                    } else if (isSecurityAuth) {
                        result["securityAuthChannels"] = channels;
                    } else if (isAntiJamming) {
                        result["antiJammingChannels"] = channels;
                    } else {
                        result["channels"] = channels;
                    }
                    result["channels"] = channels;
                } else {
                    // 兼容旧格式：I支路功率等平行数组
                    QString freq = content.value("频点").toString();

                    if (isLowOrbit) {
                        result["lowOrbitFreq"] = freq;
                    } else if (isLunarSpace) {
                        result["lunarSpaceFreq"] = freq;
                    } else if (isSecurityAuth) {
                        result["securityAuthFreq"] = freq;
                    } else if (isAntiJamming) {
                        result["antiJammingFreq"] = freq;
                    } else {
                        result["globalFreq"] = freq;
                    }
                    result["globalFreq"] = freq;

                    QJsonArray iPowerArray = content.value("I支路功率").toArray();
                    QJsonArray qPowerArray = content.value("Q支路功率").toArray();
                    QJsonArray modulationArray = content.value("调制方式").toArray();
                    QJsonArray switchArray = content.value("通道开关").toArray();

                    QVariantList channels;
                    for (int i = 0; i < 24; ++i) {
                        QVariantMap ch;
                        ch["powerValue"] = i < iPowerArray.size() ? iPowerArray[i].toDouble() : -130.0;
                        ch["powerQ"] = i < qPowerArray.size() ? qPowerArray[i].toDouble() : -130.0;
                        ch["modulation"] = i < modulationArray.size() ? modulationArray[i].toString() : "QPSK";
                        ch["switchType"] = i < switchArray.size() ? (switchArray[i].toBool() ? "开启" : "关闭") : "关闭";
                        channels.append(ch);
                    }

                    if (isLowOrbit) {
                        result["lowOrbitChannels"] = channels;
                    } else if (isLunarSpace) {
                        result["lunarSpaceChannels"] = channels;
                    } else if (isSecurityAuth) {
                        result["securityAuthChannels"] = channels;
                    } else if (isAntiJamming) {
                        result["antiJammingChannels"] = channels;
                    } else {
                        result["channels"] = channels;
                    }
                    result["channels"] = channels;
                }
            }
        }
        else if (step.optName == "轨迹输出") {
            result["trackOutput"] = content.value("轨迹输出").toString();
        }
        else if (step.optName == "仿真误差参数") {
            result["simulationErrorType"] = content.value("误差类型").toString("大气误差");
        }
        else if (step.optName == "目标轨迹参数") {
            result["trackType"] = content.value("轨迹类型").toString();
            result["dynamicTrackType"] = content.value("动态轨迹类型").toString();

            QJsonObject initialParams = content.value("初始状态参数").toObject();
            result["initialLon"] = initialParams.value("经度").toDouble();
            result["initialLat"] = initialParams.value("纬度").toDouble();
            result["initialAlt"] = initialParams.value("高程").toDouble();
            result["initialSpeed"] = initialParams.value("初始速度").toDouble();
            result["initialAzimuth"] = initialParams.value("初始方位角").toDouble();

            QJsonObject uniformParams = content.value("匀速直线运动").toObject();
            result["uniformDuration"] = uniformParams.value("运动时长").toDouble();

            QJsonObject accelParams = content.value("匀加速直线运动").toObject();
            result["accelDuration"] = accelParams.value("运动时长").toDouble();
            result["accelEndSpeed"] = accelParams.value("匀加速末速度").toDouble();

            QJsonObject turnParams = content.value("拐弯参数").toObject();
            result["turnDirectionChange"] = turnParams.value("方向改变量").toDouble();
            result["turnAngularSpeed"] = turnParams.value("转弯角速度").toDouble();

            QJsonObject diveParams = content.value("俯冲参数").toObject();
            result["divePitchAngle"] = diveParams.value("俯仰角").toDouble();
            result["diveAltChange"] = diveParams.value("高度改变量").toDouble();

            QJsonObject brakeParams = content.value("制动参数").toObject();
            result["brakeDuration"] = brakeParams.value("制动时长").toDouble();

            QJsonObject staticParams = content.value("静态轨迹参数").toObject();
            result["longitude"] = staticParams.value("经度").toDouble();
            result["latitude"] = staticParams.value("纬度").toDouble();
            result["altitude"] = staticParams.value("高程").toDouble();
        }
        else if (step.optName == "欺骗参数") {
            result["deceptionSignal"] = content.value("欺骗信号").toString();
            result["powerMode"] = content.value("功率模式").toString();
            result["initPower"] = content.value("初始功率").toDouble();
            result["ephemerisMode"] = content.value("星历模式").toString();
            result["deceptionMode"] = content.value("欺骗模式").toString();
        }
        else if (step.optName == "轨迹配置") {
            result["trackType"] = content.value("欺骗轨迹类型").toString();
            result["dynamicTrackType"] = content.value("动态轨迹类型").toString();

            QJsonObject initialParams = content.value("初始状态参数").toObject();
            result["initialLon"] = initialParams.value("经度").toDouble();
            result["initialLat"] = initialParams.value("纬度").toDouble();
            result["initialAlt"] = initialParams.value("高程").toDouble();
            result["initialSpeed"] = initialParams.value("初始速度").toDouble();
            result["initialAzimuth"] = initialParams.value("初始方位角").toDouble();

            QJsonObject uniformParams = content.value("匀速直线运动").toObject();
            result["uniformDuration"] = uniformParams.value("运动时长").toDouble();

            QJsonObject accelParams = content.value("匀加速直线运动").toObject();
            result["accelDuration"] = accelParams.value("运动时长").toDouble();
            result["accelEndSpeed"] = accelParams.value("匀加速末速度").toDouble();

            QJsonObject turnParams = content.value("拐弯参数").toObject();
            result["turnDirectionChange"] = turnParams.value("方向改变量").toDouble();
            result["turnAngularSpeed"] = turnParams.value("转弯角速度").toDouble();

            QJsonObject diveParams = content.value("俯冲参数").toObject();
            result["divePitchAngle"] = diveParams.value("俯仰角").toDouble();
            result["diveAltChange"] = diveParams.value("高度改变量").toDouble();

            QJsonObject brakeParams = content.value("制动参数").toObject();
            result["brakeDuration"] = brakeParams.value("制动时长").toDouble();

            QJsonObject staticParams = content.value("静态轨迹参数").toObject();
            result["longitude"] = staticParams.value("经度").toDouble();
            result["latitude"] = staticParams.value("纬度").toDouble();
            result["altitude"] = staticParams.value("高程").toDouble();
        }
        else if (step.optName == "连续波设置") {
            result["continuousWave"] = jammingChannelsFromDb(content.value("配置").toObject());
        }
        else if (step.optName == "脉冲信号设置") {
            result["pulseSignal"] = jammingChannelsFromDb(content.value("配置").toObject());
        }
        else if (step.optName == "白噪声设置") {
            result["whiteNoise"] = jammingChannelsFromDb(content.value("配置").toObject());
        }
        else if (step.optName == "调幅信号设置") {
            result["amSignal"] = jammingChannelsFromDb(content.value("配置").toObject());
        }
        else if (step.optName == "扩频信号设置") {
            result["spreadSpectrum"] = jammingChannelsFromDb(content.value("配置").toObject());
        }
        else if (step.optName == "扫频信号设置") {
            result["sweepFreq"] = jammingChannelsFromDb(content.value("配置").toObject());
        }
        else if (step.optName == "调频信号设置") {
            result["fmSignal"] = jammingChannelsFromDb(content.value("配置").toObject());
        }
        else if (step.optName == "诱骗位置") {
            result["deceptionLon"] = content.value("经度").toDouble();
            result["deceptionLat"] = content.value("纬度").toDouble();
            result["deceptionAlt"] = content.value("高程").toDouble();
        }
        else if (step.optName == "诱骗参数") {
            QVariantList freqList;
            QStringList freqNames = {"GPS_L1", "GLO_G2", "GPS_L5", "BDS_B2I", "BDS_B3I",
                                     "BDS_B1C", "GAL_E5A", "GAL_E5B", "BDS_B2A", "BDS_B1I",
                                     "GLO_G1", "GAL_E1", "GPS_L2"};
            for (const QString &freqName : freqNames) {
                QJsonObject freqObj = content.value(freqName).toObject();
                QVariantMap freqData;
                freqData["switch"] = freqObj.value("发射开关").toString();
                freqData["power"] = freqObj.value("功率").toDouble();
                freqData["delay"] = freqObj.value("延迟").toDouble();
                freqList.append(freqData);
            }
            result["deceptionFreq"] = freqList;
        }
        else if (step.optName == "诱骗重启") {
            result["deceptionReboot"] = content.value("重启参数").toString();
        }
        else if (step.optName == "干扰参数") {
            result["jammerCount"] = content.value("干扰机数量").toString("1");
            result["interventionTime"] = content.value("介入时间").toString();
            result["jammingDuration"] = content.value("持续时间").toString();
            result["jammingDirection"] = content.value("干扰方向").toString();
            result["jammingMode"] = content.value("干扰方式").toString("固定模式");
            result["jammingPower"] = content.value("干扰功率").toString();
        }
        else if (step.optName == "RMO") {
            result["rmoOpenMode"] = content.value("打开模式").toString();
            result["rmoTargetStatement"] = content.value("目标语句").toString();
            result["rmoOutputFreq"] = content.value("目标语句输出频度").toInt();
        }
        else if (step.optName == "评估类型") {
            // 新格式：评估方式为数组（单选），评估类型为字符串，阈值配置为对象
            if (content.contains("评估方式")) {
                QJsonArray evalMethods = content.value("评估方式").toArray();
                if (evalMethods.size() > 0) {
                    QString method = evalMethods[0].toString();
                    result["evalMethod"] = method;
                    result["evalRms"] = (method == "RMS统计");
                    result["evalCpe"] = (method == "CPE统计");
                    result["eval95"] = (method == "95%统计");
                }

                result["testType"] = content.value("评估类型").toString("定位精度");

                // 读取阈值：首次定位时间用时间阈值，其他用方向阈值
                if (content.contains("时间阈值")) {
                    result["timeThreshold"] = content.value("时间阈值").toDouble(30.0);
                }
                if (content.contains("阈值配置")) {
                    QJsonObject threshold = content.value("阈值配置").toObject();
                    result["thEast"] = threshold.value("东方向").toDouble(10.0);
                    result["thNorth"] = threshold.value("北方向").toDouble(10.0);
                    result["thUp"] = threshold.value("垂直方向").toDouble(10.0);
                    result["thEastCm"] = result["thEast"];
                    result["thNorthCm"] = result["thNorth"];
                    result["thUpCm"] = result["thUp"];
                }
            }
            // 兼容旧格式：布尔值形式
            else {
                result["evalRms"] = content.value("RMS统计").toBool();
                result["evalCpe"] = content.value("CPE统计").toBool();
                result["eval95"] = content.value("95%统计").toBool();

                if (content.value("定位精度").toBool()) {
                    result["testType"] = "定位精度";
                } else if (content.value("测速精度").toBool()) {
                    result["testType"] = "测速精度";
                } else if (content.value("首次定位时间").toBool()) {
                    result["testType"] = "首次定位时间";
                } else if (content.value("抗干扰性能").toBool()) {
                    result["testType"] = "抗干扰性能测试";
                } else if (content.value("安全认证性能").toBool()) {
                    result["testType"] = "安全认证性能测试";
                } else if (content.value("地月空间定位能力").toBool()) {
                    result["testType"] = "地月空间定位能力测试";
                }
            }
        }
        // 兼容旧格式：单独的阈值配置步骤
        else if (step.optName == "阈值配置") {
            result["thEast"] = content.value("东方向").toDouble();
            result["thNorth"] = content.value("北方向").toDouble();
            result["thUp"] = content.value("垂直方向").toDouble();
            result["timeThreshold"] = content.value("时间阈值").toDouble(30.0);
            // 兼容旧字段名
            result["thEastCm"] = result["thEast"];
            result["thNorthCm"] = result["thNorth"];
            result["thUpCm"] = result["thUp"];
        }
        else if (step.optName == "数据接收") {
            result["dataCollectionCount"] = content.value("数据采集").toInt(100);
        }
        // 兼容旧数据格式
        else if (step.optName == "单频点信号设置") {
            QString freq;
            if (content.contains("频点选择")) {
                freq = content.value("频点选择").toString();
            } else {
                freq = content.value("频点").toString();
            }

            if (isLowOrbit) {
                result["lowOrbitFreq"] = freq;
            } else if (isLunarSpace) {
                result["lunarSpaceFreq"] = freq;
            } else if (isSecurityAuth) {
                result["securityAuthFreq"] = freq;
            } else if (isAntiJamming) {
                result["antiJammingFreq"] = freq;
            } else {
                result["globalFreq"] = freq;
            }
            result["globalFreq"] = freq;

            QVariantList channels;
            if (content.contains("通道配置")) {
                // 新格式：对象数组
                QJsonArray chArray = content.value("通道配置").toArray();
                for (int i = 0; i < 24; ++i) {
                    QVariantMap ch;
                    if (i < chArray.size()) {
                        QJsonObject chObj = chArray[i].toObject();
                        ch["switchType"] = chObj.value("通道开关").toString() == "1-打开" ? "开启" : "关闭";
                        ch["powerValue"] = chObj.value("通道功率").toDouble(-127.6);
                    } else {
                        ch["switchType"] = "关闭";
                        ch["powerValue"] = -127.6;
                    }
                    ch["powerQ"] = ch["powerValue"].toDouble();
                    ch["modulation"] = "QPSK";
                    channels.append(ch);
                }
            } else {
                // 旧格式：平行数组
                QJsonArray iPowerArray = content.value("I支路功率").toArray();
                QJsonArray qPowerArray = content.value("Q支路功率").toArray();
                QJsonArray modulationArray = content.value("调制方式").toArray();
                QJsonArray switchArray = content.value("通道开关").toArray();

                for (int i = 0; i < 24; ++i) {
                    QVariantMap ch;
                    ch["powerValue"] = i < iPowerArray.size() ? iPowerArray[i].toDouble() : -130.0;
                    ch["powerQ"] = i < qPowerArray.size() ? qPowerArray[i].toDouble() : -130.0;
                    ch["modulation"] = i < modulationArray.size() ? modulationArray[i].toString() : "QPSK";
                    ch["switchType"] = i < switchArray.size() ? (switchArray[i].toBool() ? "开启" : "关闭") : "关闭";
                    channels.append(ch);
                }
            }

            if (isLowOrbit) {
                result["lowOrbitChannels"] = channels;
            } else if (isLunarSpace) {
                result["lunarSpaceChannels"] = channels;
            } else if (isSecurityAuth) {
                result["securityAuthChannels"] = channels;
            } else if (isAntiJamming) {
                result["antiJammingChannels"] = channels;
            } else {
                result["channels"] = channels;
            }
            result["channels"] = channels;
        }
        else if (step.optName == "满天星配置") {
            result["scenePath"] = content.value("场景路径").toString();
            result["filePath"] = content.value("场景路径").toString();
            result["freqSelect"] = content.value("频点选择").toString();
            result["trackOutput"] = content.value("轨迹输出").toString();

            SCENE_LOG_CAT("MEDIUM_ORBIT_LOAD", QString("scenePath=%1, freqSelect=%2, trackOutput=%3")
                .arg(result["scenePath"].toString())
                .arg(result["freqSelect"].toString())
                .arg(result["trackOutput"].toString()));

            QJsonArray chArray = content.value("通道配置").toArray();
            SCENE_LOG_CAT("MEDIUM_ORBIT_LOAD", QString("chArray size: %1").arg(chArray.size()));

            if (chArray.size() > 0) {
                QJsonObject firstCh = chArray[0].toObject();
                SCENE_LOG_CAT("MEDIUM_ORBIT_RAW", QString("ch[0] raw: modulation=%1, powerI=%2, chSwitch=%3, freqSwitch=%4")
                    .arg(firstCh.value("调制类型").toString())
                    .arg(firstCh.value("I 功率").toDouble())
                    .arg(firstCh.value("通道开关").toString())
                    .arg(firstCh.value("频点开关").toString()));
            }

            QVariantList channels = channelsFromDb_MediumOrbit(chArray);
            SCENE_LOG_CAT("MEDIUM_ORBIT_LOAD", QString("parsed channels size: %1").arg(channels.size()));

            result["channels"] = channels;

            for (int i = 0; i < qMin(3, channels.size()); ++i) {
                QVariantMap ch = channels[i].toMap();
                SCENE_LOG_CAT("MEDIUM_ORBIT_CHANNEL", QString("ch[%1]: modulation=%2, powerI=%3, chSwitch=%4")
                    .arg(i)
                    .arg(ch.value("modulation").toString())
                    .arg(ch.value("powerI").toDouble())
                    .arg(ch.value("chSwitch").toString()));
            }
        }
        else if (step.optName == "安全认证转发源配置") {
            QJsonObject deceptionParams = content.value("欺骗参数").toObject();
            result["deceptionSignal"] = deceptionParams.value("欺骗信号").toString();
            result["powerMode"] = deceptionParams.value("功率模式").toString();
            result["initPower"] = deceptionParams.value("初始功率").toDouble();
            result["ephemerisMode"] = deceptionParams.value("星历模式").toString();
            result["deceptionMode"] = deceptionParams.value("欺骗模式").toString();

            QJsonObject deceptionTrack = content.value("欺骗轨迹").toObject();
            result["trackType"] = deceptionTrack.value("欺骗轨迹类型").toString();
            result["dynamicTrackType"] = deceptionTrack.value("动态轨迹类型").toString();

            QJsonObject initialParams = deceptionTrack.value("初始状态参数").toObject();
            result["initialLon"] = initialParams.value("经度").toDouble();
            result["initialLat"] = initialParams.value("纬度").toDouble();
            result["initialAlt"] = initialParams.value("高程").toDouble();
            result["initialSpeed"] = initialParams.value("初始速度").toDouble();
            result["initialAzimuth"] = initialParams.value("初始方位角").toDouble();

            QJsonObject uniformParams = deceptionTrack.value("匀速直线运动").toObject();
            result["uniformDuration"] = uniformParams.value("运动时长").toDouble();

            QJsonObject accelParams = deceptionTrack.value("匀加速直线运动").toObject();
            result["accelDuration"] = accelParams.value("运动时长").toDouble();
            result["accelEndSpeed"] = accelParams.value("匀加速末速度").toDouble();

            QJsonObject turnParams = deceptionTrack.value("拐弯参数").toObject();
            result["turnDirectionChange"] = turnParams.value("方向改变量").toDouble();
            result["turnAngularSpeed"] = turnParams.value("转弯角速度").toDouble();

            QJsonObject diveParams = deceptionTrack.value("俯冲参数").toObject();
            result["divePitchAngle"] = diveParams.value("俯仰角").toDouble();
            result["diveAltChange"] = diveParams.value("高度改变量").toDouble();

            QJsonObject brakeParams = deceptionTrack.value("制动参数").toObject();
            result["brakeDuration"] = brakeParams.value("制动时长").toDouble();

            QJsonObject staticParams = deceptionTrack.value("静态轨迹参数").toObject();
            result["longitude"] = staticParams.value("经度").toDouble();
            result["latitude"] = staticParams.value("纬度").toDouble();
            result["altitude"] = staticParams.value("高程").toDouble();
        }
        else if (step.optName == "压制干扰源配置") {
            result["continuousWave"] = jammingChannelsFromDb(content.value("连续波设置").toObject());
            result["pulseSignal"] = jammingChannelsFromDb(content.value("脉冲信号设置").toObject());
            result["whiteNoise"] = jammingChannelsFromDb(content.value("白噪声设置").toObject());
            result["amSignal"] = jammingChannelsFromDb(content.value("调幅信号设置").toObject());
            result["spreadSpectrum"] = jammingChannelsFromDb(content.value("扩频信号设置").toObject());
            result["sweepFreq"] = jammingChannelsFromDb(content.value("扫频信号设置").toObject());
            result["fmSignal"] = jammingChannelsFromDb(content.value("调频信号设置").toObject());
        }
        else if (step.optName == "诱骗干扰源配置") {
            QJsonObject deceptionPosition = content.value("诱骗位置").toObject();
            result["deceptionLon"] = deceptionPosition.value("经度").toDouble();
            result["deceptionLat"] = deceptionPosition.value("纬度").toDouble();
            result["deceptionAlt"] = deceptionPosition.value("高程").toDouble();

            QJsonObject freqParams = content.value("诱骗参数").toObject();
            QVariantList freqList;
            QStringList freqNames = {"GPS_L1", "GLO_G2", "GPS_L5", "BDS_B2I", "BDS_B3I",
                                     "BDS_B1C", "GAL_E5A", "GAL_E5B", "BDS_B2A", "BDS_B1I",
                                     "GLO_G1", "GAL_E1", "GPS_L2"};
            for (const QString &freqName : freqNames) {
                QJsonObject freqObj = freqParams.value(freqName).toObject();
                QVariantMap freqData;
                freqData["switch"] = freqObj.value("发射开关").toString();
                freqData["power"] = freqObj.value("功率").toDouble();
                freqData["delay"] = freqObj.value("延迟").toDouble();
                freqList.append(freqData);
            }
            result["deceptionFreq"] = freqList;
            result["deceptionReboot"] = content.value("诱骗重启").toString();
        }
        else if (step.optName == "评估") {
            QJsonArray indicators = content.value("指标").toArray();
            if (!indicators.isEmpty()) {
                QJsonObject threshold = indicators[0].toObject().value("参数").toObject();
                result["thEastCm"] = threshold.value("水平").toDouble();
                result["thUpCm"] = threshold.value("垂直").toDouble();
            }
        }
    }

    SCENE_LOG_CAT("LOAD_FIELDS", QString("keys: %1").arg(result.keys().join(", ")));
    if (result.contains("channels")) {
        SCENE_LOG_CAT("LOAD_CHANNELS", QString("channels count: %1").arg(result.value("channels").toList().size()));
    }
    if (result.contains("scenePath")) {
        SCENE_LOG_CAT("LOAD_SCENE_PATH", QString("scenePath: %1").arg(result.value("scenePath").toString()));
    }
    if (result.contains("freqSelect")) {
        SCENE_LOG_CAT("LOAD_FREQ", QString("freqSelect: %1").arg(result.value("freqSelect").toString()));
    }

    return result;
}

