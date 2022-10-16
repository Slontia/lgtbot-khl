# for async
import asyncio
import nest_asyncio

# for flags
import gflags
import sys

# for bot logic
from khl import Bot, Message, MessageTypes, ChannelPrivacyTypes
import lgtbot_kook
import logging

logging.basicConfig(level='INFO')

# C++ code will invoke Python methods which should not be async. In this case, we need use
# |asyncio.get_event_loop().run_until_complete| to wait for other async methods returned.
# An error of "RuntimeError: This event loop is already running in python" will be thrown
# without nest_asyncio.
nest_asyncio.apply()

gflags.DEFINE_string('token', '', 'token for Kook robot')
gflags.DEFINE_string('admin_uids', '', 'administrator user id list (splitted by \',\')')
gflags.DEFINE_string('game_path', './plugins', 'the path of game modules')
gflags.DEFINE_string('image_path', './images', 'the path of image modules')
gflags.DEFINE_string('db_path', './lgtbot_data.db', 'the path of database')

gflags.FLAGS(sys.argv)
bot = Bot(token=gflags.FLAGS.token)

@bot.on_message()
async def on_message(msg: Message):
    if msg.channel_type == ChannelPrivacyTypes.PERSON: # private message
        lgtbot_kook.on_private_message(msg.content, msg.author_id)
    elif bot.me.id in msg.extra.get('mention'):
        lgtbot_kook.on_public_message(msg.content.replace(f'(met){bot.me.id}(met)', ''), msg.author_id, msg.target_id)


def get_user_name(uid: str):
    try:
        return asyncio.get_event_loop().run_until_complete(bot.client.fetch_user(uid)).nickname
    except Exception as e:
        print(e)
        return ""


async def send_message_async(id: str, is_uid: bool, msg: str, type: MessageTypes):
    channel = await bot.client.fetch_user(id) if is_uid else await bot.client.fetch_public_channel(id)
    await channel.send(content = msg, type = type)


async def send_image_message_async(id: str, is_uid: bool, image: str):
    url = await bot.create_asset(open(image, 'rb'))
    await send_message_async(id, is_uid, url, MessageTypes.IMG)


def send_text_message(id: str, is_uid: bool, msg: str):
    try:
        asyncio.get_event_loop().run_until_complete(send_message_async(id, is_uid, msg, MessageTypes.KMD))
    except Exception as e:
        print(e)


def send_image_message(id: str, is_uid: bool, msg: str):
    try:
        asyncio.get_event_loop().run_until_complete(send_image_message_async(id, is_uid, msg))
    except Exception as e:
        print(e)


async def make_lgtbot(bot: Bot):
    lgtbot_kook.start((await bot.client.fetch_me()).id, gflags.FLAGS.game_path, gflags.FLAGS.image_path, gflags.FLAGS.admin_uids, gflags.FLAGS.db_path, get_user_name, send_text_message, send_image_message)


bot.on_startup(make_lgtbot)

# everything done, go ahead now!
bot.run()
# now invite the bot to a server, and send '/hello' in any channel
# (remember to grant the bot with read & send permissions)

