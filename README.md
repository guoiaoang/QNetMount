# QNetMount
模仿 NetMount 写的一个 GUI，可代替使用 `cmd` 的启动方式（项目肯定会半途而废，是个大坑）。

## 用法

使用 Qt 构建完成后，启动程序。
1. 点击 `Alist` Tab, 设置`程序路径`和`数据路径`，点击 `启动`。
2. 点击 `设置` Tab，勾选 `启动时隐藏窗口` 和 `默认启动 Alist`，关闭窗口即可。
下次启动该程序即可启动 Alist 服务。
关闭窗口后托盘图标依然在运行，可在托盘图标菜单中点击退出。

## 编写原因

Alist 是一个很好用的服务端，设置非常简单，功能却十分强大。但是官方不对免费用户提供图形化的服务端管理界面。

[NetMount](https://github.com/VirtualHotBar/NetMount/) 项目把 [Alist](https://github.com/alist-org/alist) 和 [RcClone](https://github.com/rclone/rclone) 集成到了一起，做了个图形化的服务端管理界面，是我日常使用的一个程序。

但是， NetMount 每次启动时都会把管理员密码设置为随机的字符串，而手动更改 Alist 的密码又会导致 Token 出现问题。

本来想修改一下 NetMount 的源码凑合用一下，不过由于不会相关的技术栈，导致效果跟预想中差很多。

光安装环境都不会配置，好几个包，好几个 G 的相关文件，而且我也不会启动调试模式，QT 虽然也不是最好的，不过胜在熟悉。

因此，在 Windows 11 上使用 QT 6 编写了该项目。

**注意：** 此项目作为练手或学习，未做跨平台的兼容，可能明天就废弃了。

## 功能

除 "设置" 页及 "Alist" 页，其他功能均未实现。
~~删除线内内容为未实现的内容~~

- 功能列表
    - 设置
        - ~~切换界面语言~~
        - ~~切换界面主题~~
        - ~~开机自启~~
        - 启动程序到托盘
        - 启动程序时是否启动 Alist 服务
    - Alist
        - 自定义 Alist 程序路径
        - 自定义 Alist 数据路径
        - 可在该界面对 Alist 服务进行启动及停止的操作
        - 在当前客户端内修改管理员密码（由于Alist限制，只允许修改不允许查看）

## 与 NetMount 区别

取消了 NetMount 每次启动均会重置 Alist 管理员密码的功能。
（自己一个人在床上躺的好好的，不想起来碰电脑改配置）

由于 NetMount 中 Alist 的配置文件 `config.json` 中路径为固定路径，不便于便携运行，因此程序在运行时会替换其中的路径到配置路径下。
（就是由于 NetMount 与 官方的 Alist 不兼容，比如官方支持便携式运行，NetMount 成了安装式，最后只能自动动手丰衣足食了）

~~NetMount 启动 Alist 使用的是 `alist server` 命令，不会在 Windows 上生成 `pid` 文件，无法使用 `alist stop` 停止服务，因此此项目使用 `alist start` 启动服务。~~

修改 Alist 启动命令为 `alist server` 。 `alist start` 启动服务时会启动中间程序，退出 `alist server` 程序，导致进程父子关系断开，无法使用 `alist stop` 命令停止服务，无法监控服务运行状态。
