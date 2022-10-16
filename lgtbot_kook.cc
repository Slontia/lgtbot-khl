#include <boost/python.hpp>
#include <boost/python/call.hpp>

#include <memory>

#include "bot_core/bot_core.h"

std::unique_ptr<void, void(*)(void*)> g_bot_core(nullptr, BOT_API::Release);
PyObject* g_get_user_name = nullptr;
PyObject* g_send_text_message = nullptr;
PyObject* g_send_image_message = nullptr;

bool Start(
        const char* const this_uid,
        const char* const game_path,
        const char* const image_path,
        const char* const admins,
        const std::filesystem::path::value_type* const db_path,
        PyObject* get_user_name,
        PyObject* send_text_message,
        PyObject* send_image_message
        )
{
    const BotOption option {
        .this_uid_ = this_uid,
        .game_path_ = game_path,
        .image_path_ = image_path,
        .admins_ = admins,
        .db_path_ = db_path,
    };
    std::cout << "Bot User ID = " << option.this_uid_ << std::endl;
    g_bot_core.reset(BOT_API::Init(&option));
    if (!g_bot_core) {
        std::cerr << "[ERROR] Init bot core failed" << std::endl;
        return false;
    }
    g_get_user_name = get_user_name;
    g_send_text_message = send_text_message;
    g_send_image_message = send_image_message;
    return true;
}

struct Messager
{
    Messager(const char* const id, const bool is_uid) : id_(id), is_uid_(is_uid) {}
    const std::string id_;
    const bool is_uid_;
    std::stringstream ss_;
};

void* OpenMessager(const char* const id, const bool is_uid)
{
    return new Messager(id, is_uid);
}

void MessagerPostText(void* p, const char* data, uint64_t len)
{
    static_cast<Messager*>(p)->ss_ << std::string_view(data, len);
}

std::string user_name_str(const char* const uid)
{
    return "<" + boost::python::call<std::string>(g_get_user_name, uid) + "(" + uid + ")>";
};

// kook does not have group nickname, so the |gid| is meaningless
const char* GetUserName(const char* const uid, const char* const /*gid*/)
{
    thread_local static std::string str;
    str =  user_name_str(uid);
    return str.c_str();
}

void MessagerPostUser(void* const p, const char* const uid, const bool is_at)
{
    const auto messager = static_cast<Messager*>(p);
    if (!is_at) {
        messager->ss_ << user_name_str(uid);
    } else if (!messager->is_uid_) {
        messager->ss_ << "(met)" << uid << "(met)";
    } else if (uid == messager->id_) {
        messager->ss_ << ("<你>");
    } else {
        messager->ss_ << user_name_str(uid);
    }
}

void MessagerFlush(void* p)
{
    const auto messager = static_cast<Messager*>(p);
    if (messager->ss_.str().empty()) {
        return;
    }
    boost::python::call<void>(g_send_text_message, messager->id_, messager->is_uid_, messager->ss_.str());
    messager->ss_.str("");
}

void MessagerPostImage(void* p, const std::filesystem::path::value_type* path)
{
    MessagerFlush(p);
    const auto messager = static_cast<Messager*>(p);
    boost::python::call<void>(g_send_image_message, messager->id_, messager->is_uid_, path);
}

void CloseMessager(void* p)
{
    const auto messager = static_cast<Messager*>(p);
    delete messager;
}

void OnPrivateMessage(const char* msg, const std::string& uid)
{
    BOT_API::HandlePrivateRequest(g_bot_core.get(), uid.c_str(), msg);
}

void OnPublicMessage(const char* msg, const std::string& uid, const std::string& gid)
{
    BOT_API::HandlePublicRequest(g_bot_core.get(), gid.c_str(), uid.c_str(), msg);
}

BOOST_PYTHON_MODULE(lgtbot_kook)
{
    namespace python = boost::python;
    {
        python::def("start", Start);
        python::def("on_private_message", OnPrivateMessage);
        python::def("on_public_message", OnPublicMessage);
    }
}
