/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2023 Justin Shannon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
*/

#include <QDateTime>
#include <QJsonArray>

#include "controller_manager.h"

namespace xpilot
{
    ControllerManager::ControllerManager(QObject *parent) :
        QObject(parent),
        m_networkManager(*QInjection::Pointer<NetworkManager>().data()),
        m_xplaneAdapter(*QInjection::Pointer<XplaneAdapter>().data())
    {
        connect(&m_xplaneAdapter, &XplaneAdapter::radioStackStateChanged, this, &ControllerManager::OnRadioStackStateChanged);
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
            QList<Controller> temp;
            for(auto &controller : m_controllers) {
                if(controller.IsDeletePending || (QDateTime::currentSecsSinceEpoch() - controller.LastUpdateReceived > 60)) {
                    temp.append(controller);
                }
            }

            for(auto &controller : temp) {
                emit controllerDeleted(controller);
                m_controllers.removeAll(controller);
            }

            m_xplaneAdapter.UpdateControllers(m_controllers);
            UpdateStationCallsigns();
        });
    }

    void ControllerManager::OnControllerUpdateReceived(QString from, uint frequency, double lat, double lon)
    {
        QList<Controller>::iterator itr = std::find_if(m_controllers.begin(), m_controllers.end(), [&](Controller o){
            return o.Callsign == from.toUpper();
        });

        if(itr != m_controllers.end()) {
            bool hasFrequencyChanged = (uint)frequency != itr->Frequency;
            bool hasLocationChanged = lat != itr->Latitude || lon != itr->Longitude;

            itr->Frequency = frequency;
            itr->FrequencyHz = frequency * 1000;
            itr->Latitude = lat;
            itr->Longitude = lon;
            itr->LastUpdateReceived = QDateTime::currentSecsSinceEpoch();

            if(hasFrequencyChanged || hasLocationChanged) {
                if(!itr->IsValid()) {
                    itr->IsDeletePending = true;
                }
                RefreshController(*itr);
            }
        }
        else {
            Controller controller {};
            controller.Callsign = from.toUpper();
            controller.Frequency = frequency;
            controller.FrequencyHz = frequency * 1000;
            controller.NormalizedFrequency = Normalize25KhzFsdFrequency(frequency);
            controller.RealName = "Unknown";
            controller.Latitude = lat;
            controller.Longitude = lon;
            controller.LastUpdateReceived = QDateTime::currentSecsSinceEpoch();
            m_controllers.push_back(controller);

            m_networkManager.requestRealName(from);
            m_networkManager.RequestIsValidATC(from);
            m_networkManager.RequestCapabilities(from);
        }
    }

    void ControllerManager::IsValidATCReceived(QString callsign)
    {
        QList<Controller>::iterator itr = std::find_if(m_controllers.begin(), m_controllers.end(), [&](Controller o){
            return o.Callsign == callsign.toUpper();
        });

        if(itr != m_controllers.end()) {
            itr->IsValidATC = true;
            if(itr->IsValid()) {
                RefreshController(*itr);
            }
        }
    }

    void ControllerManager::OnRealNameReceived(QString callsign, QString realName)
    {
        QList<Controller>::iterator itr = std::find_if(m_controllers.begin(), m_controllers.end(), [&](Controller o){
            return o.Callsign == callsign.toUpper();
        });

        if(itr != m_controllers.end()) {
            itr->RealName = realName;
            if(itr->IsValid()) {
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
        QList<Controller>::iterator itr = std::find_if(m_controllers.begin(), m_controllers.end(), [&](Controller o){
            return o.Callsign == callsign.toUpper();
        });

        if(itr != m_controllers.end()) {
            if(m_radioStackState.Com1Frequency == itr->Frequency) {
                m_xplaneAdapter.SetStationCallsign(1, "");
            }
            if(m_radioStackState.Com2Frequency == itr->Frequency) {
                m_xplaneAdapter.SetStationCallsign(2, "");
            }
            itr->IsDeletePending = true;
        }
    }

    void ControllerManager::OnRadioStackStateChanged(RadioStackState radioStack)
    {
        if(m_radioStackState != radioStack) {
            m_radioStackState = radioStack;
            UpdateStationCallsigns();
        }
    }

    void ControllerManager::UpdateStationCallsigns()
    {
        QList<Controller>::iterator com1 = std::find_if(m_controllers.begin(), m_controllers.end(), [&](Controller n){
            return n.Frequency == m_radioStackState.Com1Frequency;
        });

        if(com1 != m_controllers.end()) {
            m_xplaneAdapter.SetStationCallsign(1, com1->Callsign);
        }
        else {
            m_xplaneAdapter.SetStationCallsign(1, "");
        }

        QList<Controller>::iterator com2 = std::find_if(m_controllers.begin(), m_controllers.end(), [&](Controller n){
            return n.Frequency == m_radioStackState.Com2Frequency;
        });

        if(com2 != m_controllers.end()) {
            m_xplaneAdapter.SetStationCallsign(2, com2->Callsign);
        }
        else {
            m_xplaneAdapter.SetStationCallsign(2, "");
        }
    }
}
