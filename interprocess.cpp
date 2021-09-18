#include "interprocess.h"
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QQuickWindow>
#include "base64.hpp"
#include "protobuf/Envelope.pb.h"

InterProcess::InterProcess(QObject* parent) : QObject(parent)
{
    process.setProgram("XplaneBridge/XplaneBridge.exe");
    process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    process.start();

    QObject::connect(&process, &QProcess::readyReadStandardOutput, this, &InterProcess::Tick);
}

InterProcess::~InterProcess()
{
    qDebug("Terminating IPC process");
    process.kill();
}

void InterProcess::sendEnvelope(const xpilot::Envelope& envelope)
{
    std::string data;
    envelope.SerializeToString(&data);

    process.write(base64::base64_encode(data).c_str());
    process.write("\n");
}

void InterProcess::onSetTransponderCode(int code)
{
    xpilot::Envelope envelope;
    xpilot::TransponderCode * msg = new xpilot::TransponderCode();
    envelope.set_allocated_transponder_code(msg);
    msg->set_code(code);
    sendEnvelope(envelope);
}

void InterProcess::onSetRadioStack(int radio, int frequency)
{
    xpilot::Envelope envelope;
    xpilot::SetRadioStack * msg = new xpilot::SetRadioStack();
    envelope.set_allocated_set_radiostack(msg);
    msg->set_radio(radio);
    msg->set_frequency(frequency);
    sendEnvelope(envelope);
}

void InterProcess::Tick()
{
    const QByteArray data = QByteArray::fromBase64(process.readAllStandardOutput());

    xpilot::Envelope envelope;
    envelope.ParseFromArray(data, data.length());

    if(envelope.has_simulator_connection_state())
    {
        emit simulatorConnected(envelope.simulator_connection_state().connected());
    }

    //    qDebug() << jsonParts;

    //    QJsonParseError jsonError;
    //    QJsonDocument jsonDoc(QJsonDocument::fromJson(jsonParts.toUtf8(), &jsonError));

    //    if(!jsonParts.isEmpty()) {
    //        if(jsonError.error == QJsonParseError::NoError) {
    //            QJsonObject jsonObject = jsonDoc.object();

    //            if(jsonObject.contains("Topic")) {
    //                QString topic = jsonObject["Topic"].toString();

    //                if(topic == "NotificationPosted") {
    //                    emit notificationPosted(NotificationType::Info, jsonObject["Message"].toString());
    //                }
    //            }
    //        }
    //    }
}
