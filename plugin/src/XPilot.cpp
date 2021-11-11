/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2021 Justin Shannon
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
#include "NetworkAircraftConfig.h"
#include "FrameRateMonitor.h"
#include "NearbyATCWindow.h"
#include "SettingsWindow.h"
#include "NotificationPanel.h"
#include "TextMessageConsole.h"
#include "XPMP2/XPMPMultiplayer.h"
#include <nlohmann/json.hpp>

#include <regex>

using json = nlohmann::json;

namespace xpilot
{
	XPilot::XPilot() :
		m_xplaneAtisEnabled("sim/atc/atis_enabled", ReadWrite),
		m_pttPressed("xpilot/ptt", ReadWrite),
		m_rxCom1("xpilot/audio/com1_rx", ReadWrite),
		m_rxCom2("xpilot/audio/com2_rx", ReadWrite),
		m_networkLoginStatus("xpilot/login/status", ReadOnly),
		m_networkCallsign("xpilot/login/callsign", ReadOnly),
		m_volumeSignalLevel("xpilot/audio/vu", ReadWrite),
		m_aiControlled("xpilot/ai_controlled", ReadOnly),
		m_aircraftCount("xpilot/num_aircraft", ReadOnly),
		m_pluginVersion("xpilot/version", ReadOnly),
		m_frameRatePeriod("sim/operation/misc/frame_rate_period", ReadOnly),
		m_com1Frequency("sim/cockpit2/radios/actuators/com1_frequency_hz_833", ReadWrite),
		m_com2Frequency("sim/cockpit2/radios/actuators/com2_frequency_hz_833", ReadWrite),
		m_audioComSelection("sim/cockpit2/radios/actuators/audio_com_selection", ReadWrite),
		m_audioSelectionCom1("sim/cockpit2/radios/actuators/audio_selection_com1", ReadWrite),
		m_audioSelectionCom2("sim/cockpit2/radios/actuators/audio_selection_com2", ReadWrite)
	{
		ThisThreadIsXplane();

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
		right = screenRight - 35; /*padding left*/
		top = screenTop - 35; /*width*/
		left = screenRight - 800; /*padding top*/
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

	XPilot::~XPilot()
	{
		XPLMUnregisterDataAccessor(m_bulkDataQuick);
		XPLMUnregisterDataAccessor(m_bulkDataExpensive);
		XPLMUnregisterFlightLoopCallback(DeferredStartup, this);
		XPLMUnregisterFlightLoopCallback(MainFlightLoop, this);
	}

	float XPilot::DeferredStartup(float, float, int, void* ref)
	{
		auto* instance = static_cast<XPilot*>(ref);
		if (instance)
		{
			instance->Initialize();

		}
		return 0;
	}

	void XPilot::Initialize()
	{
		InitializeXPMP();
		TryGetTcasControl();

		if (m_zmqThread)
		{
			m_keepAlive = false;
			m_zmqThread->join();
			m_zmqThread.reset();
		}

		try
		{
			m_zmqContext = std::make_unique<zmq::context_t>(1);
			m_zmqSocket = std::make_unique<zmq::socket_t>(*m_zmqContext.get(), ZMQ_ROUTER);
			m_zmqSocket->setsockopt(ZMQ_IDENTITY, "xplane", 5);
			m_zmqSocket->setsockopt(ZMQ_LINGER, 0);
			m_zmqSocket->bind("tcp://*:" + Config::Instance().getTcpPort());
		}
		catch (zmq::error_t& e)
		{
		}
		catch (const std::exception& e)
		{
		}
		catch (...)
		{
		}

		XPLMRegisterFlightLoopCallback(MainFlightLoop, -1.0f, this);

		m_keepAlive = true;
		m_zmqThread = std::make_unique<std::thread>(&XPilot::ZmqWorker, this);
		LOG_MSG(logMSG, "Now listening on port %s", Config::Instance().getTcpPort().c_str());
	}

	void XPilot::Shutdown()
	{
		try
		{
			if (m_zmqSocket)
			{
				m_zmqSocket->close();
				m_zmqContext->close();
			}
		}
		catch (zmq::error_t& e)
		{
		}
		catch (std::exception& e)
		{
		}
		catch (...)
		{
		}

		m_keepAlive = false;

		if (m_zmqThread)
		{
			m_zmqThread->join();
			m_zmqThread.reset();
		}
	}

	void XPilot::ZmqWorker()
	{
		while (IsSocketReady())
		{
			try
			{
				zmq::message_t msg;
				m_zmqSocket->recv(msg, zmq::recv_flags::none);
				std::string data(static_cast<char*>(msg.data()), msg.size());

				if (!data.empty() && json::accept(data.c_str()))
				{
					json j = json::parse(data.c_str());

					if (j.find("type") != j.end())
					{
						std::string MessageType(j["type"]);

						if (!MessageType.empty())
						{
							if (MessageType == "AddPlane")
							{
								std::string callsign(j["data"]["callsign"]);
								std::string airline(j["data"]["airline"]);
								std::string typeCode(j["data"]["type_code"]);
								double latitude = static_cast<double>(j["data"]["latitude"]);
								double longitude = static_cast<double>(j["data"]["longitude"]);
								double altitude = static_cast<double>(j["data"]["altitude"]);
								double heading = static_cast<double>(j["data"]["heading"]);
								double bank = static_cast<double>(j["data"]["bank"]);
								double pitch = static_cast<double>(j["data"]["pitch"]);

								AircraftVisualState visualState{};
								visualState.Lat = latitude;
								visualState.Lon = longitude;
								visualState.Heading = heading;
								visualState.Altitude = altitude;
								visualState.Pitch = pitch;
								visualState.Bank = bank;

								if (!callsign.empty() && !typeCode.empty())
								{
									QueueCallback([=]
										{
											m_aircraftManager->HandleAddPlane(callsign, visualState, airline, typeCode);
										});
								}
							}

							else if (MessageType == "ChangeModel")
							{
								std::string callsign(j["data"]["callsign"]);
								std::string airline(j["data"]["airline"]);
								std::string typeCode(j["data"]["type_code"]);

								if (!callsign.empty() && !typeCode.empty())
								{
									QueueCallback([=]
										{
											m_aircraftManager->HandleChangePlaneModel(callsign, typeCode, airline);
										});
								}
							}

							else if (MessageType == "SlowPositionUpdate")
							{
								std::string callsign(j["data"]["callsign"]);
								double latitude = static_cast<double>(j["data"]["latitude"]);
								double longitude = static_cast<double>(j["data"]["longitude"]);
								double altitude = static_cast<double>(j["data"]["altitude"]);
								double heading = static_cast<double>(j["data"]["heading"]);
								double bank = static_cast<double>(j["data"]["bank"]);
								double pitch = static_cast<double>(j["data"]["pitch"]);
								double groundSpeed = static_cast<double>(j["data"]["ground_speed"]);

								AircraftVisualState visualState{};
								visualState.Lat = latitude;
								visualState.Lon = longitude;
								visualState.Heading = heading;
								visualState.Altitude = altitude;
								visualState.Pitch = pitch;
								visualState.Bank = bank;

								if (!callsign.empty())
								{
									QueueCallback([=]
										{
											m_aircraftManager->HandleSlowPositionUpdate(callsign, visualState, groundSpeed);
										});
								}
							}

							else if (MessageType == "FastPositionUpdate")
							{
								std::string callsign(j["data"]["callsign"]);
								double latitude = static_cast<double>(j["data"]["latitude"]);
								double longitude = static_cast<double>(j["data"]["longitude"]);
								double altitude = static_cast<double>(j["data"]["altitude"]);
								double heading = static_cast<double>(j["data"]["heading"]);
								double bank = static_cast<double>(j["data"]["bank"]);
								double pitch = static_cast<double>(j["data"]["pitch"]);
								double velocityLongitude = static_cast<double>(j["data"]["vx"]);
								double velocityAltitude = static_cast<double>(j["data"]["vy"]);
								double velocityLatitude = static_cast<double>(j["data"]["vz"]);
								double velocityPitch = static_cast<double>(j["data"]["vp"]);
								double velocityHeading = static_cast<double>(j["data"]["vh"]);
								double velocityBank = static_cast<double>(j["data"]["vb"]);

								AircraftVisualState visualState{};
								visualState.Lat = latitude;
								visualState.Lon = longitude;
								visualState.Altitude = altitude;
								visualState.Pitch = pitch;
								visualState.Bank = bank;
								visualState.Heading = heading;

								Vector3 positionalVector{};
								positionalVector.X = velocityLongitude;
								positionalVector.Y = velocityAltitude;
								positionalVector.Z = velocityLatitude;

								Vector3 rotationalVelocity{};
								rotationalVelocity.X = velocityPitch * -1;
								rotationalVelocity.Y = velocityHeading;
								rotationalVelocity.Z = velocityBank * -1;

								if (!callsign.empty())
								{
									QueueCallback([=]
										{
											m_aircraftManager->HandleFastPositionUpdate(callsign,
												visualState, positionalVector, rotationalVelocity);
										});
								}
							}

							else if (MessageType == "AirplaneConfig")
							{
								auto acconfig = j.get<NetworkAircraftConfig>();
								QueueCallback([=]
									{
										m_aircraftManager->HandleAircraftConfig(acconfig.data.callsign, acconfig);
									});
							}

							else if (MessageType == "RemovePlane")
							{
								std::string callsign(j["data"]["callsign"]);

								if (!callsign.empty())
								{
									QueueCallback([=]
										{
											m_aircraftManager->HandleRemovePlane(callsign);
										});
								}
							}

							else if (MessageType == "RemoveAllPlanes")
							{
								QueueCallback([=]
									{
										m_aircraftManager->RemoveAllPlanes();
									});
							}

							else if (MessageType == "NetworkConnected")
							{
								std::string callsign(j["data"]["callsign"]);

								QueueCallback([=]
									{
										m_aircraftManager->RemoveAllPlanes();
										m_frameRateMonitor->startMonitoring();
										TryGetTcasControl();
										m_xplaneAtisEnabled = 0;
										m_networkCallsign.setValue(callsign);
										m_networkLoginStatus.setValue(true);
									});
							}

							else if (MessageType == "NetworkDisconnected")
							{
								QueueCallback([=]
									{
										m_aircraftManager->RemoveAllPlanes();
										m_frameRateMonitor->stopMonitoring();
										ReleaseTcasControl();
										m_xplaneAtisEnabled = 1;
										m_networkCallsign.setValue("");
										m_networkLoginStatus.setValue(false);
									});
							}

							else if (MessageType == "NearbyAtc")
							{
								QueueCallback([=]
									{
										m_nearbyAtcWindow->UpdateList(j);
									});
							}

							else if (MessageType == "NotificationPosted")
							{
								std::string msg(j["data"]["message"]);
								long color = static_cast<long>(j["data"]["color"]);
								int red = ((color >> 16) & 0xff);
								int green = ((color >> 8) & 0xff);
								int blue = ((color) & 0xff);
								addNotification(msg, red, green, blue);
							}

							else if (MessageType == "RadioMessageSent")
							{
								std::string msg(j["data"]["message"]);
								RadioMessageReceived(msg, 0, 255, 255);
								AddNotificationPanelMessage(msg, 0, 255, 255);
							}

							else if (MessageType == "RadioMessageReceived")
							{
								std::string msg(j["data"]["message"]);
								bool isDirect = static_cast<bool>(j["data"]["direct"]);
								double r = isDirect ? 255 : 192;
								double g = isDirect ? 255 : 192;
								double b = isDirect ? 255 : 192;
								RadioMessageReceived(msg, r, g, b);
								AddNotificationPanelMessage(msg, r, g, b);
							}

							else if (MessageType == "PrivateMessageReceived")
							{
								std::string msg(j["data"]["message"]);
								std::string from(j["data"]["from"]);
								AddPrivateMessage(from, msg, ConsoleTabType::Received);
								AddNotificationPanelMessage(string_format("%s [pvt]: %s", from.c_str(), msg.c_str()), 255, 255, 255);
							}

							else if (MessageType == "PrivateMessageSent")
							{
								std::string msg(j["data"]["message"]);
								std::string to(j["data"]["to"]);
								AddPrivateMessage(to, msg, ConsoleTabType::Sent);
								AddNotificationPanelMessage(string_format("%s [pvt]: %s", m_networkCallsign.value().c_str(), msg.c_str()), 255, 255, 255);
							}

							else if (MessageType == "ValidateCsl")
							{
								json reply;
								reply["type"] = "ValidateCsl";
								reply["data"]["is_valid"] = XPMPGetNumberOfInstalledModels() > 0;
								SendReply(reply.dump());
							}

							else if (MessageType == "PluginVersion")
							{
								json reply;
								reply["type"] = "PluginVersion";
								reply["data"]["version"] = PLUGIN_VERSION;
								SendReply(reply.dump());
							}
						}
					}
				}
			}
			catch (zmq::error_t& e)
			{
				if (e.num() != ETERM)
				{
					LOG_MSG(logERROR, "Socket receive exception: %s", e.what());
				}
			}
			catch (std::exception& e)
			{
				LOG_MSG(logERROR, "Socket receive exception: %s", e.what());
			}
			catch (...)
			{
			}
		}
	}

	void XPilot::SendReply(const std::string& message)
	{
		try
		{
			if (IsSocketConnected() && !message.empty())
			{
				std::string identity = "xpilot";
				zmq::message_t part1(identity.size());
				std::memcpy(part1.data(), identity.data(), identity.size());
				m_zmqSocket->send(part1, zmq::send_flags::sndmore);

				zmq::message_t part2(message.size());
				std::memcpy(part2.data(), message.data(), message.size());
				zmq::send_result_t rc = m_zmqSocket->send(part2, zmq::send_flags::none);
			}
		}
		catch (zmq::error_t& e)
		{
			LOG_MSG(logERROR, "Error sending socket message: %s", e.what());
		}
	}

	float XPilot::MainFlightLoop(float inElapsedSinceLastCall, float, int, void* ref)
	{
		auto* instance = static_cast<XPilot*>(ref);
		if (instance)
		{
			instance->InvokeQueuedCallbacks();
			instance->m_aiControlled = XPMPHasControlOfAIAircraft();
			instance->m_aircraftCount = XPMPCountPlanes();
			UpdateMenuItems();
		}
		return -1.0;
	}

	void XPilot::DisableXplaneAtis(bool disabled)
	{
		m_xplaneAtisEnabled = (int)disabled;
	}

	int CBIntPrefsFunc(const char*, [[maybe_unused]] const char* item, int defaultVal)
	{
		if (!strcmp(item, XPMP_CFG_ITM_MODELMATCHING))
			return Config::Instance().getDebugModelMatching();
		if (!strcmp(item, XPMP_CFG_ITM_LOGLEVEL))
			return Config::Instance().getLogLevel();
		if (!strcmp(item, XPMP_CFG_ITM_REPLDATAREFS))
			return 1;
		if (!strcmp(item, XPMP_CFG_ITM_CLAMPALL))
			return 0;
		return defaultVal;
	}

	void XPilot::onNetworkConnected()
	{
		m_aircraftManager->RemoveAllPlanes();
		m_frameRateMonitor->startMonitoring();
		m_xplaneAtisEnabled = 0;
		m_networkLoginStatus = 1;
		TryGetTcasControl();
	}

	void XPilot::onNetworkDisconnected()
	{
		m_aircraftManager->RemoveAllPlanes();
		m_frameRateMonitor->stopMonitoring();
		m_xplaneAtisEnabled = 1;
		m_networkLoginStatus = 0;
		m_networkCallsign = "";
		ReleaseTcasControl();
	}

	void XPilot::forceDisconnect(std::string reason)
	{
		json msg;
		msg["type"] = "ForceDisconnect";
		msg["data"]["reason"] = reason;
		SendReply(msg.dump());
	}

	void XPilot::requestStationInfo(std::string callsign)
	{
		json msg;
		msg["type"] = "RequestStationInfo";
		msg["data"]["callsign"] = callsign;
		SendReply(msg.dump());
	}

	void XPilot::requestMetar(std::string station)
	{
		json msg;
		msg["type"] = "RequestMetar";
		msg["data"]["station"] = station;
		SendReply(msg.dump());
	}

	void XPilot::setCom1Frequency(float frequency)
	{
		QueueCallback([=]
			{
				m_com1Frequency = frequency;
			});
	}

	void XPilot::setCom2Frequency(float frequency)
	{
		QueueCallback([=]
			{
				m_com2Frequency = frequency;
			});
	}

	void XPilot::setAudioSelection(int radio, bool on)
	{
		QueueCallback([=]
			{
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

	void XPilot::setAudioComSelection(int radio)
	{
		QueueCallback([=]
			{
				m_audioComSelection = (radio == 1) ? 6 : 7;
			});
	}

	bool XPilot::InitializeXPMP()
	{
		const std::string pathResources(GetPluginPath() + "Resources");

		auto err = XPMPMultiplayerInit(PLUGIN_NAME, pathResources.c_str(), &CBIntPrefsFunc);

		if (*err)
		{
			LOG_MSG(logERROR, "Error initializing multiplayer: %s", err);
			XPMPMultiplayerCleanup();
			return false;
		}

		if (!Config::Instance().hasValidPaths())
		{
			std::string err = "No valid CSL paths are configured (or enabled). Verify the CSL configuration in X-Plane (via the top menu: Plugins > xPilot > Settings > CSL Configuration).";
			addNotification(err.c_str(), 192, 57, 43);
			LOG_MSG(logERROR, err.c_str());
		}
		else
		{
			for (const CslPackage& p : Config::Instance().getCSLPackages())
			{
				if (!p.path.empty() && p.enabled && CountFilesInPath(p.path) > 0)
				{
					try
					{
						err = XPMPLoadCSLPackage(p.path.c_str());
						if (*err)
						{
							LOG_MSG(logERROR, "Error loading CSL package %s: %s", p.path.c_str(), err);
						}
					}
					catch (std::exception& e)
					{
						LOG_MSG(logERROR, "Error loading CSL package %s: %s", p.path.c_str(), err);
					}
				}
			}
		}

		XPMPEnableAircraftLabels(Config::Instance().getShowHideLabels());
		XPMPSetAircraftLabelDist(Config::Instance().getMaxLabelDistance(), Config::Instance().getLabelCutoffVis());
		return true;
	}

	void XPilot::AddPrivateMessage(const std::string& recipient, const std::string& msg, ConsoleTabType tabType)
	{
		if (!recipient.empty() && !msg.empty())
		{
			QueueCallback([=]()
			{
				m_textMessageConsole->HandlePrivateMessage(recipient, msg, tabType);
			});
		}
	}

	void XPilot::RadioMessageReceived(const std::string& msg, double red, double green, double blue)
	{
		if (!msg.empty())
		{
			QueueCallback([=]()
			{
				m_textMessageConsole->RadioMessageReceived(msg.c_str(), red, green, blue);
			});
		}
	}

	void XPilot::AddNotificationPanelMessage(const std::string& msg, double red, double green, double blue)
	{
		if (!msg.empty())
		{
			QueueCallback([=]()
			{
				m_notificationPanel->AddNotificationPanelMessage(msg, red, green, blue);
			});
		}
	}

	void XPilot::addNotification(const std::string& msg, double red, double green, double blue)
	{
		RadioMessageReceived(msg, red, green, blue);
		AddNotificationPanelMessage(msg, red, green, blue);
	}

	void XPilot::QueueCallback(const std::function<void()> &cb)
	{
		std::lock_guard<std::mutex> lck(m_mutex);
		m_queuedCallbacks.push_back(cb);
	}

	void XPilot::InvokeQueuedCallbacks()
	{
		std::deque<std::function<void()>> temp;
		{
			std::lock_guard<std::mutex> lck(m_mutex);
			std::swap(temp, m_queuedCallbacks);
		}
		while (!temp.empty())
		{
			auto cb = temp.front();
			temp.pop_front();
			cb();
		}
	}

	void XPilot::togglePreferencesWindow()
	{
		m_settingsWindow->SetVisible(!m_settingsWindow->GetVisible());
	}

	void XPilot::toggleNearbyAtcWindow()
	{
		m_nearbyAtcWindow->SetVisible(!m_nearbyAtcWindow->GetVisible());
	}

	void XPilot::toggleTextMessageConsole()
	{
		m_textMessageConsole->SetVisible(!m_textMessageConsole->GetVisible());
	}

	void XPilot::setNotificationPanelAlwaysVisible(bool visible)
	{
		m_notificationPanel->setAlwaysVisible(visible);
	}

	bool XPilot::setNotificationPanelAlwaysVisible()const
	{
		return m_notificationPanel->isAlwaysVisible();
	}

	void callbackRequestTcasAgain(void*)
	{
		XPMPMultiplayerEnable(callbackRequestTcasAgain);
	}

	void XPilot::TryGetTcasControl()
	{
		if (!XPMPHasControlOfAIAircraft())
		{
			auto err = XPMPMultiplayerEnable(callbackRequestTcasAgain);
			if (*err)
			{
				addNotification(err, 231, 76, 60);
				LOG_MSG(logERROR, err);
			}
		}
	}

	void XPilot::ReleaseTcasControl()
	{
		if (XPMPHasControlOfAIAircraft())
		{
			XPMPMultiplayerDisable();
			LOG_MSG(logDEBUG, "xPilot has released TCAS control");
		}
	}

	void XPilot::DeleteAllAircraft()
	{
		m_aircraftManager->RemoveAllPlanes();
	}

	int XPilot::GetBulkData(void* inRefcon, void* outData, int inStartPos, int inNumBytes)
	{
		dataRefs dr = (dataRefs)reinterpret_cast<long long>(inRefcon);
		assert(dr == xpilot::dataRefs::DR_BULK_QUICK || dr == xpilot::dataRefs::DR_BULK_EXPENSIVE);

		static int size_quick = 0, size_expensive = 0;
		if (!outData)
		{
			if (dr == xpilot::dataRefs::DR_BULK_QUICK)
			{
				size_quick = inNumBytes;
				return (int)sizeof(XPilotAPIAircraft::XPilotAPIBulkData);
			}
			else
			{
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
			pIter = mapGetNextAircraft(pIter), iAc++, pOut += size)
		{
			const NetworkAircraft& ac = *pIter->second;
			if (dr == xpilot::dataRefs::DR_BULK_QUICK)
				ac.copyBulkData((XPilotAPIAircraft::XPilotAPIBulkData*)pOut, size);
			else
				ac.copyBulkData((XPilotAPIAircraft::XPilotAPIBulkInfoTexts*)pOut, size);
		}

		return (iAc - startAc) * size;
	}
}
