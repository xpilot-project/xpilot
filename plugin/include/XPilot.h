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

#ifndef XPilot_h
#define XPilot_h

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imfilebrowser.h"
#include "XPImgWindow.h"
#include "DataRefAccess.h"
#include "OwnedDataRef.h"
#include "TextMessageConsole.h"
#include "XPLMMenus.h"
#include "XPLMUtilities.h"
#include "XPLMProcessing.h"
#include "Utilities.h"
#include "Dto.h"
#include "msgpack.hpp"
#include "nng/nng.h"
#include "nng/protocol/pair1/pair.h"

#include <deque>
#include <thread>
#include <mutex>
#include <functional>
#include <map>
#include <atomic>
#include <algorithm>
#include <iostream>

namespace xpilot
{
	enum class dataRefs
	{
		DR_BULK_QUICK,
		DR_BULK_EXPENSIVE,
		DR_NUM_AIRCRAFT
	};

	class FrameRateMonitor;
	class AircraftManager;
	class NotificationPanel;
	class TextMessageConsole;
	class NearbyATCWindow;
	class SettingsWindow;

	class XPilot
	{
	public:
		XPilot();
		~XPilot();

		void Initialize();
		void Shutdown();
		void TryGetTcasControl();
		void ReleaseTcasControl();

		std::string NetworkCallsign() const { return m_networkCallsign; }
		bool IsXplaneAtisDisabled() const { return !m_xplaneAtisEnabled; }
		bool IsNetworkConnected() const { return m_networkLoginStatus; }
		bool IsRadiosPowered() const { return m_avionicsPower && (m_audioComSelection == 6 || m_audioComSelection == 7); }
		int GetTxRadio() const { return m_audioComSelection; }

		void SetPttActive(bool active);
		void SetCom1Frequency(int frequency);
		void SetCom2Frequency(int frequency);
		void SetAudioComSelection(int radio);
		void SetAudioSelection(int radio, bool on);
		void SetTransponderCode(int code);
		void RequestStationInfo(const std::string& station);
		void RequestMetar(const std::string& station);
		void AircraftDeleted(const std::string& callsign);
		void AircraftAdded(const std::string& callsign);
		void DeleteAllAircraft();

		void ForceDisconnect(std::string reason = "");
		void DisableXplaneAtis(bool disabled);

		void SendRadioMessage(const std::string& message);
		void SendPrivateMessage(const std::string& to, const std::string& message);
		void SendWallop(const std::string& message);

		void AddNotificationMessage(const std::string& message, const rgb& color = Colors::White, bool addToConsole = true);
		void AddNotificationShowPanel(const std::string& message, const rgb& color = Colors::White, bool addToConsole = true);
		void PrivateMessageReceived(const std::string& from, const std::string& message);
		void PrivateMessageSent(const std::string& to, const std::string& message);

		void ToggleSettingsWindow();
		void ToggleNearbyAtcWindow();
		void ToggleTextMessageConsole();
		void SetNotificationPanelAlwaysVisible(bool visible);
		bool GetNotificationPanelAlwaysVisible() const;

	protected:
		OwnedDataRef<int> m_pttPressed;
		OwnedDataRef<int> m_networkLoginStatus;
		OwnedDataRef<std::string> m_networkCallsign;
		OwnedDataRef<int> m_rxCom1;
		OwnedDataRef<int> m_rxCom2;
		OwnedDataRef<float> m_volumeSignalLevel;
		OwnedDataRef<int> m_aiControlled;
		OwnedDataRef<int> m_aircraftCount;
		OwnedDataRef<int> m_pluginVersion;
		OwnedDataRef<std::string> m_selcalCode;
		OwnedDataRef<int> m_selcalReceived;
		OwnedDataRef<int> m_selcalMuteOverride;
		OwnedDataRef<std::string> m_com1StationCallsign;
		OwnedDataRef<std::string> m_com2StationCallsign;
		DataRefAccess<int> m_xplaneAtisEnabled;
		DataRefAccess<float> m_frameRatePeriod;
		DataRefAccess<int> m_com1Frequency;
		DataRefAccess<int> m_com2Frequency;
		DataRefAccess<int> m_audioComSelection;
		DataRefAccess<int> m_audioSelectionCom1;
		DataRefAccess<int> m_audioSelectionCom2;
		DataRefAccess<int> m_transponderCode;
		DataRefAccess<int> m_avionicsPower;

	private:
		static float DeferredStartup(float, float, int, void* ref);
		static float MainFlightLoop(float, float, int, void* ref);
		bool InitializeXPMP();

		bool m_keepSocketAlive = false;
		nng_socket m_socket;
		std::unique_ptr<std::thread> m_socketThread;

		void SocketWorker();
		void ProcessPacket(const BaseDto& dto);

		std::mutex m_mutex;
		std::deque<std::function<void()>> m_queuedCallbacks;
		void InvokeQueuedCallbacks();
		void QueueCallback(const std::function<void()>& cb);

		XPLMDataRef m_bulkDataQuick{}, m_bulkDataExpensive{};
		static int GetBulkData(void* inRefcon, void* outData, int inStartPos, int inNumBytes);

		std::unique_ptr<FrameRateMonitor> m_frameRateMonitor;
		std::unique_ptr<AircraftManager> m_aircraftManager;
		std::unique_ptr<NotificationPanel> m_notificationPanel;
		std::unique_ptr<TextMessageConsole> m_textMessageConsole;
		std::unique_ptr<NearbyATCWindow> m_nearbyAtcWindow;
		std::unique_ptr<SettingsWindow> m_settingsWindow;

		template<class T>
		void SendDto(const T& dto)
		{
			msgpack::sbuffer dtoBuf;
			if (encodeDto(dtoBuf, dto))
			{
				if (dtoBuf.size() == 0)
					return;

				std::vector<unsigned char> dgBuffer(dtoBuf.data(), dtoBuf.data() + dtoBuf.size());
				nng_send(m_socket, reinterpret_cast<char*>(dgBuffer.data()), dgBuffer.size(), NNG_FLAG_NONBLOCK);
			}
		}
	};
}

#endif // !XPilot_h