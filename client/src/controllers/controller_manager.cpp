#include <QDateTime>
#include <QJsonArray>

#include "controller_manager.h"
#include "src/common/frequency_utils.h"

namespace xpilot
{
    ControllerManager::ControllerManager(NetworkManager& networkManager, XplaneAdapter& xplaneAdapter, QObject *parent) :
        QObject(parent),
        m_networkManager(networkManager),
        m_xplaneAdapter(xplaneAdapter)
    {
        connect(&m_networkManager, &NetworkManager::controllerUpdateReceived, this, &ControllerManager::OnControllerUpdateReceived);
        connect(&m_networkManager, &NetworkManager::isValidAtcReceived, this, &ControllerManager::IsValidATCReceived);
        connect(&m_networkManager, &NetworkManager::realNameReceived, this, &ControllerManager::OnRealNameReceived);
        connect(&m_networkManager, &NetworkManager::controllerDeleted, this, &ControllerManager::OnControllerDeleted);
        connect(&m_networkManager, &NetworkManager::networkConnected, this, [&] {
            m_controllers.clear();
            m_nearbyAtcTimer.start(1000);
        });
        connect(&m_networkManager, &NetworkManager::networkDisconnected, this, [&] {
            m_controllers.clear();
            m_nearbyAtcTimer.stop();
        });
        connect(&m_nearbyAtcTimer, &QTimer::timeout, this, [&]{

            for(auto it = m_controllers.begin(), next_it = it; it != m_controllers.end(); it = next_it)
            {
                ++next_it;
                if((QDateTime::currentSecsSinceEpoch() - it->LastUpdate) > 60) {
                    m_controllers.erase(it);
                }
            }

            QJsonObject reply;
            reply.insert("type", "NearbyAtc");

            QJsonArray dataArray;
            for(auto &atc : m_controllers)
            {
                if(atc.IsValid)
                {
                    QJsonObject data;
                    data.insert("callsign", atc.Callsign);
                    data.insert("xplane_frequency", (qint32)atc.Frequency);
                    data.insert("frequency", QString::number(atc.Frequency / 1000.0, 'f', 3));
                    data.insert("real_name", atc.RealName);
                    dataArray.push_back(data);
                }
            }

            reply.insert("data", dataArray);
            QJsonDocument doc(reply);
            m_xplaneAdapter.sendSocketMessage(QString(doc.toJson(QJsonDocument::Compact)));
        });
    }

    void ControllerManager::OnControllerUpdateReceived(QString from, uint frequency, double lat, double lon)
    {
        auto itr = std::find_if(m_controllers.begin(), m_controllers.end(), [=](const Controller& n){
            return n.Callsign == from;
        });

        if(itr != m_controllers.end())
        {
            bool hasFrequencyChanged = (uint)frequency != itr->Frequency;
            bool hasLocationChanged = lat != itr->Latitude || lon != itr->Longitude;
            itr->Frequency = frequency;
            itr->FrequencyHz = frequency * 1000;
            itr->Latitude = lat;
            itr->Longitude = lon;
            itr->LastUpdate = QDateTime::currentSecsSinceEpoch();

            ValidateController(*itr);

            if(hasFrequencyChanged || hasLocationChanged)
                RefreshController(*itr);
        }
        else
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
    }

    void ControllerManager::IsValidATCReceived(QString callsign)
    {
        auto itr = std::find_if(m_controllers.begin(), m_controllers.end(), [=](const Controller& n){
            return n.Callsign == callsign;
        });

        if(itr != m_controllers.end())
            itr->IsValidATC = true;

        ValidateController(*itr);
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
                RefreshController(*itr);
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
        bool isValid = controller.IsValidATC && (controller.Frequency >= 118000 && controller.Frequency <= 136975);
        controller.IsValid = isValid;
    }
}
