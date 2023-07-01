<div align="center">

![Logo](https://github.com/Slontia/lgtbot/blob/master/images/logo_transparent_colorful.svg)

![image](https://img.shields.io/badge/author-slontia-blue.svg) ![image](https://img.shields.io/badge/language-c++20-green.svg)

</div>

## 1 项目简介

*「LGT」源自日本漫画家甲斐谷忍创作的《Liar Game》中的虚构组织「**L**iar **G**ame **T**ournament 事务所」。*

[LGTBot](https://github.com/Slontia/lgtbot) 是一个基于 C++ 实现的，用于在 **聊天室** 或 **其它通讯软件** 中，实现多人 **文字推理游戏** 的裁判机器人库。而 [lgtbot-khl](https://github.com/Slontia/lgtbot-khl) 基于 [khl.py](https://github.com/TWT233/khl.py) 框架，将 LGTBot 库适配到了 Kook，实现与机器人的交互。

**欢迎入群体验（Kook 服务器）：https://kook.top/ePPqa3**

## 3 构建方法

### 3.1 编译

请确保您的编译器支持 **C++20** 语法，建议使用 g++10 以上版本

要求 Python >= 3.6.8

以 Centos 为例：

``` bash
# 安装依赖库（Ubuntu 系统）
sudo apt-get install -y libgoogle-glog-dev libgflags-dev libgtest-dev libsqlite3-dev libqt5webkit5-dev libcurl-dev

# 安装 khl.py
pip install khl.py

# 完整克隆本项目
git clone github.com/slontia/lgtbot-khl

# 安装子模块
git submodule update --init --recursive

# 构建二进制路径
mkdir build

# 编译项目
cmake .. -DWITH_GCOV=OFF -DWITH_ASAN=OFF -DWITH_GLOG=OFF -DWITH_SQLITE=ON -DWITH_TEST=OFF -DWITH_GAMES=ON
make

```

### 3.2 启动

执行如下命令启动机器人：

```
python3 main.py --token <your bot token>
```

附加参数列表：

- `token`: token for Kook robot
- `admin_uids`: administrator user id list (splitted by \',\')
- `game_path`: the path of game modules, default value is "./plugins"
- `db_path`: the path of database, default value is "./lgtbot_data.db"
- `conf_path`: the path of the configuration
- `image_path`: the path of image modules, default value is "./images"

