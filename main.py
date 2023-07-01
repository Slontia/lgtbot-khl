# for async
import asyncio
import nest_asyncio
import threading

# for flags
import gflags
import sys

# for bot logic
from khl import Bot, Message, MessageTypes, ChannelPrivacyTypes
import lgtbot_kook
import logging

# for listen ctrl-c
import signal

logging.basicConfig(level='INFO')

# C++ code will invoke Python methods which should not be async. In this case, we need use
# |asyncio.get_event_loop().run_until_complete| to wait for other async methods returned.
# An error of "RuntimeError: This event loop is already running in python" will be thrown
# without nest_asyncio.
nest_asyncio.apply()

gflags.DEFINE_string('token', '', 'token for Kook robot')
gflags.DEFINE_string('admin_uids', '', 'administrator user id list (splitted by \',\')')
gflags.DEFINE_string('game_path', './plugins', 'the path of game modules')
gflags.DEFINE_string('db_path', './lgtbot_data.db', 'the path of database')
gflags.DEFINE_string('conf_path', '', 'the path of the configuration')
gflags.DEFINE_string('image_path', './images', 'the path of image modules')

gflags.FLAGS(sys.argv)
bot = Bot(token=gflags.FLAGS.token)

@bot.on_message()
async def on_message(msg: Message):
    # We must start a new thread to handle the request. Otherwise, the coroutine may be stucked at a match lock,
    # and the other thread holding the match lock may be stucked at waiting the future.
    if msg.channel_type == ChannelPrivacyTypes.PERSON: # private message
        threading.Thread(target=lgtbot_kook.on_private_message, args=(msg.content, msg.author_id,)).start()
    elif bot.me.id in msg.extra.get('mention'): # public message where the bot is mentioned
        threading.Thread(target=lgtbot_kook.on_public_message, args=(msg.content.replace(f'(met){bot.me.id}(met)', ''), msg.author_id, msg.target_id,)).start()


async def send_message_async(id: str, is_uid: bool, msg: str, type: MessageTypes):
    channel = await bot.client.fetch_user(id) if is_uid else await bot.client.fetch_public_channel(id)
    await channel.send(content = msg, type = type)


async def send_image_message_async(id: str, is_uid: bool, image: str):
    url = await bot.create_asset(open(image, 'rb'))
    await send_message_async(id, is_uid, url, MessageTypes.IMG)


def sync_exec(func, *args):
    try:
        if bot.loop._thread_id == threading.current_thread().ident:
            return asyncio.run_until_complete(func(*args), bot.loop)
        else:
            return asyncio.run_coroutine_threadsafe(func(*args), bot.loop).result()
    except Exception as e:
        print(e)


def get_user_name(uid: str):
    return sync_exec(bot.client.fetch_user, uid).nickname

def get_user_avatar_url(uid: str):
    return sync_exec(bot.client.fetch_user, uid).avatar

def send_text_message(id: str, is_uid: bool, msg: str):
    sync_exec(send_message_async, id, is_uid, msg, MessageTypes.KMD)


def send_image_message(id: str, is_uid: bool, msg: str):
    sync_exec(send_image_message_async,id, is_uid, msg)

if not lgtbot_kook.start(gflags.FLAGS.game_path, gflags.FLAGS.db_path, gflags.FLAGS.conf_path, gflags.FLAGS.image_path, gflags.FLAGS.admin_uids, get_user_name, get_user_avatar_url, send_text_message, send_image_message):
    sys.exit(1)

def sigint_handler(signum, frame):
    if lgtbot_kook.release_bot_if_not_processing_games():
        sys.exit(0)
    else:
        print("There are processing games, please retry later or force exit by kill -9.");

signal.signal(signal.SIGINT, sigint_handler)

# everything done, go ahead now!
bot.run()
# now invite the bot to a server, and send '/hello' in any channel
# (remember to grant the bot with read & send permissions)

