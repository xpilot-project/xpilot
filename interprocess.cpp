#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QTimer>
#include <QQuickWindow>

#include "interprocess.h"
#include "base64.hpp"
#include "protobuf/Envelope.pb.h"

InterProcess::InterProcess(QObject* parent) : QObject(parent)
{
    process = new QProcess(this);
    process->setProgram("XplaneBridge/XplaneBridge.exe");
    process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    process->start();

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &InterProcess::restartProcess);
    timer->start(1000);

    QObject::connect(process, &QProcess::readyReadStandardOutput, this, &InterProcess::Tick);
}

InterProcess::~InterProcess()
{
    qDebug("Terminating IPC process");
    process->kill();
}

void InterProcess::sendEnvelope(const xpilot::Envelope& envelope)
{
    std::string data;
    envelope.SerializeToString(&data);

    process->write(base64::base64_encode(data).c_str());
    process->write("\n");
}

void InterProcess::Tick()
{
    const QByteArray data = QByteArray::fromBase64(process->readAllStandardOutput());

    xpilot::Envelope envelope;
    envelope.ParseFromArray(data, data.length());

    if(envelope.has_simulator_connection_state())
    {
        emit simulatorConnected(envelope.simulator_connection_state().connected());
    }

    if(envelope.has_radio_stack())
    {
        RadioStack stack{};
        stack.avionicsPowerOn = envelope.radio_stack().avionics_power_on();
        stack.com1Frequency = envelope.radio_stack().com1_frequency();
        stack.com1ReceiveEnabled = envelope.radio_stack().com1_receive_enabled();
        stack.com2Frequency = envelope.radio_stack().com2_frequency();
        stack.com2ReceiveEnabled = envelope.radio_stack().com2_receive_enabled();
        stack.transmitComSelection = envelope.radio_stack().transmit_com_selection();
        emit radioStackReceived(stack);
    }

    if(envelope.has_app_config_dto())
    {
        AppConfig config{};
        config.vatsimId = QString(envelope.app_config_dto().vatsim_id().c_str());
        config.vatsimPassword = QString(envelope.app_config_dto().vatsim_password().c_str());
        config.homeAirport = QString(envelope.app_config_dto().home_airport().c_str());
        config.name = QString(envelope.app_config_dto().name().c_str());
        emit appConfigReceived(config);
    }
}

void InterProcess::onHandleSetTransponderCode(int code)
{
    xpilot::Envelope envelope;
    xpilot::TransponderCode * msg = new xpilot::TransponderCode();
    envelope.set_allocated_transponder_code(msg);
    msg->set_code(code);
    sendEnvelope(envelope);
}

void InterProcess::onHandleSetRadioStack(int radio, int frequency)
{
    xpilot::Envelope envelope;
    xpilot::SetRadioStack * msg = new xpilot::SetRadioStack();
    envelope.set_allocated_set_radiostack(msg);
    msg->set_radio(radio);
    msg->set_frequency(frequency);
    sendEnvelope(envelope);
}

void InterProcess::onHandleTransponderModeC(bool active)
{
    xpilot::Envelope envelope;
    xpilot::TransponderMode * msg = new xpilot::TransponderMode();
    envelope.set_allocated_transponder_mode(msg);
    msg->set_mode_c(active);
    sendEnvelope(envelope);
}

void InterProcess::onHandleTransponderIdent()
{
    xpilot::Envelope envelope;
    xpilot::TransponderIdent * msg = new xpilot::TransponderIdent();
    envelope.set_allocated_transponder_ident(msg);
    msg->set_ident(true);
    sendEnvelope(envelope);
}

void InterProcess::onHandleRequestConfig()
{
    xpilot::Envelope envelope;
    xpilot::RequestConfig * msg = new xpilot::RequestConfig();
    envelope.set_allocated_request_config(msg);
    sendEnvelope(envelope);
}

void InterProcess::onHandleUpdateConfig(QVariant cfg)
{
    AppConfig config = cfg.value<AppConfig>();
    xpilot::Envelope envelope;
    xpilot::AppConfigDto * msg = new xpilot::AppConfigDto();
    envelope.set_allocated_app_config_dto(msg);
    msg->set_vatsim_id(config.vatsimId.toStdString().c_str());
    msg->set_vatsim_password(config.vatsimPassword.toStdString().c_str());
    msg->set_name(config.name.toStdString().c_str());
    msg->set_home_airport(config.homeAirport.toStdString().c_str());
    sendEnvelope(envelope);
}

void InterProcess::restartProcess()
{
    if(process->state() == QProcess::NotRunning)
    {
        delete process;

        process = new QProcess(this);
        process->setProgram("XplaneBridge/XplaneBridge.exe");
        process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
        process->start();
    }
}
