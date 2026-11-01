// SceneDetailMapper 的通道序列化：channels*/jammingChannels*/isJammingScene
#include "SceneDetailMapper.h"
#include "utils/SceneLogger.h"

#include <QVariantList>
#include <QDebug>

bool SceneDetailMapper::isJammingScene(const QVector<SceneStep> &steps)
{
    for (const auto &step : steps) {
        if (step.optName == "压制干扰源配置" || step.optName == "诱骗干扰源配置") {
            return true;
        }
    }
    return false;
}

QJsonArray SceneDetailMapper::channelsToDb(const QVariantList &channelList)
{
    QJsonArray channels;
    for (const QVariant &ch : channelList) {
        QVariantMap chMap = ch.toMap();
        QJsonObject chObj;
        chObj["功率"] = chMap.value("powerValue", 0).toDouble();
        chObj["参数类型"] = chMap.value("paramType", "通道开关").toString();
        chObj["开关状态"] = chMap.value("switchType", "关闭").toString();
        channels.append(chObj);
    }
    return channels;
}

QVariantList SceneDetailMapper::channelsFromDb(const QJsonArray &channelArray)
{
    QVariantList channels;
    for (const QJsonValue &chVal : channelArray) {
        QJsonObject chObj = chVal.toObject();
        QVariantMap chMap;
        chMap["powerValue"] = chObj.value("功率").toDouble();
        chMap["paramType"] = chObj.value("参数类型").toString();
        chMap["switchType"] = chObj.value("开关状态").toString();
        channels.append(chMap);
    }
    return channels;
}

QJsonArray SceneDetailMapper::channelsToDb_MediumOrbit(const QVariantList &channelList)
{
    QJsonArray channels;
    for (const QVariant &ch : channelList) {
        QVariantMap chMap = ch.toMap();
        QJsonObject chObj;
        chObj["调制类型"] = chMap.value("modulation", "0-关闭").toString();
        chObj["I 功率"] = chMap.value("powerI", -127.6).toDouble();
        chObj["Q 功率"] = chMap.value("powerQ", -127.6).toDouble();
        chObj["通道开关"] = chMap.value("chSwitch", chMap.value("channelSwitch", "0-关闭")).toString();
        chObj["频点开关"] = chMap.value("freqSwitch", "0-关闭").toString();
        channels.append(chObj);
    }
    return channels;
}

QVariantList SceneDetailMapper::channelsFromDb_MediumOrbit(const QJsonArray &channelArray)
{
    QVariantList channels;
    for (const QJsonValue &chVal : channelArray) {
        QJsonObject chObj = chVal.toObject();
        QVariantMap chMap;
        chMap["modulation"] = chObj.value("调制类型").toString();
        chMap["powerI"] = chObj.value("I 功率").toDouble(-127.6);
        chMap["powerQ"] = chObj.value("Q 功率").toDouble(-127.6);
        chMap["chSwitch"] = chObj.value("通道开关").toString();
        chMap["channelSwitch"] = chObj.value("通道开关").toString();
        chMap["freqSwitch"] = chObj.value("频点开关").toString();
        channels.append(chMap);
    }
    return channels;
}

QJsonObject SceneDetailMapper::jammingChannelsToDb(const QVariantList &channelList, const QString &signalTypeName)
{
    // 前端英文字段名 → 数据库中文字段名映射
    static const QMap<QString, QString> enToCn = {
        {"paramType",           "参数类型"},
        {"signalPower",         "信号功率"},
        {"centerFreq",          "中心频率"},
        {"period",              "周期"},
        {"pulseWidth",          "脉宽"},
        {"signalBandwidth",     "信号带宽"},
        {"signalType",          "信号类型"},
        {"modulationFreq",      "调制频率"},
        {"modulationDegree",    "调制度"},
        {"startFreq",           "起始频率"},
        {"endFreq",             "终止频率"},
        {"freqStep",            "频率步进"},
        {"dwellTime",           "驻留时间"},
        {"videoNoiseBandwidth", "视频噪声带宽"},
        {"noiseBandwidth",      "噪声带宽"}
    };

    // 每种信号类型必须包含的默认字段
    static const QMap<QString, QMap<QString, QVariant>> typeDefaults = {
        {"连续波设置", {{"中心频率", -127.6}, {"信号功率", -127.6}}},
        {"脉冲信号设置", {{"中心频率", -127.6}, {"周期", -127.6}, {"脉宽", -127.6}, {"信号功率", -127.6}}},
        {"白噪声设置", {{"中心频率", -127.6}, {"信号功率", -127.6}, {"信号带宽", -127.6}}},
        {"调幅信号设置", {{"中心频率", -127.6}, {"信号类型", "正弦波"}, {"调制频率", -127.6}, {"调制度", -127.6}, {"信号功率", -127.6}}},
        {"扩频信号设置", {{"中心频率", -127.6}, {"信号功率", -127.6}, {"信号带宽", -127.6}}},
        {"扫频信号设置", {{"起始频率", -127.6}, {"终止频率", -127.6}, {"频率步进", -127.6}, {"驻留时间", -127.6}, {"信号功率", -127.6}}},
        {"调频信号设置", {{"中心频率", -127.6}, {"视频噪声带宽", "1K"}, {"噪声带宽", -127.6}, {"信号功率", -127.6}}}
    };

    QMap<QString, QVariant> defaults = typeDefaults.value(signalTypeName);

    // 返回对象格式 {"0": {...}, "1": {...}, "2": {...}}
    QJsonObject result;
    for (int i = 0; i < 3; ++i) {
        QVariantMap chMap;
        if (i < channelList.size()) {
            chMap = channelList[i].toMap();
        }

        QJsonObject chObj;
        chObj["参数类型"] = chMap.value("paramType", "停止输出").toString();

        // 填入默认字段
        for (auto it = defaults.constBegin(); it != defaults.constEnd(); ++it) {
            QVariant defVal = it.value();
            if (defVal.typeId() == QMetaType::Double) {
                chObj[it.key()] = defVal.toDouble();
            } else {
                chObj[it.key()] = defVal.toString();
            }
        }

        // 用前端值覆盖
        for (const QString &key : chMap.keys()) {
            if (key == "paramType" || key == "channelIndex") continue;
            QString dbKey = enToCn.value(key, key);
            QVariant value = chMap.value(key);
            if (value.typeId() == QMetaType::Double) {
                chObj[dbKey] = value.toDouble();
            } else if (value.typeId() == QMetaType::Int) {
                chObj[dbKey] = value.toInt();
            } else {
                chObj[dbKey] = value.toString();
            }
        }

        result[QString::number(i)] = chObj;
    }
    return result;
}

QVariantList SceneDetailMapper::jammingChannelsFromDb(const QJsonObject &channelObj)
{
    static const QMap<QString, QString> cnToEn = {
        {"参数类型",     "paramType"},
        {"信号功率",     "signalPower"},
        {"中心频率",     "centerFreq"},
        {"周期",         "period"},
        {"脉宽",         "pulseWidth"},
        {"信号带宽",     "signalBandwidth"},
        {"信号类型",     "signalType"},
        {"调制频率",     "modulationFreq"},
        {"调制度",       "modulationDegree"},
        {"起始频率",     "startFreq"},
        {"终止频率",     "endFreq"},
        {"频率步进",     "freqStep"},
        {"驻留时间",     "dwellTime"},
        {"视频噪声带宽", "videoNoiseBandwidth"},
        {"噪声带宽",     "noiseBandwidth"}
    };

    QVariantList channels;
    for (int i = 0; i < 3; ++i) {
        QString key = QString::number(i);
        QVariantMap chMap;

        if (channelObj.contains(key)) {
            QJsonObject chObj = channelObj[key].toObject();
            for (const QString &cnKey : chObj.keys()) {
                QString enKey = cnToEn.value(cnKey, cnKey);
                QJsonValue value = chObj.value(cnKey);
                if (value.isDouble()) {
                    chMap[enKey] = value.toDouble();
                } else if (value.isString()) {
                    chMap[enKey] = value.toString();
                } else {
                    chMap[enKey] = value.toVariant();
                }
            }
        }

        if (!chMap.contains("paramType")) {
            chMap["paramType"] = "停止输出";
        }
        chMap["channelIndex"] = i;

        channels.append(chMap);
    }
    return channels;
}