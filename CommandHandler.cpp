﻿#include "CommandHandler.h"
#include <dpp/dispatcher.h>
#include "curl/curl.h"
#include "Client.h"
#include "Common.h"
#include "AuthenticationValues.h"
#include "Enums/ServerType.h"
#include "NotPhotonListener.h"
#include <regex>
#include "Helpers.h"
#include "Command.h"
#include "UrbanDictionaryJson.h"
#include "DiscordBotStuff.h"
#include <stdio.h>
#include <iostream>
#include <random>
#include "Logger.h"
#include "Logger.h"
#include "nlohmann/json.hpp"

size_t CommandHandler::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

std::string CommandHandler::GetResponse(std::string Url)
{
	auto curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, Url.c_str());
		std::string response_string;
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
		curl_easy_perform(curl);
		//std::cout << response_string << std::endl;

		curl_easy_cleanup(curl);
		curl_global_cleanup();
		curl = NULL;

		return response_string;
	}
	else
	{
		Logger::LogError("No curl");
	}
	return "";
}

NotPhotonListener* CommandHandler::GetBot(const dpp::message_create_t& event, bool OwnerOnly)
{
	for (int i = 0; StoreVector.size() > i; i++)
	{
		auto BotChannelId = StoreVector[i]->ChannelId;
		auto ChannelId = event.msg.channel_id;
		if (BotChannelId != ChannelId) continue;

		if (!OwnerOnly)
		{
			return StoreVector[i];
		}
		if (StoreVector[i]->CreatorId == event.msg.author.id)
		{
			return StoreVector[i];
		}
	}
	return nullptr;
}

void CommandHandler::RemoveBot(NotPhotonListener* OldBot)
{
	for (int i = 0; StoreVector.size() > i; i++)
	{
		if (StoreVector[i] == OldBot)
		{
			StoreVector[i] = NULL;
			StoreVector.erase(StoreVector.begin() + i);
		}
	}
}

void CommandHandler::HandleChat()
{
	while (true)
	{
		for (int i = 0; StoreVector.size() > i; i++)
		{
			auto Bot = StoreVector[i];
			if (Bot->Client.getState() != 15 || Bot->Chat == "") continue;

			if (Bot->Chat.size() > 2000)
				Bot->Chat = "Message was over 2k Characters, it got ate";
			dpp::message msg(Bot->ChannelId, Bot->Chat);
			msg.set_guild_id(Bot->GuildId);
			Bot->Chat = "";
			try
			{
				DiscordBotStuff::SendMsg(msg);
			}
			catch (std::exception ex)
			{
				Logger::LogError("Message error AoTTG -> Discord: " + std::string(ex.what()));
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
	}
}

bool CommandHandler::SameVoiceChat(const dpp::message_create_t& event)
{
	dpp::guild* g = dpp::find_guild(event.msg.guild_id);

	if (!g) return false;
	auto current_vc = event.from->get_voice(event.msg.guild_id);
	if (!current_vc) return false;

	for (auto& i : g->voice_members)
	{
		if (event.msg.author.id == i.second.user_id)
			return current_vc->channel_id == i.second.channel_id;
	}
	return false;
}

void CommandHandler::Init()
{
	std::thread(HandleChat).detach();
}

void CommandHandler::UrbanDictionary(const dpp::message_create_t& event, std::string args)
{
	std::string Url = "https://api.urbandictionary.com/v0/define?term=" + args;
	std::string response_string = GetResponse(Url);
	json json = json::parse(response_string);

	UrbanDictionaryJson ud(json["list"]);
	if (ud.Definitions.empty())
	{
		event.send("No definition found");
		return;
	}
	Definitions = ud;
	event.send(ud.Definitions[0] + "\t1" + "/" + std::to_string(ud.Definitions.size()));
}

void CommandHandler::Ahegao(const dpp::message_create_t& event, std::string args)
{
	std::string Url = "https://g.tenor.com/v1/search?q=ahegao&media_filter=minimal&key=1D4ZQ37D7W46&limit=1&pos=" + std::to_string(1 + (rand() % 100));

	std::string response_string = GetResponse(Url);
	json j = json::parse(response_string);

	std::string result;
	result = j["results"][0]["media"][0]["gif"]["url"];
	event.send(result);
}

void CommandHandler::Meow(const dpp::message_create_t& event, std::string args)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, 100);
	int random = dis(gen);

	std::cout << random << std::endl;

	std::string Url = "https://g.tenor.com/v1/search?q=meow&media_filter=minimal&key=1D4ZQ37D7W46&limit=1&pos=" + std::to_string(random);

	std::string response_string = GetResponse(Url);
	json j = json::parse(response_string);

	std::string result;
	result = j["results"][0]["media"][0]["gif"]["url"];
	event.send(result);
}

void CommandHandler::CreateRoom(const dpp::message_create_t& event, std::string args)
{
	auto Bot = GetBot(event, true);
	if (Bot == nullptr) return;

	ExitGames::LoadBalancing::RoomOptions roomoptions;
	roomoptions.setLobbyType(NULL);
	roomoptions.setMaxPlayers(20);

	auto props = Common::Hashtable();

	//auto a = Common::JString(std::string(9999, 'T').c_str());

	props.put("Private", "This Room is AoTTG2 password protected");
	props.put("password", "test");

	roomoptions.setCustomRoomProperties(props);
	ExitGames::Common::JVector<Common::JString> ListedProps;
	ListedProps.addElement("Private");

	roomoptions.setPropsListedInLobby(ListedProps);

	Bot->Client.opCreateRoom("FoodForTesting`The Forest III`normal`999999`day``205349876", roomoptions);
}

void CommandHandler::Test(const dpp::message_create_t& event, std::string args)
{
	if (!SameVoiceChat(event)) return;
	if (MusicPlayers.count(event.msg.guild_id) == 0) return;
	MusicPlayers[event.msg.guild_id]->PlaybackDelay = std::stod(args);

	return;

	

	std::vector<std::string> TestPages;
	TestPages.push_back("Page 1");
	TestPages.push_back("Page 2");

	DiscordBotStuff::DiscordBot->message_create(
		dpp::message(event.msg.channel_id, "this text has buttons").add_component(
			dpp::component().add_component(
				dpp::component().set_label("Back").
				set_type(dpp::cot_button).
				set_emoji(u8"🤡").
				set_id("back")
			).add_component(
				dpp::component().set_label("Next").
				set_type(dpp::cot_button).
				set_emoji(":AlcoholicPepe:", 875055062756515840).
				set_id("next")
			)//<:AlcoholicPepe:875055062756515840>
		)
	);
}

void CommandHandler::Debug(const dpp::message_create_t& event, std::string args)
{
	auto Bot = GetBot(event);
	if (Bot == nullptr) return;
	std::stringstream List;
	auto te = ClientStates[Bot->Client.getState()];
	//	auto test = ExitGames::LoadBalancing::PeerStates::(8);
	List << "```\nCreator:		" << Bot->CreatorId << "\n";
	List << "Channel:			" << Bot->ChannelId << "\n";
	List << "State:				" << te << " " + std::to_string(Bot->Client.getState()) << "\n";
	List << "Ping:				" << Bot->Client.getRoundTripTime() << "\n";
	List << "Ping variance:		" << Bot->Client.getRoundTripTimeVariance() << "```";

	event.send(List.str());
}

void CommandHandler::PlayerList(const dpp::message_create_t& event, std::string args)
{
	auto Bot = GetBot(event);
	if (Bot == nullptr) return;
	if (Bot->Client.getState() != 15) return;

	std::stringstream List;

	List << "```";
	//std::string List = "```";
	auto RoomName = Helpers::Split(std::regex_replace(Bot->Client.getCurrentlyJoinedRoom().getName().UTF8Representation().cstr(), std::regex("\\[[a-zA-Z0-9\]{6}\\]"), ""), '`');
	List << "Roomname: " + RoomName[0] + "\n";
	List << "Current Playercount: " + std::to_string(Bot->Client.getCurrentlyJoinedRoom().getPlayerCount()) + "/" + std::to_string(Bot->Client.getCurrentlyJoinedRoom().getMaxPlayers()) + "\n";
	List << "Room: " + RoomName[1] + "\n";

	for (int i = 0; Bot->Client.getCurrentlyJoinedRoom().getPlayers().getSize() > i; i++)
	{
		auto player = Bot->Client.getCurrentlyJoinedRoom().getPlayers()[i];
		auto props = player->getCustomProperties();

		List << (props.contains("NoodleDoodle") ? "[B]" : player->getIsMasterClient() ? "[M]" : "[P]");

		if (props.contains("isTitan"))
		{
			bool IsTitan = Common::ValueObject<int>(props.getValue("isTitan")).getDataCopy() == 2;
			List << (IsTitan ? "[T]" : "[H]");
		}
		else
		{
			List << "[H]";
		}
		List << " [";
		auto BiggestId = std::to_string(Bot->Client.getCurrentlyJoinedRoom().getPlayers().getLastElement()->getNumber());
		List << std::string(BiggestId.size() - std::to_string(player->getNumber()).size(), '0');
		List << std::to_string(player->getNumber()) + "] ";

		std::string Name;
		//name
		if (props.contains("name"))
		{
			Name = std::regex_replace(Common::ValueObject<Common::JString>(props.getValue("name")).getDataCopy().UTF8Representation().cstr(), std::regex("\\[[a-zA-Z0-9\]{6}\\]"), "");
		}
		else
		{
			Name = "Nameless";
		}
		if (Name.size() > 20)
			Name.erase(20, Name.size());

		List << Name + std::string(23 - Name.size(), ' ');

		std::string Stats;
		if (props.contains("kills"))
		{
			Stats += std::to_string(Common::ValueObject<int>(props.getValue("kills")).getDataCopy()) + "/";
		}
		else
		{
			Stats += "-1/";
		}
		if (props.contains("deaths"))
		{
			Stats += std::to_string(Common::ValueObject<int>(props.getValue("deaths")).getDataCopy()) + "/";
		}
		else
		{
			Stats += "-1/";
		}
		if (props.contains("max_dmg"))
		{
			Stats += std::to_string(Common::ValueObject<int>(props.getValue("max_dmg")).getDataCopy()) + "/";
		}
		else
		{
			Stats += "-1/";
		}
		if (props.contains("total_dmg"))
		{
			Stats += std::to_string(Common::ValueObject<int>(props.getValue("total_dmg")).getDataCopy());
		}
		else
		{
			Stats += "-1";
		}

		List << Stats + std::string(20 - Stats.size(), ' ');

		if (props.contains("dead"))
		{
			bool IsDead = Common::ValueObject<bool>(props.getValue("dead")).getDataCopy();
			List << (IsDead ? "Dead" : "Alive");
		}
		else
		{
			List << "Dead";
		}

		List << "\n";
	}
	List << "```";

	event.send(List.str());
	//Bot->Chat += List.str();
}

void CommandHandler::Join(const dpp::message_create_t& event, std::string args)
{
	auto Bot = GetBot(event, true);
	if (Bot == nullptr) return;
	args = Helpers::ToLower(args);

	LoadBalancing::Room* Target = NULL;
	for (int i = 0; Bot->Client.getRoomList().getSize() > i; i++)
	{
		LoadBalancing::Room* room = Bot->Client.getRoomList()[i];
		std::string Name = Helpers::ToLower(std::regex_replace(room->getName().UTF8Representation().cstr(), std::regex("\\[[a-zA-Z0-9\]{6}\\]"), ""));

		if (Name.find(args) != std::string::npos)
		{
			//Logger::LogDebug("Join Found a room");
			Target = room;
			break;
		}
	}

	if (Target != NULL)
	{
		if (Target->getMaxPlayers() != 0 && Target->getMaxPlayers() == Target->getPlayerCount())
		{
			event.reply("Room is full");
			return;
		}
		for (int i = 0; Target->getCustomProperties().getSize() > i; i++)
		{
			auto& prop = Target->getCustomProperties().getKeys()[i];
			if (std::string(prop.toString().UTF8Representation().cstr()) == "\"Private\"")
			{
				event.reply("Use the nonexistent join method to join AoTTG2 passworded rooms");
				return;
			}
		}
		Bot->Client.opJoinRoom(Target->getName(), false, 0, Common::JVector<Common::JString>());
		bool createChannel = false;
		auto guild = dpp::find_guild(event.msg.guild_id);
		dpp::snowflake parentId;
		for (int i = 0; guild->channels.size() > i; i++)
		{
			auto channel = dpp::find_channel(guild->channels[i]);

			if (channel == NULL) continue;
			if ((Helpers::ToLower(channel->name) == "aottg rooms" || Helpers::ToLower(channel->name) == "aottg room") && channel->is_category())
			{
				createChannel = true;
				parentId = channel->id;
				break;
			}
		}

		if (!createChannel) return;

		auto channel = dpp::channel();
		channel.guild_id = event.msg.guild_id;
		channel.name = Helpers::TrimStartEnd(Helpers::Split(std::regex_replace(Target->getName().UTF8Representation().cstr(), std::regex("\\[[a-zA-Z0-9\]{6}\\]"), ""), '`')[0]);
		channel.set_parent_id(parentId);

		DiscordBotStuff().DiscordBot->channel_create(channel, [Bot](const dpp::confirmation_callback_t& event)
			{
				if (event.is_error())
				{
					Logger::LogError(event.get_error().message);
					return;
				}
				Bot->ChannelId = event.get<dpp::channel>().id;
				Bot->CreatedChannel = true;
			});
	}
}

void CommandHandler::List(const dpp::message_create_t& event, std::string args)
{
	auto Bot = GetBot(event);
	if (Bot == nullptr) return;
	std::stringstream List;

	for (int i = 0; Bot->Client.getRoomList().getSize() > i; i++)
	{
		LoadBalancing::Room* room = Bot->Client.getRoomList()[i];

		auto cleanname = std::regex_replace(room->getName().ANSIRepresentation().cstr(), std::regex("\\[[a-zA-Z0-9\]{6}\\]"), "");
		auto splitted = Helpers::Split(cleanname, '`');

		std::string RoomInfo;

		std::string RoomName = splitted[0];
		if (RoomName.size() > 26)
			RoomName = RoomName.substr(0, 23) + "...";

		RoomInfo += RoomName + std::string(27 - RoomName.size(), ' ') + " | ";

		RoomInfo += splitted[1] + std::string(21 - splitted[1].size(), ' ') + " | ";

		std::string PlayerCount = std::to_string(room->getPlayerCount());
		std::string MaxPlayerCount = std::to_string(room->getMaxPlayers());

		RoomInfo += PlayerCount + std::string(4 - PlayerCount.size(), ' ') + "/";
		RoomInfo += MaxPlayerCount + std::string(4 - MaxPlayerCount.size(), ' ') + "|";
		RoomInfo += room->getIsOpen() ? "" : "CLOSED";
		RoomInfo += splitted[5] == "" ? "" : R"([PWD])";
		if (room->getCustomProperties().getSize() > 0)
		{
			RoomInfo += " { ";
			for (int o = 0; room->getCustomProperties().getSize() > o; o++)
			{
				auto propname = room->getCustomProperties().getKeys()[o].toString().UTF8Representation().cstr();
				auto propvalue = room->getCustomProperties()[o].toString().UTF8Representation().cstr();
				RoomInfo += std::string(propname) + "," + std::string(propvalue) + " ";
			}
			RoomInfo += "} ";
		}

		List << RoomInfo << "\n";
	}

	event.send("```" + List.str() + "```");
}

void CommandHandler::Disconnect(const dpp::message_create_t& event, std::string args)
{
	auto Bot = GetBot(event, true);
	if (Bot == nullptr) return;

	Bot->Client.disconnect();
	Bot->KeepRunning = false;
	//Bot->~NotPhotonListener();
	RemoveBot(Bot);
	//delete Bot;

	event.send(Bot->CreatedChannel ? "Channel will get deleted in 5 seconds" : "Disconnected");
	if (!Bot->CreatedChannel)		return;

	std::this_thread::sleep_for(std::chrono::seconds(5));

	DiscordBotStuff().DiscordBot->channel_delete(event.msg.channel_id, [](const dpp::confirmation_callback_t& event)
		{
			if (event.is_error())
			{
				Logger::LogError(event.get_error().message);
			}
		});
}

void CommandHandler::Start(const dpp::message_create_t& event, std::string args)
{
	auto OldBot = GetBot(event);
	if (OldBot != nullptr)
	{
		if (OldBot->CreatorId == event.msg.author.id && OldBot->Client.getState() != 15); // If creator called -s again and bot is not inside a Room
		{
			//remove(StoreVector.begin(), StoreVector.end(), OldBot);
			OldBot->KeepRunning = false;
			//OldBot->~NotPhotonListener();
			RemoveBot(OldBot);
			goto Continue;
		}
		event.reply("Active Bot in Channel");
		return;
	}
Continue:
	if (args.size() == 0)
		args = "eu";

	Common::JString Ip = "";
	args = Helpers::ToLower(args);
	if (args == "eu")
		Ip = "135.125.239.180";
	else if (args == "us")
		Ip = "142.44.242.29";
	else if (args == "asia")
		Ip = "51.79.164.137";
	else if (args == "sa")
		Ip = "172.107.193.233";
	if (Ip == "")
	{
		event.send("Enter a valid Region!");
		return;
	}

	NotPhotonListener* Bot = new NotPhotonListener("");
	Bot->CreatorId = event.msg.author.id;
	Bot->GuildId = event.msg.guild_id;
	Bot->ChannelId = event.msg.channel_id;

	/*ExitGames::LoadBalancing::ConnectOptions options2(ExitGames::LoadBalancing::AuthenticationValues().setUserID("notsocrusty"), "crustycunt");

	Bot->Client.connect(options2);*/

	std::string name = std::string("notsocrusty") + std::to_string(1 + (rand() % 1000));
	ExitGames::LoadBalancing::ConnectOptions options(ExitGames::LoadBalancing::AuthenticationValues().setUserID(name.c_str()), "crustycunty", Ip, ExitGames::LoadBalancing::ServerType::MASTER_SERVER);
	Bot->Client.connect(options);

	std::string InGameName = "NoodleDoodleTesting" + std::to_string(1 + (rand() % 1000));
	Bot->Client.getLocalPlayer().addCustomProperty("name", InGameName.c_str());
	Bot->Client.getLocalPlayer().addCustomProperty("dead", true);
	Bot->Client.getLocalPlayer().addCustomProperty("kills", 0);
	Bot->Client.getLocalPlayer().addCustomProperty("deaths", -1);
	Bot->Client.getLocalPlayer().addCustomProperty("max_dmg", 0);
	Bot->Client.getLocalPlayer().addCustomProperty("total_dmg", 0);
	Bot->Client.getLocalPlayer().addCustomProperty("RCteam", 1);
	Bot->Client.getLocalPlayer().addCustomProperty("NoodleDoodle", "I'm a Discord Bot");

	StoreVector.push_back(Bot);
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	if (Bot->Client.getState() != LoadBalancing::PeerStates::JoinedLobby)
	{
		event.reply("Failed to join Server, try again");
		//Debug(event);
		Disconnect(event);
		return;
	}
	/*for (int i = 0; Bot->Client.getRoomList().getSize() > i; i++)
	{
		auto room = Bot->Client.getRoomList()[i];
		for (int o = 0; room->getCustomProperties().getSize() > o; o++)
		{
			std::cout << room->getCustomProperties()[o].toString().UTF8Representation().cstr() << std::endl;
		}
	}*/

	List(event);
}

void CommandHandler::Next(const dpp::message_create_t& event, std::string args)
{
	if (Definitions.Definitions.empty()) return;

	auto definition = Definitions.GetNext();

	if (definition == "") return;

	event.send(definition);
}

void CommandHandler::Back(const dpp::message_create_t& event, std::string args)
{
	if (Definitions.Definitions.empty()) return;

	auto definition = Definitions.GetPrevious();

	if (definition == "") return;

	event.send(definition);
}

void CommandHandler::JoinVC(const dpp::message_create_t& event, std::string args)
{
	dpp::guild* g = dpp::find_guild(event.msg.guild_id);
	auto current_vc = event.from->get_voice(event.msg.guild_id);
	bool join_vc = true;
	/* Check if we are currently on any vc */
	if (current_vc)
	{
		/* Find the channel id  that the user is currently on */
		auto users_vc = g->voice_members.find(event.msg.author.id);
		/* See if we currently share a channel with the user */
		if (users_vc != g->voice_members.end() && current_vc->channel_id == users_vc->second.channel_id)
		{
			join_vc = false;
			/* We are on this voice channel, at this point we can send any audio instantly to vc:

			 * current_vc->send_audio_raw(...)
			 */
		}
		else
		{
			/* We are on a different voice channel. Leave it, then join the new one
			 * by falling through to the join_vc branch below.
			 */
			 /*event.from->disconnect_voice(event.msg.guild_id);
			 join_vc = true;*/
			DiscordBotStuff::DiscordBot->message_create(dpp::message(event.msg.channel_id, "I'm already in a VoiceChannel"));
			return;
		}
	}
	/* If we need to join a vc at all, join it here if join_vc == true */
	if (join_vc)
	{
		if (!g->connect_member_voice(event.msg.author.id))
		{
			/* The user issuing the command is not on any voice channel, we can't do anything */
			DiscordBotStuff::DiscordBot->message_create(dpp::message(event.msg.channel_id, "You don't seem to be on a voice channel! :("));
		}
		else
		{
			/* We are now connecting to a vc. Wait for on_voice_ready
			 * event, and then send the audio within that event:
			 *
			 * event.voice_client->send_audio_raw(...);
			 *
			 * NOTE: We can't instantly send audio, as we have to wait for
			 * the connection to the voice server to be established!
			 */
		}
	}
}

void CommandHandler::LeaveVC(const dpp::message_create_t& event, std::string args)
{
	if (SameVoiceChat(event))
	{
		auto current_vc = event.from->get_voice(event.msg.guild_id);
		current_vc->voiceclient->stop_audio();
		event.from->disconnect_voice(event.msg.guild_id);
		if (MusicPlayers.count(event.msg.guild_id) != 0)
		{
			MusicPlayers[event.msg.guild_id]->Stop();
			delete MusicPlayers[event.msg.guild_id];
			MusicPlayers.erase(event.msg.guild_id);
		}
	}
}

#include <Windows.h>
void CommandHandler::Play(const dpp::message_create_t& event, std::string args)
{
	if (!SameVoiceChat(event)) return;

	dpp::voiceconn* v = event.from->get_voice(event.msg.guild_id);
	if (!v)
	{
		event.reply("I'm not inside a VoiceChat yet");
		return;
	}
	if (args.find("youtu") == std::string::npos || args.find(" ") != std::string::npos)
	{
		event.reply("Invalid URL ||whitespaces are not allowed||");
		return;
	}
	if (MusicPlayers.count(event.msg.guild_id) == 0)
	{
		MusicPlayers[event.msg.guild_id] = new PlaybackHandler(event.from, event.msg.guild_id);
	}

	MusicPlayers[event.msg.guild_id]->Add(args);
}

void CommandHandler::Skip(const dpp::message_create_t& event, std::string args)
{
	if (!SameVoiceChat(event)) return;

	if (MusicPlayers.count(event.msg.guild_id) == 0) return;
	MusicPlayers[event.msg.guild_id]->Skip();
	auto current_vc = event.from->get_voice(event.msg.guild_id);
	if (current_vc)
	{
		current_vc->voiceclient->stop_audio();
	}
}

void CommandHandler::Resume(const dpp::message_create_t& event, std::string args)
{
	if (!SameVoiceChat(event)) return;

	if (MusicPlayers.count(event.msg.guild_id) == 0) return;
	MusicPlayers[event.msg.guild_id]->Resume();
}

void CommandHandler::Pause(const dpp::message_create_t& event, std::string args)
{
	if (!SameVoiceChat(event)) return;

	if (MusicPlayers.count(event.msg.guild_id) == 0) return;
	MusicPlayers[event.msg.guild_id]->Pause();
}
void CommandHandler::Queue(const dpp::message_create_t& event, std::string args)
{
	if (MusicPlayers.count(event.msg.guild_id) == 0) return;
	std::string queue = MusicPlayers[event.msg.guild_id]->QueueString();

	try
	{
		//event.reply(MusicPlayers[event.msg.guild_id]->QueueString().c_str());
		//DiscordBotStuff::DiscordBot->message_create(dpp::message(event.msg.channel_id, MusicPlayers[event.msg.guild_id]->QueueString().c_str()));

		//DiscordBotStuff::SendMsg(dpp::message(event.msg.channel_id, "Im a Baka"));

		DiscordBotStuff::SendMsg(dpp::message(event.msg.channel_id, queue.data()));
	}
	catch (std::exception ex)
	{
		Logger::LogError("String was " + std::to_string(queue.size()) + " long");
		Logger::LogError(queue);
		Logger::LogError(ex.what());
	}
}

void CommandHandler::Remove(const dpp::message_create_t& event, std::string args)
{
	if (!SameVoiceChat(event)) return;
	if (MusicPlayers.count(event.msg.guild_id) == 0) return;

	try
	{
		int num = std::stoi(args);
		if (1 > num)
			return;
		if (num == 1)
		{
			event.reply("Cannot remove currently playing song");
			return;
		}
		if (args.find('-') == std::string::npos)
		{
			MusicPlayers[event.msg.guild_id]->Remove(num);
		}
		else
		{
			auto arg = Helpers::Split(args, '-');
			if (args.length() < 0)
			{
				event.reply("Specify start and end");
				return;
			}
			try
			{
				int start = std::stoi(arg[0]);
				int end = std::stoi(arg[1]);

				MusicPlayers[event.msg.guild_id]->Remove(start, end);
			}
			catch (std::exception ex)
			{
				Logger::LogError(ex.what());
			}
		}
	}
	catch (const std::invalid_argument& e) {
		// Conversion failed
		event.reply("Invalid input");
	}
}

void CommandHandler::Shuffle(const dpp::message_create_t& event, std::string args)
{
	if (MusicPlayers.count(event.msg.guild_id) == 0) return;
	MusicPlayers[event.msg.guild_id]->Shuffle();
	event.reply(u8"👌");
}

void CommandHandler::PlayNow(const dpp::message_create_t& event, std::string args)
{
	if (!SameVoiceChat(event)) return;

	dpp::voiceconn* v = event.from->get_voice(event.msg.guild_id);
	if (!v)
	{
		event.reply("I'm not inside a VoiceChat yet");
		return;
	}
	if (args.find("youtu") == std::string::npos || args.find(" ") != std::string::npos)
	{
		event.reply("Invalid URL ||whitespaces are not allowed||");
		return;
	}
	else if (args.find("list=") != std::string::npos)
	{
		event.reply("Don't use PlayNow to queue Playlists");
		return;
	}
	MusicPlayers[event.msg.guild_id]->PlayNow(args);
}