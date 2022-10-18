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

#include "Plugin.h"
#include "XPilot.h"
#include "Config.h"
#include "Utilities.h"
#include "AircraftManager.h"
#include "NetworkAircraft.h"
#include "FrameRateMonitor.h"
#include "NearbyATCWindow.h"
#include "SettingsWindow.h"
#include "NotificationPanel.h"
#include "TextMessageConsole.h"
#include "XPMPMultiplayer.h"

namespace xpilot {
	XPilot::XPilot() :
		m_xplaneAtisEnabled("sim/atc/atis_enabled", ReadWrite),
		m_pttPressed("xpilot/ptt", ReadWrite),
		m_rxCom1("xpilot/audio/com1_rx", ReadWrite),
		m_rxCom2("xpilot/audio/com2_rx", ReadWrite),
		m_networkLoginStatus("xpilot/login/status", ReadOnly),
		m_networkCallsign("xpilot/login/callsign", ReadWrite),
		m_volumeSignalLevel("xpilot/audio/vu", ReadWrite),
		m_aiControlled("xpilot/ai_controlled", ReadOnly),
		m_aircraftCount("xpilot/num_aircraft", ReadOnly),
		m_pluginVersion("xpilot/version", ReadOnly),
		m_selcalCode("xpilot/selcal", ReadOnly),
		m_selcalReceived("xpilot/selcal_received", ReadWrite),
		m_selcalMuteOverride("xpilot/selcal_mute_override", ReadWrite),
		m_com1StationCallsign("xpilot/com1_station_callsign", ReadOnly),
		m_com2StationCallsign("xpilot/com2_station_callsign", ReadOnly),
		m_frameRatePeriod("sim/operation/misc/frame_rate_period", ReadOnly),
		m_com1Frequency("sim/cockpit2/radios/actuators/com1_frequency_hz_833", ReadWrite),
		m_com2Frequency("sim/cockpit2/radios/actuators/com2_frequency_hz_833", ReadWrite),
		m_audioComSelection("sim/cockpit2/radios/actuators/audio_com_selection", ReadWrite),
		m_audioSelectionCom1("sim/cockpit2/radios/actuators/audio_selection_com1", ReadWrite),
		m_audioSelectionCom2("sim/cockpit2/radios/actuators/audio_selection_com2", ReadWrite),
		m_transponderCode("sim/cockpit/radios/transponder_code", ReadWrite),
		m_avionicsPower("sim/cockpit2/switches/avionics_power_on", ReadOnly) {
		m_bulkDataQuick = XPLMRegisterDataAccessor("xpilot/bulk/quick",
			xplmType_Data,
			false,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL, NULL,
			NULL, NULL,
			NULL, NULL,
			GetBulkData,
			NULL,
			(void*)xpilot::dataRefs::DR_BULK_QUICK,
			(void*)xpilot::dataRefs::DR_BULK_QUICK
		);

		m_bulkDataExpensive = XPLMRegisterDataAccessor("xpilot/bulk/expensive",
			xplmType_Data,
			false,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL, NULL,
			NULL, NULL,
			NULL, NULL,
			GetBulkData,
			NULL,
			(void*)xpilot::dataRefs::DR_BULK_EXPENSIVE,
			(void*)xpilot::dataRefs::DR_BULK_EXPENSIVE
		);

		int left, top, right, bottom, screenTop, screenRight;
		XPLMGetScreenBoundsGlobal(nullptr, &screenTop, &screenRight, nullptr);
		right = screenRight - 35; /*margin right*/
		top = screenTop - 35; /*margin top*/
		left = screenRight - 800; /*width*/
		bottom = top - 100; /*height*/
		m_notificationPanel = std::make_unique<NotificationPanel>(left, top, right, bottom);
		m_textMessageConsole = std::make_unique<TextMessageConsole>(this);
		m_nearbyAtcWindow = std::make_unique<NearbyATCWindow>(this);
		m_settingsWindow = std::make_unique<SettingsWindow>();
		m_frameRateMonitor = std::make_unique<FrameRateMonitor>(this);
		m_aircraftManager = std::make_unique<AircraftManager>(this);
		m_pluginVersion = PLUGIN_VERSION;

		XPLMRegisterFlightLoopCallback(DeferredStartup, -1.0f, this);
	}

	XPilot::~XPilot() {
		XPLMUnregisterDataAccessor(m_bulkDataQuick);
		XPLMUnregisterDataAccessor(m_bulkDataExpensive);
		XPLMUnregisterFlightLoopCallback(DeferredStartup, this);
		XPLMUnregisterFlightLoopCallback(MainFlightLoop, this);
	}

	void XPilot::Initialize() {
		InitializeXPMP();
		TryGetTcasControl();
		XPLMRegisterFlightLoopCallback(MainFlightLoop, -1.0f, this);

		int rv;
		if ((rv = nng_pair1_open(&_socket)) != 0) {
			LOG_MSG(logERROR, "Error opening socket: %s", nng_strerror(rv));
			return;
		}

		nng_setopt_int(_socket, NNG_OPT_RECVBUF, 1024);
		nng_setopt_int(_socket, NNG_OPT_SENDBUF, 1024);

		std::string url = "ipc:///tmp//xpilot.ipc";
		if (Config::GetInstance().GetUseTcpSocket() && Config::GetInstance().GetTcpPort() > 0) {
			url = string_format("tcp://*:%i", Config::GetInstance().GetTcpPort());
		}

		if ((rv = nng_listen(_socket, url.c_str(), NULL, 0)) != 0) {
			LOG_MSG(logERROR, "Socket listen error (%s): %s", url.c_str(), nng_strerror(rv));
			return;
		}
		LOG_MSG(logMSG, "Now listening on %s", url.c_str());

		m_keepSocketAlive = true;
		m_socketThread = std::make_unique<std::thread>(&XPilot::SocketWorker, this);
	}

	void XPilot::Shutdown() {
		ShutdownDto dto{};
		SendDto(dto);

		m_keepSocketAlive = false;

		nng_close(_socket);
		_socket = NNG_SOCKET_INITIALIZER;
		nng_fini();

		if (m_socketThread) {
			m_socketThread->join();
		}
	}

	int CBIntPrefsFunc(const char*, [[maybe_unused]] const char* item, int defaultVal) {
		if (!strcmp(item, XPMP_CFG_ITM_MODELMATCHING))
			return Config::GetInstance().GetDebugModelMatching();
		if (!strcmp(item, XPMP_CFG_ITM_LOGLEVEL))
			return Config::GetInstance().GetLogLevel();
		if (!strcmp(item, XPMP_CFG_ITM_CLAMPALL))
			return 0;
		return defaultVal;
	}

	bool XPilot::InitializeXPMP() {
		const std::string pathResources(GetPluginPath() + "Resources");

		auto err = XPMPMultiplayerInit(PLUGIN_NAME, pathResources.c_str(), &CBIntPrefsFunc);

		if (*err) {
			LOG_MSG(logERROR, "Error initializing multiplayer: %s", err);
			XPMPMultiplayerCleanup();
			return false;
		}

		if (!Config::GetInstance().HasValidPaths()) {
			std::string err = "There are no valid CSL paths configured. Please verify your CSL configuration in X-Plane (Plugins > xPilot > Settings > CSL Configuration).";
			NotificationPosted(err.c_str(), 192, 57, 43, true);
			LOG_MSG(logERROR, err.c_str());
		} else {
			for (const CslPackage& p : Config::GetInstance().GetCSLPackages()) {
				if (!p.path.empty() && p.enabled && CountFilesInPath(p.path) > 0) {
					try {
						err = XPMPLoadCSLPackage(p.path.c_str());
						if (*err) {
							LOG_MSG(logERROR, "Error loading CSL package %s: %s", p.path.c_str(), err);
						}
					}
					catch (std::exception& e) {
						LOG_MSG(logERROR, "Error loading CSL package %s: %s", p.path.c_str(), err);
					}
				}
			}
		}

		XPMPEnableAircraftLabels(Config::GetInstance().GetShowHideLabels());
		XPMPSetAircraftLabelDist(Config::GetInstance().GetMaxLabelDistance(), Config::GetInstance().GetLabelCutoffVis());
		return true;
	}

	float XPilot::DeferredStartup(float, float, int, void* ref) {
		auto* instance = static_cast<XPilot*>(ref);
		if (instance)
			instance->Initialize();
		return 0;
	}

	float XPilot::MainFlightLoop(float inElapsedSinceLastCall, float, int, void* ref) {
		auto* instance = static_cast<XPilot*>(ref);
		if (instance) {
			instance->InvokeQueuedCallbacks();
			instance->m_aiControlled = XPMPHasControlOfAIAircraft();
			instance->m_aircraftCount = XPMPCountPlanes();
			UpdateMenuItems();
		}
		return -1.0;
	}

	void XPilot::SocketWorker() {
		while (m_keepSocketAlive) {
			char* buffer;
			size_t bufferLen;

			int err;
			err = nng_recv(_socket, &buffer, &bufferLen, NNG_FLAG_ALLOC);

			if (err == 0) {
				BaseDto dto;
				auto obj = msgpack::unpack(reinterpret_cast<const char*>(buffer), bufferLen);

				try {
					obj.get().convert(dto);
					ProcessPacket(dto);
				}
				catch (const msgpack::type_error& e) {}

				nng_free(buffer, bufferLen);
			}
		}
	}

	void XPilot::ProcessPacket(const BaseDto& packet) {
		if (packet.type == dto::PLUGIN_VER) {
			PluginVersionDto dto{ PLUGIN_VERSION };
			SendDto(dto);
		}
		if (packet.type == dto::VALIDATE_CSL) {
			ValidateCslDto dto{ XPMPGetNumberOfInstalledModels() > 0 };
			SendDto(dto);
		}
		if (packet.type == dto::ADD_AIRCRAFT) {
			AddAircraftDto dto;
			packet.dto.convert(dto);

			AircraftVisualState visualState{};
			visualState.Lat = dto.latitude;
			visualState.Lon = dto.longitude;
			visualState.Heading = dto.heading;
			visualState.AltitudeTrue = dto.altitudeTrue;
			visualState.Pitch = dto.pitch;
			visualState.Bank = dto.bank;

			if (!dto.callsign.empty() && !dto.typeCode.empty()) {
				QueueCallback([=] {
					m_aircraftManager->HandleAddPlane(dto.callsign, visualState, dto.airline, dto.typeCode);
				});
			}
		}
		if (packet.type == dto::HEARTBEAT) {
			HeartbeatDto dto;
			packet.dto.convert(dto);

			QueueCallback([=] {
				m_aircraftManager->HandleHeartbeat(dto.callsign);
			});
		}
		if (packet.type == dto::FAST_POSITION_UPDATE) {
			FastPositionUpdateDto dto;
			packet.dto.convert(dto);

			AircraftVisualState visualState{};
			visualState.Lat = dto.latitude;
			visualState.Lon = dto.longitude;
			visualState.AltitudeTrue = dto.altitudeTrue;
			visualState.AltitudeAgl = dto.altitudeAgl;
			visualState.Pitch = dto.pitch;
			visualState.Bank = dto.bank;
			visualState.Heading = dto.heading;
			visualState.NoseWheelAngle = dto.noseWheelAngle;

			Vector3 positionalVector{};
			positionalVector.X = dto.vx; // vel lon
			positionalVector.Y = dto.vy; // vel alt
			positionalVector.Z = dto.vz; // vel lat

			Vector3 rotationalVelocity{};
			rotationalVelocity.X = dto.vp * -1; // vel pitch
			rotationalVelocity.Y = dto.vh; // vel heading
			rotationalVelocity.Z = dto.vb * -1; // vel bank

			if (!dto.callsign.empty()) {
				QueueCallback([=] {
					m_aircraftManager->HandleFastPositionUpdate(dto.callsign,
						visualState, positionalVector, rotationalVelocity, dto.speed);
				});
			}
		}
		if (packet.type == dto::DELETE_AIRCRAFT) {
			DeleteAircraftDto dto;
			packet.dto.convert(dto);

			if (!dto.callsign.empty()) {
				QueueCallback([=] {
					m_aircraftManager->HandleRemovePlane(dto.callsign);
				});
			}
		}
		if (packet.type == dto::DELETE_ALL_AIRCRAFT) {
			QueueCallback([=] {
				m_aircraftManager->RemoveAllPlanes();
			});
		}
		if (packet.type == dto::AIRCRAFT_CONFIG) {
			AircraftConfigDto dto;
			packet.dto.convert(dto);
			QueueCallback([=] {
				m_aircraftManager->HandleAircraftConfig(dto.callsign, dto);
			});
		}
		if (packet.type == dto::NOTIFICATION_POSTED) {
			NotificationPostedDto dto;
			packet.dto.convert(dto);

			long color = dto.color;
			int red = ((color >> 16) & 0xff);
			int green = ((color >> 8) & 0xff);
			int blue = ((color) & 0xff);

			NotificationPosted(dto.message.c_str(), red, green, blue);
		}
		if (packet.type == dto::RADIO_MESSAGE_SENT) {
			RadioMessageSentDto dto;
			packet.dto.convert(dto);

			std::string msg = dto.message.c_str();
			NotificationPosted(msg, 0, 255, 255);
		}
		if (packet.type == dto::RADIO_MESSAGE_RECEIVED) {
			RadioMessageReceivedDto dto;
			packet.dto.convert(dto);

			double r = dto.isDirect ? 255 : 192;
			double g = dto.isDirect ? 255 : 192;
			double b = dto.isDirect ? 255 : 192;
			std::string msg = string_format("%s: %s", dto.from.c_str(), dto.message.c_str());
			NotificationPosted(msg, r, g, b);
		}
		if (packet.type == dto::PRIVATE_MESSAGE_SENT) {
			PrivateMessageSentDto dto;
			packet.dto.convert(dto);

			std::string msg = dto.message;
			std::string to = dto.to;
			AddPrivateMessage(to, msg, ConsoleTabType::Sent);
			AddNotificationPanelMessage(string_format("%s [pvt]: %s", m_networkCallsign.value().c_str(), msg.c_str()), 0, 255, 255);
		}
		if (packet.type == dto::PRIVATE_MESSAGE_RECEIVED) {
			PrivateMessageReceivedDto dto;
			packet.dto.convert(dto);

			std::string msg = dto.message;
			std::string from = dto.from;
			AddPrivateMessage(from, msg, ConsoleTabType::Received);
			AddNotificationPanelMessage(string_format("%s [pvt]: %s", from.c_str(), msg.c_str()), 255, 255, 255);
		}
		if (packet.type == dto::NEARBY_ATC) {
			NearbyAtcDto dto;
			packet.dto.convert(dto);

			QueueCallback([=] {
				m_nearbyAtcWindow->UpdateList(dto);
			});
		}
		if (packet.type == dto::CONNECTED) {
			ConnectedDto dto;
			packet.dto.convert(dto);

			std::string callsign = dto.callsign;
			std::string selcal = dto.selcal;

			QueueCallback([=] {
				m_aircraftManager->RemoveAllPlanes();
				m_frameRateMonitor->StartMonitoring();
				TryGetTcasControl();
				m_xplaneAtisEnabled = 0;
				m_networkCallsign.setValue(callsign);
				m_selcalCode.setValue(selcal);
				m_networkLoginStatus.setValue(true);
				m_com1StationCallsign.setValue("");
				m_com2StationCallsign.setValue("");
			});
		}
		if (packet.type == dto::DISCONNECTED) {
			QueueCallback([=] {
				m_aircraftManager->RemoveAllPlanes();
				m_frameRateMonitor->StopMonitoring();
				m_nearbyAtcWindow->UpdateList({});
				ReleaseTcasControl();
				m_xplaneAtisEnabled = 1;
				m_networkCallsign.setValue("");
				m_selcalCode.setValue("");
				m_networkLoginStatus.setValue(false);
				m_com1StationCallsign.setValue("");
				m_com2StationCallsign.setValue("");
			});
		}
		if (packet.type == dto::STATION_CALLSIGN) {
			ComStationCallsign dto;
			packet.dto.convert(dto);

			std::string callsign = dto.callsign;
			int comStack = dto.com;

			QueueCallback([=] {
				switch (comStack) {
				case 1:
					m_com1StationCallsign.setValue(callsign);
					break;
				case 2:
					m_com2StationCallsign.setValue(callsign);
					break;
				}
			});
		}
	}

	void XPilot::OnNetworkConnected() {
		m_aircraftManager->RemoveAllPlanes();
		m_frameRateMonitor->StartMonitoring();
		m_xplaneAtisEnabled = 0;
		m_networkLoginStatus = 1;
		TryGetTcasControl();
	}

	void XPilot::OnNetworkDisconnected() {
		m_aircraftManager->RemoveAllPlanes();
		m_frameRateMonitor->StopMonitoring();
		m_xplaneAtisEnabled = 1;
		m_networkLoginStatus = 0;
		m_networkCallsign = "";
		ReleaseTcasControl();
	}

	void XPilot::ForceDisconnect(std::string reason) {
		ForcedDisconnectDto dto{ reason };
		SendDto(dto);
	}

	void XPilot::RequestStationInfo(std::string station) {
		RequestStationInfoDto dto{ station };
		SendDto(dto);
	}

	void XPilot::RequestMetar(std::string station) {
		RequestMetarDto dto{ station };
		SendDto(dto);
	}

	void XPilot::SetCom1Frequency(int frequency) {
		QueueCallback([=] {
			m_com1Frequency = frequency;
		});
	}

	void XPilot::SetCom2Frequency(int frequency) {
		QueueCallback([=] {
			m_com2Frequency = frequency;
		});
	}

	void XPilot::SetAudioSelection(int radio, bool on) {
		QueueCallback([=] {
			switch (radio) {
			case 1:
				m_audioSelectionCom1 = (int)on;
				break;
			case 2:
				m_audioSelectionCom2 = (int)on;
				break;
			}
		});
	}

	void XPilot::SetAudioComSelection(int radio) {
		QueueCallback([=] {
			m_audioComSelection = (radio == 1) ? 6 : 7;
		});
	}

	void XPilot::SetTransponderCode(int code) {
		QueueCallback([=] {
			m_transponderCode = code;
		});
	}

	void XPilot::DisableXplaneAtis(bool disabled) {
		m_xplaneAtisEnabled = (int)disabled;
	}

	void XPilot::SendWallop(std::string message) {
		WallopSentDto dto{ message };
		SendDto(dto);
	}

	void XPilot::SendRadioMessage(std::string message) {
		if (!IsNetworkConnected())
			return;

		RadioMessageSentDto dto{ message };
		SendDto(dto);
	}

	void XPilot::SendPrivateMessage(std::string to, std::string message) {
		if (!IsNetworkConnected())
			return;

		PrivateMessageSentDto dto{ to, message };
		SendDto(dto);
	}

	void XPilot::AddPrivateMessage(const std::string& recipient, const std::string& msg, ConsoleTabType tabType) {
		if (!recipient.empty() && !msg.empty()) {
			QueueCallback([=]() {
				m_textMessageConsole->HandlePrivateMessage(recipient, msg, tabType);
			});
		}
	}

	void XPilot::RadioMessageReceived(const std::string& msg, double red, double green, double blue) {
		if (!msg.empty()) {
			QueueCallback([=]() {
				m_textMessageConsole->RadioMessageReceived(msg.c_str(), red, green, blue);
			});
		}
	}

	void XPilot::AddNotificationPanelMessage(const std::string& msg, double red, double green, double blue, bool forceShow) {
		if (!msg.empty()) {
			QueueCallback([=]() {
				m_notificationPanel->AddNotificationPanelMessage(msg, red, green, blue, forceShow);
			});
		}
	}

	void XPilot::NotificationPosted(const std::string& msg, double red, double green, double blue, bool forceShow) {
		RadioMessageReceived(msg, red, green, blue);
		AddNotificationPanelMessage(msg, red, green, blue, forceShow);
	}

	void XPilot::AircraftDeleted(std::string callsign) {
		AircraftDeletedDto dto{ callsign };
		SendDto(dto);
	}

	void XPilot::AircraftAdded(std::string callsign) {
		AircraftAddedDto dto{ callsign };
		SendDto(dto);
	}

	void XPilot::DeleteAllAircraft() {
		m_aircraftManager->RemoveAllPlanes();
	}

	void XPilot::QueueCallback(const std::function<void()>& cb) {
		std::lock_guard<std::mutex> lck(m_mutex);
		m_queuedCallbacks.push_back(cb);
	}

	void XPilot::InvokeQueuedCallbacks() {
		std::deque<std::function<void()>> temp;
		{
			std::lock_guard<std::mutex> lck(m_mutex);
			std::swap(temp, m_queuedCallbacks);
		}
		while (!temp.empty()) {
			auto cb = temp.front();
			temp.pop_front();
			cb();
		}
	}

	void XPilot::ToggleSettingsWindow() {
		m_settingsWindow->SetVisible(!m_settingsWindow->GetVisible());
	}

	void XPilot::ToggleNearbyAtcWindow() {
		m_nearbyAtcWindow->SetVisible(!m_nearbyAtcWindow->GetVisible());
	}

	void XPilot::ToggleTextMessageConsole() {
		m_textMessageConsole->SetVisible(!m_textMessageConsole->GetVisible());
	}

	void XPilot::SetNotificationPanelAlwaysVisible(bool visible) {
		m_notificationPanel->SetAlwaysVisible(visible);
	}

	bool XPilot::GetNotificationPanelAlwaysVisible()const {
		return m_notificationPanel->IsAlwaysVisible();
	}

	void callbackRequestTcasAgain(void*) {
		XPMPMultiplayerEnable(callbackRequestTcasAgain);
	}

	void XPilot::TryGetTcasControl() {
		if (!XPMPHasControlOfAIAircraft()) {
			auto err = XPMPMultiplayerEnable(callbackRequestTcasAgain);
			if (*err) {
				NotificationPosted(err, 231, 76, 60, true);
				LOG_MSG(logERROR, err);
			}
		}
	}

	void XPilot::ReleaseTcasControl() {
		if (XPMPHasControlOfAIAircraft()) {
			XPMPMultiplayerDisable();
			LOG_MSG(logDEBUG, "xPilot has released TCAS control");
		}
	}

	int XPilot::GetBulkData(void* inRefcon, void* outData, int inStartPos, int inNumBytes) {
		dataRefs dr = (dataRefs)reinterpret_cast<long long>(inRefcon);
		assert(dr == xpilot::dataRefs::DR_BULK_QUICK || dr == xpilot::dataRefs::DR_BULK_EXPENSIVE);

		static int size_quick = 0, size_expensive = 0;
		if (!outData) {
			if (dr == xpilot::dataRefs::DR_BULK_QUICK) {
				size_quick = inNumBytes;
				return (int)sizeof(XPilotAPIAircraft::XPilotAPIBulkData);
			} else {
				size_expensive = inNumBytes;
				return (int)sizeof(XPilotAPIAircraft::XPilotAPIBulkInfoTexts);
			}
		}

		int size = dr == xpilot::dataRefs::DR_BULK_QUICK ? size_quick : size_expensive;
		if (!size) return 0;

		if ((inStartPos % size != 0) ||
			(inNumBytes % size != 0))
			return 0;

		const int startAc = 1 + inStartPos / size;
		const int endAc = startAc + (inNumBytes / size);
		char* pOut = (char*)outData;
		int iAc = startAc;
		for (mapPlanesTy::iterator pIter = mapGetAircraftByIndex(iAc);
			pIter != mapPlanes.end() && iAc < endAc;
			pIter = mapGetNextAircraft(pIter), iAc++, pOut += size) {
			const NetworkAircraft& ac = *pIter->second;
			if (dr == xpilot::dataRefs::DR_BULK_QUICK)
				ac.copyBulkData((XPilotAPIAircraft::XPilotAPIBulkData*)pOut, size);
			else
				ac.copyBulkData((XPilotAPIAircraft::XPilotAPIBulkInfoTexts*)pOut, size);
		}

		return (iAc - startAc) * size;
	}
}
