#include <QDateTime>

#include "controller_manager.h"
#include "src/common/frequency_utils.h"

namespace xpilot
{
    ControllerManager::ControllerManager(NetworkManager& networkManager, QObject *parent) :
        QObject(parent),
        m_networkManager(networkManager)
    {
        connect(&m_networkManager, &NetworkManager::controllerUpdateReceived, this, &ControllerManager::OnControllerUpdateReceived);
    }

    void ControllerManager::OnControllerUpdateReceived(QString from, uint frequency, double lat, double lon)
    {
        auto controllerIt = std::find_if(m_controllers.begin(), m_controllers.end(), [=](const Controller& n){
            return n.Callsign == from;
        });

        if(controllerIt == m_controllers.end()) {
            Controller controller {};
            controller.Callsign = from.toUpper();
            controller.Frequency = frequency;
            controller.NormalizedFrequency = Normalize25KhzFsdFrequency(frequency);
            controller.RealName = "Unknown";
            controller.Latitude = lat;
            controller.Longitude = lon;
            controller.LastUpdate = QDateTime::currentSecsSinceEpoch();
            m_controllers.push_back(controller);

            m_networkManager.RequestRealName(from);
            m_networkManager.RequestIsValidATC(from);
            m_networkManager.RequestCapabilities(from);
        }
        else
        {
            bool isValid = controllerIt->IsValid;
            bool hasFrequencyChanged = (uint)frequency != controllerIt->Frequency;
            bool hasLocationChanged = lat != controllerIt->Latitude || lon != controllerIt->Longitude;
            controllerIt->Frequency = frequency;
            controllerIt->Latitude = lat;
            controllerIt->Longitude = lon;
            controllerIt->LastUpdate = QDateTime::currentSecsSinceEpoch();
            ValidateController(*controllerIt);
        }
    }

    void ControllerManager::ValidateController(Controller &controller)
    {
//        bool isValid = controller.IsValid;
//        controller.IsValid = controller.IsValidATC && (controller.Frequency >= 18000 && controller.Frequency <= 36975);
//        if(isValid && !controller.IsValid)
//        {
//            emit controllerDeleted(controller);
//            m_controllers.removeAll(controller);
//        }
//        else if(!isValid && controller.IsValid)
//        {
//            emit controllerAdded(controller);
//        }
    }
}
