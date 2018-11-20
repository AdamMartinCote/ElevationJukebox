#include <math.h>
#include <thread>
#include <future>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <utility>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "RestApi.hpp"
#include "database/Database.hpp"
#include "misc/Base64.hpp"
#include "misc/id_utils.hpp"
#include "mp3/header/Mp3Header.hpp"
#include "exception/MissingTokenException.hpp"
#include "exception/InvalidTokenException.hpp"

using namespace elevation;
using namespace std::placeholders;
namespace fs = std::experimental::filesystem;

RestApi::RestApi(Address addr, Logger& logger, FileCache& cache, Mp3EventClientSocket playerEventSocket)
: m_httpEndpoint(std::make_shared<Http::Endpoint>(addr))
, m_desc("Rest API", "1.0")
, m_logger(logger)
, m_cache(cache)
, m_playerEventSocket(std::move(playerEventSocket))
{
    Database::instance();
}

RestApi::~RestApi() {
    try {
        m_httpEndpoint->shutdown();
    }
    catch (std::runtime_error& e) { } // Pistache seems to have an issue where calling shutdown throws an exception.
}

void RestApi::init() {
    auto opts = Http::Endpoint::options();
    m_httpEndpoint->init(opts);
    createDescription_();
}

void RestApi::start() {
    m_router.initFromDescription(m_desc);

    Rest::Swagger swagger(m_desc);
    swagger
            .install(m_router);

    m_httpEndpoint->setHandler(m_router.handler());
    m_httpEndpoint->serve();
}

void RestApi::createDescription_() {
    m_desc
            .info()
            .license("Apache", "http://www.apache.org/licenses/LICENSE-2.0");

    auto backendErrorResponse =
            m_desc.response(Http::Code::Internal_Server_Error, "An error occurred with the backend");

    m_desc
            .schemes(Rest::Scheme::Http)
            .produces(MIME(Application, Json))
            .consumes(MIME(Application, Json));

    auto usagerPath = m_desc.path("/usager");

    usagerPath
            .route(m_desc.post("/identification"))
            .bind(&RestApi::getIdentification_, this)
            .hide();

    usagerPath
            .route(m_desc.get("/file/:id"))
            .bind(&RestApi::getFileList_, this)
            .hide();

    usagerPath
            .route(m_desc.post("/chanson/:id"))
            .bind(&RestApi::postFile_, this)
            .hide();

    usagerPath
            .route(m_desc.del("/chanson/:id/:no"))
            .bind(&RestApi::deleteFile_, this)
            .hide();
}

std::string generateBody(uint32_t id, std::string message) {
    rapidjson::Document idDoc;
    idDoc.SetObject();
    idDoc.AddMember("identificateur", id, idDoc.GetAllocator());
    idDoc.AddMember("message", rapidjson::Value(message.c_str(), message.length()), idDoc.GetAllocator());

    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    idDoc.Accept(writer);
    return buf.GetString();
}

void RestApi::getIdentification_(const Rest::Request& request, Http::ResponseWriter response) {

    std::string passwordHash = elevation::id_utils::generateMd5Hash("password123", "abc");
    std::cout << "passwordHash= " << passwordHash << std::endl;
    auto body = request.body();
    rapidjson::Document request_json;
    request_json.Parse(body.c_str());
    std::ostringstream logMsg;

    m_logger.log("getIdentification_ called");

    if ((!request_json.IsObject()
            || (!request_json.HasMember("mac")
                || !request_json.HasMember("ip")
                || request_json["mac"] == '\n'
                || request_json["ip"] == '\n'))) {
        logMsg << "Could not login the user. The request is malformed.";
        m_logger.err(logMsg.str());
        response.send(Http::Code::Bad_Request, "Malformed request");
        return;
    }
    std::thread([&](rapidjson::Document request_json, Http::ResponseWriter response, std::ostringstream logMsg) {
        User_t requestUser = { 0 };
        if (request_json.IsObject()) {
            strncpy(requestUser.mac, request_json["mac"].GetString(), User_t::MAC_LENGTH);
            strncpy(requestUser.ip, request_json["ip"].GetString(), User_t::IP_LENGTH);
            if (request_json.HasMember("nom")) {
                strncpy(requestUser.name, request_json["nom"].GetString(), User_t::NAME_LENGTH);
            }
        }

        User_t existingUser = { 0 };
        Database* db = Database::instance();
        existingUser = db->getUserByMac(requestUser.mac);
        if (*existingUser.mac == 0) {
            std::string salt = id_utils::generateSalt(strlen(requestUser.mac));
            requestUser.userId = id_utils::generateId(requestUser.mac, salt);

            db->createUser(&requestUser);
            db->connectUser(&requestUser);

            logMsg << '{' << requestUser.mac << '}' << " Assigned token \"" << requestUser.userId << "\" to user \"" << requestUser.name << "\"";
            m_logger.log(logMsg.str());

            std::string body = generateBody(requestUser.userId, "connection successful");
            response.send(Http::Code::Ok, body);
            return;
        } else {
            requestUser.userId = existingUser.userId;
            try {
                db->createUser(&requestUser);

                logMsg << '{' << requestUser.mac << '}' << " Assigned token \"" << requestUser.userId << "\" to user \"" << requestUser.name << "\"";
                m_logger.log(logMsg.str());

                std::string body = generateBody(requestUser.userId, "connection successful");
                response.send(Http::Code::Ok, body);
                return;
            } catch (sqlite_error& e) {
                logMsg << "couldn't create user in db: " << e.what();
                m_logger.err(logMsg.str());
                response.send(Http::Code::Internal_Server_Error, logMsg.str());
                return;
            }
        }
    }, std::move(request_json), std::move(response), std::move(logMsg)).detach();
}

void RestApi::getFileList_(const Rest::Request& request, Http::ResponseWriter response) {
    // querying a param from the request object, by name
    std::thread([this](const Rest::Request& request, Http::ResponseWriter response) {
        try {
            User_t requestUser = getUserFromRequestToken_(request);
            std::string serializedList = generateAllSongsAsViewedBy_(requestUser.userId);

            std::ostringstream logMsg;
            logMsg << "The file list for user \"" << requestUser.userId << "\" was successfuly sent.";
            m_logger.log(logMsg.str());
            response.send(Http::Code::Ok, serializedList);
        }
        catch (const std::exception& e) {
            std::ostringstream logMsg;
            logMsg << "An error occurred: " << e.what();
            m_logger.err(logMsg.str());
            response.send(Pistache::Http::Code::Forbidden, e.what());
            return;
        }
    }, std::move(request), std::move(response)).detach();
}

void RestApi::postFile_(const Rest::Request& request, Http::ResponseWriter response) {
    m_logger.log("postFile called");

    User_t requestUser = {};
    try {
        requestUser = getUserFromRequestToken_(request);
    }
    catch (const MissingTokenException& e) {
        std::ostringstream logMsg;
        logMsg << "Could not post the file. Received invalid token.";
        m_logger.err(e.what());
        response.send(Http::Code::Bad_Request, e.what());
        return;
    }
    catch (const InvalidTokenException& e) {
        m_logger.err(e.what());
        response.send(Http::Code::Forbidden, e.what());
        return;
    }

    if (!m_cache.isInitialized()) {
        std::ostringstream logMsg;
        logMsg << "Could not post the file. Cache is not initialized";
        m_logger.err(logMsg.str());
        response.send(Http::Code::Internal_Server_Error, "Cache not initialized");
        return;
    }

    const std::string body = request.body();

    std::thread([this, requestUser](const std::string body, Http::ResponseWriter response) {
        Mp3Header* header = nullptr;
        try {
            Database* db = Database::instance();

            std::stringstream decoded;
            // Start the decoding immediately
            auto decodedFuture = std::async(std::launch::async, [&](){
                // Decode the string.
                std::istringstream encoded(body);
                Base64::Decode(encoded, decoded);
                return;
            });

            // Start fetching the user's songs
            auto songsFuture = std::async(std::launch::async, std::bind(&Database::getSongsByUser, db, _1), requestUser.userId);

            // +=-=-=-= Save in a file =-=-=-=+
            // Generate
            std::time_t now = std::time(nullptr);
            std::tm* nowTm = std::localtime(&now);
            std::stringstream fileName;
            // Generate a new name
            fileName << std::put_time(nowTm, "%Y-%m-%d_%H-%M-%S_") << std::hash<uint32_t>()(requestUser.userId);
            std::experimental::filesystem::path filePath(fileName.str());
            std::experimental::filesystem::path tmpPath = filePath;
            // Try a new file name until we find on that is not used.
            size_t counter = 0;
            while (m_cache.isFileCached(tmpPath)) {
                tmpPath = filePath;
                tmpPath += "_" + std::to_string(++counter);
            }
            tmpPath += ".mp3";
            filePath = tmpPath;

            decodedFuture.get(); // Wait for the decoding to finish
            m_cache.setFileContent(filePath, decoded);
            filePath = m_cache.getFile(filePath).path(); // resolve the filename into a real path

            // Fetch MP3 header
            try {
                header = new Mp3Header(filePath.string());
            }
            catch (std::invalid_argument& e) {
                std::ostringstream logMsg;
                logMsg << "Could not post the file. The file does not have an id3v2 tag.";
                m_logger.err(logMsg.str());
                response.send(Http::Code::Unsupported_Media_Type, "The file is not an MP3 file");
                m_cache.deleteFile(filePath.filename());
                delete header;
                return;
            }

            // Put the song's path in the DB.
            Song_t song;
            strncpy(song.title, header->getTitle().c_str(), Song_t::TITLE_LENGTH);
            strncpy(song.artist, header->getArtist().c_str(), Song_t::ARTIST_LENGTH);
            song.userId = requestUser.userId;
            song.duration = header->getDuration().getDurationInSeconds();
            strncpy(song.path, filePath.string().c_str(), Song_t::PATH_LENGTH);

            const auto& songs = songsFuture.get();
            bool songInQueue = std::any_of(songs.cbegin(), songs.cend(), [&](const Song_t& a) {return strcmp(a.title, song.title) == 0;});
            if (songInQueue || songs.size() >= MAX_SONG_PER_USER) {
                std::ostringstream logMsg;
                logMsg << "Could not post the file. The song \"" << song.title << "\" is aleady in the queue";
                m_logger.err(logMsg.str());
                response.send(Http::Code::Request_Entity_Too_Large, "Song already in the queue");
                m_cache.deleteFile(filePath.filename());
                delete header;
                return;
            }
            else {
                std::ostringstream logMsg;
                logMsg << '{' << requestUser.mac << '}' << " Sent MP3 \"" << song.title << "\" of length " << song.duration;
                m_logger.log(logMsg.str());

                db->createSong(&song);

                response.send(Http::Code::Ok, "Ok"); // We send the response at the end in the case there is an error in the process.

                delete header;
                return;
            }
        }
        catch (std::runtime_error &e) {
            std::ostringstream logMsg;
            logMsg << "An error occured while saving the song : " << e.what();
            m_logger.err(logMsg.str());
            response.send(Http::Code::Internal_Server_Error, e.what());
            delete header;
            return;
        }
        response.send(Http::Code::Internal_Server_Error, "Request terminated without an answer...");
    }, std::move(body), std::move(response)).detach();
}

void RestApi::deleteFile_(const Rest::Request& request, Http::ResponseWriter response) {
    std::async(
        std::launch::async,
        [this](const Rest::Request& request, Http::ResponseWriter response) {
            Database* db = Database::instance();
            uint32_t songId;
            try {
                songId = request.param(":no").as<uint32_t>();
            }
            catch (const std::runtime_error& e) {
                response.send(Pistache::Http::Code::Bad_Request, e.what());
            }

            try {
                User_t requestUser = getUserFromRequestToken_(request);

                Song_t song =
                    db->getSongById(songId);

                if (song.id != 0 && song.userId == requestUser.userId) {
                    db->removeSong(songId);
                    m_cache.deleteFile(song.path);

                    std::ostringstream logMessage;
                    logMessage << '{' << requestUser.mac << '}' << " Removed MP3 \"" << song.title << "\" of length " << song.duration;
                    m_logger.log(logMessage.str());
                    response.send(Pistache::Http::Code::Ok);
                }
                else {
                    std::ostringstream logMessage;
                    if (song.id == 0) {
                        logMessage << '{' << requestUser.mac << '}' << " tried to remove nonexistant song of id " << songId;
                    }
                    else {
                        logMessage << '{' << requestUser.mac << '}' << " tried to remove song \"" << song.title << "\" of user " << song.userId;
                    }
                    m_logger.err(logMessage.str());
                    response.send(Pistache::Http::Code::Method_Not_Allowed);
                }
            }
            catch (const std::exception& e) {
                response.send(Pistache::Http::Code::Forbidden, e.what());
                return;
            }
        },
        request,
        std::move(response)
    );
}

std::string RestApi::generateAllSongsAsViewedBy_(uint32_t token, bool adminSerialization) {
    std::vector<Song_t> songs = Database::instance()->getAllSongs();
    rapidjson::Document songsDoc;
    songsDoc.SetObject();
    rapidjson::Value songsArray;
    songsArray.SetArray();
    for (auto& song : songs) {
        songsArray.PushBack(generateSong_(song, token, songsDoc.GetAllocator(), adminSerialization), songsDoc.GetAllocator());
    }
    songsDoc.AddMember(rapidjson::Value().SetString("chansons"), songsArray, songsDoc.GetAllocator());

    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    songsDoc.Accept(writer);

    return buf.GetString();
}

rapidjson::Value& RestApi::generateSong_(const Song_t& song, uint32_t token, rapidjson::Document::AllocatorType& allocator, bool adminSerialization) {
    rapidjson::Value songDoc;
    songDoc.SetObject();
    try {
        User_t user = Database::instance()->getUserById(song.userId);
        Mp3Duration d(song.duration);
        std::stringstream duration;
        duration << std::setfill('0') << std::setw(2);
        duration << d.getMinutes() << ':' << d.getSeconds();
        songDoc.AddMember(rapidjson::StringRef("titre"), rapidjson::Value(song.title, strlen(song.title)), allocator);
        songDoc.AddMember(rapidjson::StringRef("artiste"), rapidjson::Value(song.artist, strlen(song.artist)), allocator);
        songDoc.AddMember("duree", rapidjson::Value(duration.str().c_str(), duration.str().length()), allocator);
        songDoc.AddMember("proposeePar", rapidjson::Value(user.name, strlen(user.name)), allocator);
        songDoc.AddMember("proprietaire", token == song.userId ? true : false, allocator);
        songDoc.AddMember("no", song.id, allocator);

        if (adminSerialization) {
            Database* db = Database::instance();
            User_t owner = db->getUserById(song.userId);
            songDoc.AddMember("mac", rapidjson::Value(owner.mac, strlen(owner.mac)), allocator);
            songDoc.AddMember("ip" , rapidjson::Value(owner.ip , strlen(owner.ip )), allocator);
            songDoc.AddMember("id" , owner.userId, allocator);
        }
    }
    catch (sqlite_error& e) {
        std::stringstream msg;
        msg << "An error occured while generating song a song's json: " << e.what();
        m_logger.err(msg.str());
    }
    return songDoc.Move();
}

User_t RestApi::getUserFromRequestToken_(const Rest::Request& request) {
    auto t = request.headers().getRaw("X-Auth-Token");
    if (t.value().empty()) {
        throw MissingTokenException{};
    }

    uint32_t token = std::stoul(t.value());
    User_t requestUser = {0};
    try {
        requestUser = Database::instance()->getUserById(token);
    } catch (sqlite_error& e) { }

    if (token == 0 || *requestUser.mac == 0) {
        throw InvalidTokenException{t.value()};
    }

    return requestUser;
}

