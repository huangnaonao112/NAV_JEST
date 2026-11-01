#include "SceneDetailMapper.h"
#include "utils/SceneLogger.h"

#include <QVariantList>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>

QVector<SceneDetailMapper::StepBuildResult> SceneDetailMapper::buildSteps(const QVariantMap &detailData)
{
    QVector<StepBuildResult> result;

    QString testType = detailData.value("testType").toString();
    QString sceneType = detailData.value("sceneType").toString();

    // 场景类型判断
    bool isMediumOrbit = (sceneType == "中高轨导航终端性能测试");
    bool isAntiJamming = (sceneType == "导航用户装备抗干扰性能试验");
    bool isSecurityAuth = (sceneType == "安全认证测试终端试验");
    bool isLowOrbit = (sceneType == "低轨导航终端性能测试");
    bool isLunarSpace = (sceneType == "地月空间终端设备性能测试");

    int currentIndex = 0;

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

    // ===== 步骤0: 场景配置 (原仿真开始) =====
    {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        step.optName = "场景配置";
        step.optType = "信号源控制";

        QJsonObject content;
        content["仿真场景文件名"] = detailData.value("filePath", "").toString();
        content["等待模拟器稳定时间"] = 20;
        content["通道总数"] = isMediumOrbit ? 36 : 24;
        content["场景类型"] = sceneType;
        content["开始时间"] = detailData.value("startTime", "2025/12/16 12:00:00").toString();
        content["结束时间"] = detailData.value("endTime", "2025/12/16 14:00:00").toString();

        // ★ 如果scenePath为空，使用默认路径
        QString scenePath = detailData.value("scenePath", "").toString();
        if (scenePath.trimmed().isEmpty()) {
            scenePath = defaultScenePath;
        }
        content["场景存储路径"] = scenePath;

        step.optContent = content;

        result.append(step);
    }

    // ===== 步骤1: 初始化 =====
    {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        step.optName = "初始化";
        step.optType = "信号源控制";

        QJsonObject content;
        content["信息类型"] = "0xFFFFFF02";
        step.optContent = content;

        result.append(step);
    }

    // ===== 步骤2: 启动 =====
    {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        step.optName = "启动";
        step.optType = "信号源控制";

        QJsonObject content;
        content["信息类型"] = "0xFFFFFF03";
        step.optContent = content;

        result.append(step);
    }

    // ===== 步骤3: 停止 =====
    {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        step.optName = "停止";
        step.optType = "信号源控制";

        QJsonObject content;
        content["信息类型"] = "0xFFFFFF01";
        step.optContent = content;

        result.append(step);
    }

    // ===== 步骤4: 信号设置 =====
    {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        // 根据场景类型区分：满天星(中高轨) / 差分信号(其他)
        step.optName = isMediumOrbit ? "满天星信号设置" : "差分信号设置";
        step.optType = "信号源控制";

        QJsonObject content;

        if (isMediumOrbit) {
            // 中高轨：满天星信号设置
            content["频点选择"] = detailData.value("freqSelect", "GPS_L1").toString();

            // ★ 添加场景路径
            QString scenePath = detailData.value("scenePath", "").toString();
            if (scenePath.trimmed().isEmpty()) {
                scenePath = defaultScenePath;
            }
            content["场景路径"] = scenePath;

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
            content["通道配置"] = channelConfig;
        } else {
            // 其他场景：差分信号设置（24通道对象数组格式，参考满天星信号设置）
            QString freq;
            if (isAntiJamming) {
                freq = detailData.value("antiJammingFreq", detailData.value("globalFreq", "GPS_L1")).toString();
            } else if (isSecurityAuth) {
                freq = detailData.value("securityAuthFreq", detailData.value("globalFreq", "GPS_L1")).toString();
            } else if (isLowOrbit) {
                freq = detailData.value("lowOrbitFreq", detailData.value("globalFreq", "GPS_L1")).toString();
            } else if (isLunarSpace) {
                freq = detailData.value("lunarSpaceFreq", detailData.value("globalFreq", "GPS_L1")).toString();
            } else {
                freq = detailData.value("globalFreq", "GPS_L1").toString();
            }
            content["频点选择"] = freq;

            QVariantList channels;
            if (isAntiJamming) {
                channels = detailData.value("antiJammingChannels", detailData.value("channels")).toList();
            } else if (isSecurityAuth) {
                channels = detailData.value("securityAuthChannels", detailData.value("channels")).toList();
            } else if (isLowOrbit) {
                channels = detailData.value("lowOrbitChannels", detailData.value("channels")).toList();
            } else if (isLunarSpace) {
                channels = detailData.value("lunarSpaceChannels", detailData.value("channels")).toList();
            } else {
                channels = detailData.value("channels").toList();
            }

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
            content["通道配置"] = channelConfig;
        }
        step.optContent = content;
        result.append(step);
    }

    // ===== 步骤6: 轨迹输出 (仅中高轨) =====
    if (isMediumOrbit) {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        step.optName = "轨迹输出";
        step.optType = "信号源控制";

        QJsonObject content;
        content["轨迹输出"] = detailData.value("trackOutput", "停止输出").toString();
        step.optContent = content;

        result.append(step);
    }

    // ===== 仿真误差参数 (中高轨和地月空间场景) =====
    if (isMediumOrbit || isLunarSpace) {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        step.optName = "仿真误差参数";
        step.optType = "信号源控制";

        QJsonObject content;
        content["误差类型"] = detailData.value("simulationErrorType", "大气误差").toString();
        step.optContent = content;

        result.append(step);
    }

    // ===== 目标轨迹参数 (中高轨和地月空间场景) =====
    if (isMediumOrbit || isLunarSpace) {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        step.optName = "目标轨迹参数";
        step.optType = "信号源控制";

        auto round2 = [](double v) -> double { return qRound(v * 100.0) / 100.0; };

        QJsonObject content;
        content["轨迹类型"] = detailData.value("trackType", "静态轨迹").toString();
        content["动态轨迹类型"] = detailData.value("dynamicTrackType", "初始状态").toString();
        content["初始状态参数"] = QJsonObject{
            {"经度", round2(detailData.value("initialLon", 116.4074).toDouble())},
            {"纬度", round2(detailData.value("initialLat", 39.9042).toDouble())},
            {"高程", round2(detailData.value("initialAlt", 50.0).toDouble())},
            {"初始速度", round2(detailData.value("initialSpeed", 0.0).toDouble())},
            {"初始方位角", round2(detailData.value("initialAzimuth", 0.0).toDouble())}
        };
        content["匀速直线运动"] = QJsonObject{
            {"运动时长", round2(detailData.value("uniformDuration", 0.0).toDouble())}
        };
        content["匀加速直线运动"] = QJsonObject{
            {"运动时长", round2(detailData.value("accelDuration", 0.0).toDouble())},
            {"匀加速末速度", round2(detailData.value("accelEndSpeed", 0.0).toDouble())}
        };
        content["拐弯参数"] = QJsonObject{
            {"方向改变量", round2(detailData.value("turnDirectionChange", 0.0).toDouble())},
            {"转弯角速度", round2(detailData.value("turnAngularSpeed", 0.0).toDouble())}
        };
        content["俯冲参数"] = QJsonObject{
            {"俯仰角", round2(detailData.value("divePitchAngle", 0.0).toDouble())},
            {"高度改变量", round2(detailData.value("diveAltChange", 0.0).toDouble())}
        };
        content["制动参数"] = QJsonObject{
            {"制动时长", round2(detailData.value("brakeDuration", 0.0).toDouble())}
        };
        content["静态轨迹参数"] = QJsonObject{
            {"经度", round2(detailData.value("longitude", 116.38456).toDouble())},
            {"纬度", round2(detailData.value("latitude", 39.69394).toDouble())},
            {"高程", round2(detailData.value("altitude", 50.0).toDouble())}
        };
        step.optContent = content;

        result.append(step);
    }

    // ===== 安全认证场景特有配置 =====
    if (isSecurityAuth) {
        // 欺骗参数
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "欺骗参数";
            step.optType = "信号源控制";

            QJsonObject content;
            content["欺骗信号"] = detailData.value("deceptionSignal", "BDS_B1I").toString();
            content["功率模式"] = detailData.value("powerMode", "自动模式").toString();
            content["初始功率"] = detailData.value("initPower", -130.0).toDouble();
            content["星历模式"] = detailData.value("ephemerisMode", "对天接收").toString();
            content["欺骗模式"] = detailData.value("deceptionMode", "轨迹欺骗").toString();
            step.optContent = content;

            result.append(step);
        }

        // 轨迹配置
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "轨迹配置";
            step.optType = "信号源控制";

            auto round2 = [](double v) -> double { return qRound(v * 100.0) / 100.0; };

            QJsonObject content;
            content["欺骗轨迹类型"] = detailData.value("trackType", "静态轨迹").toString();
            content["动态轨迹类型"] = detailData.value("dynamicTrackType", "初始状态").toString();
            content["初始状态参数"] = QJsonObject{
                {"经度", round2(detailData.value("initialLon", 116.4074).toDouble())},
                {"纬度", round2(detailData.value("initialLat", 39.9042).toDouble())},
                {"高程", round2(detailData.value("initialAlt", 50.0).toDouble())},
                {"初始速度", round2(detailData.value("initialSpeed", 0.0).toDouble())},
                {"初始方位角", round2(detailData.value("initialAzimuth", 0.0).toDouble())}
            };
            content["匀速直线运动"] = QJsonObject{
                {"运动时长", round2(detailData.value("uniformDuration", 0.0).toDouble())}
            };
            content["匀加速直线运动"] = QJsonObject{
                {"运动时长", round2(detailData.value("accelDuration", 0.0).toDouble())},
                {"匀加速末速度", round2(detailData.value("accelEndSpeed", 0.0).toDouble())}
            };
            content["拐弯参数"] = QJsonObject{
                {"方向改变量", round2(detailData.value("turnDirectionChange", 0.0).toDouble())},
                {"转弯角速度", round2(detailData.value("turnAngularSpeed", 0.0).toDouble())}
            };
            content["俯冲参数"] = QJsonObject{
                {"俯仰角", round2(detailData.value("divePitchAngle", 0.0).toDouble())},
                {"高度改变量", round2(detailData.value("diveAltChange", 0.0).toDouble())}
            };
            content["制动参数"] = QJsonObject{
                {"制动时长", round2(detailData.value("brakeDuration", 0.0).toDouble())}
            };
            content["静态轨迹参数"] = QJsonObject{
                {"经度", round2(detailData.value("longitude", 116.38456).toDouble())},
                {"纬度", round2(detailData.value("latitude", 39.69394).toDouble())},
                {"高程", round2(detailData.value("altitude", 50.0).toDouble())}
            };
            step.optContent = content;

            result.append(step);
        }
    }

    // ===== 抗干扰场景特有配置 =====
    if (isAntiJamming) {
        // 连续波设置
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "连续波设置";
            step.optType = "信号源控制";
            QJsonObject content;
            content["配置"] = jammingChannelsToDb(detailData.value("continuousWave").toList(), "连续波设置");
            step.optContent = content;
            result.append(step);
        }

        // 脉冲信号设置
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "脉冲信号设置";
            step.optType = "信号源控制";
            QJsonObject content;
            content["配置"] = jammingChannelsToDb(detailData.value("pulseSignal").toList(), "脉冲信号设置");
            step.optContent = content;
            result.append(step);
        }

        // 白噪声设置
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "白噪声设置";
            step.optType = "信号源控制";
            QJsonObject content;
            content["配置"] = jammingChannelsToDb(detailData.value("whiteNoise").toList(), "白噪声设置");
            step.optContent = content;
            result.append(step);
        }

        // 调幅信号设置
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "调幅信号设置";
            step.optType = "信号源控制";
            QJsonObject content;
            content["配置"] = jammingChannelsToDb(detailData.value("amSignal").toList(), "调幅信号设置");
            step.optContent = content;
            result.append(step);
        }

        // 扩频信号设置
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "扩频信号设置";
            step.optType = "信号源控制";
            QJsonObject content;
            content["配置"] = jammingChannelsToDb(detailData.value("spreadSpectrum").toList(), "扩频信号设置");
            step.optContent = content;
            result.append(step);
        }

        // 扫频信号设置
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "扫频信号设置";
            step.optType = "信号源控制";
            QJsonObject content;
            content["配置"] = jammingChannelsToDb(detailData.value("sweepFreq").toList(), "扫频信号设置");
            step.optContent = content;
            result.append(step);
        }

        // 调频信号设置
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "调频信号设置";
            step.optType = "信号源控制";
            QJsonObject content;
            content["配置"] = jammingChannelsToDb(detailData.value("fmSignal").toList(), "调频信号设置");
            step.optContent = content;
            result.append(step);
        }

        // 诱骗位置
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "诱骗位置";
            step.optType = "信号源控制";

            QJsonObject content;
            content["经度"] = detailData.value("deceptionLon", 0.0).toDouble();
            content["纬度"] = detailData.value("deceptionLat", 0.0).toDouble();
            content["高程"] = detailData.value("deceptionAlt", 0.0).toDouble();
            step.optContent = content;

            result.append(step);
        }

        // 诱骗参数
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "诱骗参数";
            step.optType = "信号源控制";

            QJsonObject content;
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
                    content[freqNames[i]] = freqObj;
                }
            }
            step.optContent = content;

            result.append(step);
        }

        // 诱骗重启
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "诱骗重启";
            step.optType = "信号源控制";

            QJsonObject content;
            content["重启参数"] = detailData.value("deceptionReboot", "REBOOT").toString();
            step.optContent = content;

            result.append(step);
        }

        // 干扰参数
        {
            StepBuildResult step;
            step.optIndex = currentIndex++;
            step.optName = "干扰参数";
            step.optType = "信号源控制";

            QJsonObject content;
            content["干扰机数量"] = detailData.value("jammerCount", "1").toString();
            content["介入时间"] = detailData.value("interventionTime", "").toString();
            content["持续时间"] = detailData.value("jammingDuration", "").toString();
            content["干扰方向"] = detailData.value("jammingDirection", "").toString();
            content["干扰方式"] = detailData.value("jammingMode", "固定模式").toString();
            content["干扰功率"] = detailData.value("jammingPower", "").toString();
            step.optContent = content;

            result.append(step);
        }
    }

    // ===== 复位 =====
    {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        step.optName = "复位";
        step.optType = "待测设备";

        QJsonObject content;
        content["复位指令"] = "$CCRIS,*64<CR><LF>";
        step.optContent = content;

        result.append(step);
    }

    // ===== RMO =====
    {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        step.optName = "RMO";
        step.optType = "待测设备";

        QString rmoOpenMode = detailData.value("rmoOpenMode", "").toString();
        if (rmoOpenMode == "打开指令" || rmoOpenMode == "打开全部" || rmoOpenMode == "OPEN") {
            rmoOpenMode = "OPEN";
        } else {
            rmoOpenMode = "CLOSE";
        }

        QJsonObject content;
        content["打开模式"] = rmoOpenMode;
        content["目标语句"] = detailData.value("rmoTargetStatement", "GGA").toString();
        content["目标语句输出频度"] = QString::number(detailData.value("rmoOutputFreq", 1).toInt());
        step.optContent = content;

        result.append(step);
    }

    // ===== 评估类型（含阈值配置） =====
    {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        step.optName = "评估类型";
        step.optType = "流程逻辑控制";

        QJsonObject content;

        // 评估类型：字符串形式，只存一个
        QString testType = detailData.value("testType").toString();
        if (testType == "定位精度" || testType == "干扰定位") {
            content["评估类型"] = "定位精度";
        } else if (testType == "测速精度" || testType == "干扰测速") {
            content["评估类型"] = "测速精度";
        } else if (testType == "首次定位时间" || testType == "干扰首次定位时间") {
            content["评估类型"] = "首次定位时间";
        } else if (testType == "抗干扰性能测试") {
            content["评估类型"] = "抗干扰性能";
        } else if (testType == "安全认证性能测试") {
            content["评估类型"] = "安全认证性能";
        } else if (testType == "地月空间定位能力测试") {
            content["评估类型"] = "地月空间定位能力";
        } else {
            content["评估类型"] = "定位精度"; // 默认
        }

        // 首次定位时间：只存评估类型和时间阈值，不存评估方式和方向阈值
        bool isTtff = (testType == "首次定位时间" || testType == "干扰首次定位时间");
        if (isTtff) {
            content["时间阈值"] = detailData.value("timeThreshold", 30.0).toDouble();
        } else {
            // 其他类型：存评估方式 + 方向阈值
            QString evalMethod = detailData.value("evalMethod", "RMS统计").toString();
            QJsonArray evalMethods;
            evalMethods.append(evalMethod);
            content["评估方式"] = evalMethods;

            QJsonObject thresholdConfig;
            thresholdConfig["东方向"] = detailData.value("thEast", 10.0).toDouble();
            thresholdConfig["北方向"] = detailData.value("thNorth", 10.0).toDouble();
            thresholdConfig["垂直方向"] = detailData.value("thUp", 10.0).toDouble();
            content["阈值配置"] = thresholdConfig;
        }

        step.optContent = content;

        result.append(step);
    }

    // ===== 数据接收 =====
    {
        StepBuildResult step;
        step.optIndex = currentIndex++;
        step.optName = "数据接收";
        step.optType = "流程逻辑控制";

        QJsonObject content;
        content["数据采集"] = detailData.value("dataCollectionCount", 100).toInt();
        step.optContent = content;

        result.append(step);
    }

    return result;
}
