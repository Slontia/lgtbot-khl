#include <boost/python.hpp>
#include <boost/python/call.hpp>

#include "bot_core/bot_core.h"

#include <memory>
#include <thread>
#include <iostream>

#include <curl/curl.h>

void* g_bot_core = nullptr;
PyObject* g_get_user_name = nullptr;
PyObject* g_get_user_avatar_url = nullptr;
PyObject* g_send_text_message = nullptr;
PyObject* g_send_image_message = nullptr;

class AcquireGIL
{
  public:
    inline AcquireGIL() { state = PyGILState_Ensure(); }
    inline ~AcquireGIL() { PyGILState_Release(state); }

  private:
    PyGILState_STATE state;
};

class ReleaseGIL
{
  public:
    inline ReleaseGIL() { save_state = PyEval_SaveThread(); }
    inline ~ReleaseGIL() { PyEval_RestoreThread(save_state); }

  private:
    PyThreadState *save_state;
};

void HandleMessages(void* handler, const char* const id, const int is_uid, const LGTBot_Message* messages, const size_t size)
{
    std::string s;
    const auto flush = [&]()
        {
            if (s.empty()) {
                return;
            }
            try {
                AcquireGIL a;
                boost::python::call<void>(g_send_text_message, id, is_uid, s);
            } catch (...) {
                std::cerr << "flush message failed, message: " << s << std::endl;
            }
            s.clear();
        };
    for (size_t i = 0; i < size; ++i) {
        const auto& msg = messages[i];
        switch (msg.type_) {
        case LGTBOT_MSG_TEXT:
            s.append(msg.str_);
            break;
        case LGTBOT_MSG_USER_MENTION:
            s.append("(met)");
            s.append(msg.str_);
            s.append("(met)");
            break;
        case LGTBOT_MSG_IMAGE:
            flush();
            try {
                AcquireGIL a;
                boost::python::call<void>(g_send_image_message, id, is_uid, msg.str_);
            } catch (...) {
                std::cerr << "post image failed: " << msg.str_ << std::endl;
            }
            break;
        default:
            assert(false);
        }
    }
    flush();
}

void GetUserName(void* handler, char* const buffer, const size_t size, const char* const uid) {
    try {
        AcquireGIL a;
        const std::string user_name = boost::python::call<std::string>(g_get_user_name, uid);
        snprintf(buffer, size, "<%s(%s)>", user_name.c_str(), uid);
    } catch (...) {
        std::cerr << "GetUserName failed: " << uid << std::endl;
        snprintf(buffer, size, "<%s>", uid);
    }
}

// kook does not have group nickname, so the |gid| is meaningless
void GetUserNameInGroup(void* handler, char* const buffer, const size_t size, const char* group_id, const char* const user_id)
{
    return GetUserName(handler, buffer, size, user_id);
}

int DownloadUserAvatar(void* handler, const char* const uid, const char* const dest_filename)
{
    std::string url;
    try {
        AcquireGIL a;
        url = boost::python::call<std::string>(g_get_user_avatar_url, uid);
    } catch (...) {
        std::cerr << "DownloadUserAvatar get url with uid:" << uid << " failed" << std::endl;
        return false;
    }
    if (url.empty()) {
        std::cerr << "DownloadUserAvatar get empty url with uid:" << uid << std::endl;
        return false;
    }
    CURL* const curl = curl_easy_init();
    if (!curl) {
        std::cerr << "DownloadUserAvatar curl_easy_init() failed" << std::endl;
        return false;
    }
    FILE* const fp = fopen(dest_filename, "wb");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    const CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "DownloadUserAvatar curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    }
    curl_easy_cleanup(curl);
    fclose(fp);
    return res == CURLE_OK;
}

bool Start(
        const char* const game_path,
        const char* const db_path,
        const char* const conf_path,
        const char* const image_path,
        const char* const admins,
        PyObject* get_user_name,
        PyObject* get_user_avatar_url,
        PyObject* send_text_message,
        PyObject* send_image_message
        )
{
    ReleaseGIL r;
    const LGTBot_Option option {
        .game_path_ = game_path,
        .db_path_ = db_path,
        .conf_path_ = std::strlen(conf_path) == 0 ? nullptr : conf_path,
        .image_path_ = image_path,
        .admins_ = admins,
        .callbacks_ = LGTBot_Callback{
            .get_user_name = GetUserName,
            .get_user_name_in_group = GetUserNameInGroup,
            .download_user_avatar = DownloadUserAvatar,
            .handle_messages = HandleMessages,
        },
    };
    g_get_user_name = get_user_name;
    g_get_user_avatar_url = get_user_avatar_url;
    g_send_text_message = send_text_message;
    g_send_image_message = send_image_message;
    const char* errmsg = nullptr;
    g_bot_core = LGTBot_Create(&option, &errmsg);
    if (!g_bot_core) {
        std::cerr << "[ERROR] Init bot core failed, err: " << errmsg << std::endl;
        return false;
    }
    return true;
}

void OnPrivateMessage(const char* msg, const std::string& uid)
{
    ReleaseGIL r;
    LGTBot_HandlePrivateRequest(g_bot_core, uid.c_str(), msg);
}

void OnPublicMessage(const char* msg, const std::string& uid, const std::string& gid)
{
    ReleaseGIL r;
    LGTBot_HandlePublicRequest(g_bot_core, gid.c_str(), uid.c_str(), msg);
}

bool ReleaseBotIfNoProcessingGames()
{
    ReleaseGIL r;
    return LGTBot_ReleaseIfNoProcessingGames(g_bot_core);
}

BOOST_PYTHON_MODULE(lgtbot_kook)
{
    namespace python = boost::python;
    {
        python::def("start", Start);
        python::def("on_private_message", OnPrivateMessage);
        python::def("on_public_message", OnPublicMessage);
        python::def("release_bot_if_not_processing_games", ReleaseBotIfNoProcessingGames);
    }
}
