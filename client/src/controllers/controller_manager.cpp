#include <QDateTime>
#include <QJsonArray>

#include "controller_manager.h"
#include "src/common/frequency_utils.h"

namespace xpilot
{
    ControllerManager::ControllerManager(NetworkManager& networkManager, XplaneAdapter& xplaneAdapter, QObject *parent) :
        QObject(parent),
        m_networkManager(networkManager),
        m_xplaneAdapter(xplaneAdapter),
        m_nearbyAtcTimer(new QTimer(this))
    {
        connect(&m_networkManager, &NetworkManager::controllerUpdateReceived, this, &ControllerManager::OnControllerUpdateReceived);
        connect(&m_networkManager, &NetworkManager::isValidAtcReceived, this, &ControllerManager::IsValidATCReceived);
        connect(&m_networkManager, &NetworkManager::realNameReceived, this, &ControllerManager::OnRealNameReceived);
        connect(&m_networkManager, &NetworkManager::controllerDeleted, this, &ControllerManager::OnControllerDeleted);
        connect(&m_networkManager, &NetworkManager::networkConnected, this, [&] {
            m_controllers.clear();
            m_nearbyAtcTimer->start(5000);
        });
        connect(&m_networkManager, &NetworkManager::networkDisconnected, this, [&] {
            m_controllers.clear();
            m_nearbyAtcTimer->stop();
        });
        connect(m_nearbyAtcTimer, &QTimer::timeout, this, [&]{
            QJsonObject reply;
            reply.insert("type", "NearbyAtc");

            QJsonArray data_array;
            for(const auto &atc : std::as_const(m_controllers))
            {
                QJsonObject data;
                data.insert("callsign", atc.Callsign);
                data.insert("xplane_frequency", (qint32)atc.Frequency);
                data.insert("frequency", QString::number(atc.Frequency / 1000.0));
                data.insert("real_name", atc.RealName);
                data_array.push_back(data);
            }

            reply.insert("data", data_array);
            QJsonDocument doc(reply);
            m_xplaneAdapter.sendSocketMessage(QString(doc.toJson(QJsonDocument::Compact)));
        });
    }

    void ControllerManager::OnControllerUpdateReceived(QString from, uint frequency, double lat, double lon)
    {
        auto itr = std::find_if(m_controllers.begin(), m_controllers.end(), [=](const Controller& n){
            return n.Callsign == from;
        });

        if(itr == m_controllers.end())
        {
            Controller controller {};
            controller.Callsign = from.toUpper();
            controller.Frequency = frequency;
            controller.FrequencyHz = frequency * 1000;
            controller.NormalizedFrequency = Normalize25KhzFsdFrequency(frequency);
            controller.RealName = "Unknown";
            controller.Latitude = lat;
            controller.Longitude = lon;
            controller.LastUpdate = QDateTime::currentSecsSinceEpoch();
            m_controllers.push_back(controller);

            m_networkManager.requestRealName(from);
            m_networkManager.RequestIsValidATC(from);
            m_networkManager.RequestCapabilities(from);
        }
        else
        {
            bool isValid = itr->IsValid;
            bool hasFrequencyChanged = (uint)frequency != itr->Frequency;
            bool hasLocationChanged = lat != itr->Latitude || lon != itr->Longitude;
            itr->Frequency = frequency;
            itr->FrequencyHz = frequency * 1000;
            itr->Latitude = lat;
            itr->Longitude = lon;
            itr->LastUpdate = QDateTime::currentSecsSinceEpoch();
            ValidateController(*itr);
            if(itr != m_controllers.end())
            {
                if(isValid && itr->IsValid)
                {
                    if(hasFrequencyChanged || hasLocationChanged)
                    {
                        RefreshController(*itr);
                    }
                }
            }
        }
    }

    void ControllerManager::IsValidATCReceived(QString callsign)
    {
        auto itr = std::find_if(m_controllers.begin(), m_controllers.end(), [=](const Controller& n){
            return n.Callsign == callsign;
        });
        if(itr != m_controllers.end())
        {
            itr->IsValidATC = true;
            ValidateController(*itr);
        }
    }

    void ControllerManager::OnRealNameReceived(QString callsign, QString realName)
    {
        auto itr = std::find_if(m_controllers.begin(), m_controllers.end(), [=](const Controller& n){
            return n.Callsign == callsign;
        });
        if(itr != m_controllers.end())
        {
            itr->RealName = realName;
            if(itr->IsValid)
            {
                RefreshController(*itr);
            }
        }
    }

    void ControllerManager::RefreshController(Controller controller)
    {
        emit controllerDeleted(controller);
        emit controllerAdded(controller);
    }

    void ControllerManager::OnControllerDeleted(QString callsign)
    {
        auto itr = std::find_if(m_controllers.begin(), m_controllers.end(), [=](const Controller& n){
            return n.Callsign == callsign;
        });
        if(itr != m_controllers.end())
        {
            emit controllerDeleted(*itr);
            m_controllers.removeAll(*itr);
        }
    }

    void ControllerManager::ValidateController(Controller &controller)
    {
        bool isValid = controller.IsValid;
        controller.IsValid = controller.IsValidATC && (controller.Frequency >= 118000 && controller.Frequency <= 136975);
        if(isValid && !controller.IsValid)
        {
            emit controllerDeleted(controller);
            m_controllers.removeAll(controller);
        }
        else if(!isValid && controller.IsValid)
        {
            emit controllerAdded(controller);
        }
    }
}
