/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2022 Justin Shannon
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
#include "zmq.hpp"
#include "Utilities.h"

#ifdef _WIN32
#define BOOST_INTERPROCESS_SHARED_DIR_FUNC
#endif
#include <interprocess/include/boost/interprocess/ipc/message_queue.hpp>

#include <deque>
#include <thread>
#include <mutex>
#include <functional>
#include <map>
#include <atomic>
#include <algorithm>
#include <iostream>

using namespace std;
namespace bip = boost::interprocess;

#define OUTBOUND_QUEUE "xpilot.outbound"
#define INBOUND_QUEUE "xpilot.inbound"
#define MAX_MESSAGES 500
#define MAX_MESSAGE_SIZE 2048

#ifdef _WIN32
namespace boost {
	namespace interprocess {
		namespace ipcdetail {
			inline void get_shared_dir(std::string& shared_dir) {
				winapi::get_local_app_data(shared_dir);
				shared_dir += "/org.vatsim.xpilot/";
			}
		}
	}
}
#endif

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

		void RadioMessageReceived(const string& msg, double red = 255, double green = 255, double blue = 255);
		void AddPrivateMessage(const string& recipient, const string& msg, ConsoleTabType tabType);
		void AddNotificationPanelMessage(const string& msg, double red = 255, double green = 255, double blue = 255);
		void addNotification(const string& msg, double red = 255, double green = 255, double blue = 255);

		void onNetworkDisconnected();
		void onNetworkConnected();
		void forceDisconnect(string reason = "");
		void requestStationInfo(string callsign);
		void requestMetar(string station);
		void setCom1Frequency(float frequency);
		void setCom2Frequency(float frequency);
		void setAudioComSelection(int radio);
		void setAudioSelection(int radio, bool on);
		void setTransponderCode(int code);
		void sendWallop(string message);

		void SendReply(const string& message);

		string ourCallsign() const { return m_networkCallsign;  }
		bool isNetworkConnected() const { return m_networkLoginStatus; }
		void setPttActive(bool active) { m_pttPressed = active;  }
		int getTxRadio() const { return m_audioComSelection; }
		bool radiosPowered() const { return m_avionicsPower && (m_audioComSelection == 6 || m_audioComSelection == 7); }

		void DisableXplaneAtis(bool disabled);
		bool IsXplaneAtisDisabled() const { return !m_xplaneAtisEnabled; }

		void TryGetTcasControl();
		void ReleaseTcasControl();

		void toggleSettingsWindow();
		void toggleNearbyAtcWindow();
		void toggleTextMessageConsole();
		void setNotificationPanelAlwaysVisible(bool visible);
		bool getNotificationPanelAlwaysVisible() const;

		void DeleteAllAircraft();

		void Initialize();
		void Shutdown();

	protected:
		OwnedDataRef<int> m_pttPressed;
		OwnedDataRef<int> m_networkLoginStatus;
		OwnedDataRef<string> m_networkCallsign;
		OwnedDataRef<int> m_rxCom1;
		OwnedDataRef<int> m_rxCom2;
		OwnedDataRef<float> m_volumeSignalLevel;
		OwnedDataRef<int> m_aiControlled;
		OwnedDataRef<int> m_aircraftCount;
		OwnedDataRef<int> m_pluginVersion;
		OwnedDataRef<string> m_selcalCode;
		OwnedDataRef<int> m_selcalReceived;
		OwnedDataRef<int> m_selcalMuteOverride;
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

		thread::id m_xplaneThread;
		void ThisThreadIsXplane()
		{
			m_xplaneThread = this_thread::get_id();
		}
		bool IsXplaneThread()const
		{
			return this_thread::get_id() == m_xplaneThread;
		}

		bool m_zmqKeepAlive;
		unique_ptr<thread> m_zmqThread;
		unique_ptr<zmq::context_t> m_zmqContext;
		unique_ptr<zmq::socket_t> m_zmqSocket;

		void ZmqWorker();
		void MessageQueueWorker();
		void ProcessMessage(const std::string& msg);
		
		bool IsSocketConnected()const
		{
			return m_zmqSocket != nullptr && m_zmqSocket->handle() != nullptr;
		}
		bool IsSocketReady()const
		{
			return m_zmqKeepAlive && IsSocketConnected();
		}

		mutex m_mutex;
		deque<function<void()>> m_queuedCallbacks;
		void InvokeQueuedCallbacks();
		void QueueCallback(const function<void()>& cb);

		XPLMDataRef m_bulkDataQuick{}, m_bulkDataExpensive{};
		static int GetBulkData(void* inRefcon, void* outData, int inStartPos, int inNumBytes);
		int m_currentAircraftCount = 1;

		unique_ptr<FrameRateMonitor> m_frameRateMonitor;
		unique_ptr<AircraftManager> m_aircraftManager;
		unique_ptr<NotificationPanel> m_notificationPanel;
		unique_ptr<TextMessageConsole> m_textMessageConsole;
		unique_ptr<NearbyATCWindow> m_nearbyAtcWindow;
		unique_ptr<SettingsWindow> m_settingsWindow;

		std::unique_ptr<bip::message_queue> m_outboundQueue;	// xpilot -> xplane
		std::unique_ptr<bip::message_queue> m_inboundQueue;		// xplane -> xpilot
		bool m_keepMessageQueueAlive = false;
		std::unique_ptr<thread> m_messageQueueThread;
		bool IsMessageQueueReady() const { return m_outboundQueue != nullptr && m_keepMessageQueueAlive; }
	};
}

#endif // !XPilot_h