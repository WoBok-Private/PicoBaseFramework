# Pico Base Framework

**中文** | [English](README.md)

面向 Unreal Engine 5.8 的通用 PICO VR 玩家基础框架，提供 PicoBaseGameMode、PicoBasePawn、PicoBasePlayerController 和 PicoBaseStart，支持桌面操作、头部与手柄追踪，以及水平移动。

## 安装与场景配置

1. 将 PicoBaseFramework 文件夹放入项目的 Plugins 目录，启用插件并编译 C++ 工程。
2. 在 World Settings 中，将 GameMode Override 设置为 **PicoBaseGameMode**。
3. 在 Place Actors 中搜索 **PicoBaseStart**，将其原点放在地面上，箭头指向初始视线方向。

PicoBaseGameMode 会创建并控制 PicoBasePawn。插件在运行时创建 Enhanced Input 输入映射。

## 编辑器中的名称

类的显示名称为 **PicoBaseStart**、**PicoBasePawn**、**PicoBaseGameMode** 和 **PicoBasePlayerController**，本文使用相同名称。

Place Actors 显示类的显示名称。Outliner 的 **Type** 列表示类，**Label** 列表示可编辑的场景实例名称。新建 PicoBaseStart 通常使用 PicoBaseStart 作为实例标签，多个实例可能带数字后缀。已有或手动修改的实例标签会保留。

插件属性分组为 **Pico Base Framework**。

## PicoBaseStart 设置

| 属性 | 默认值 | 作用 |
| --- | --- | --- |
| Move Speed | 300 cm/s（3 m/s） | 桌面和右摇杆移动速度。 |
| Desktop Eye Height | 165 cm | 桌面模式下，相机相对地面原点的高度。 |
| Right Stick Dead Zone | 0.2 | VR 摇杆死区，超出死区后的输入会重新缩放。 |
| Enable Collision | 关闭 | 使用玩家胶囊检测阻挡虚拟移动，并支持沿墙滑动。 |
| Priority | 0 | 存在多个 PicoBaseStart 时，数值较高者优先。 |

PicoBasePawn 的 **Pico Base Framework** 分组中也提供 **Enable Collision**。使用 PicoBaseStart 出生时，会将起点设置复制到玩家 Pawn；没有 PicoBaseStart 时，使用 Pawn 的类默认值。

## 出生位置与地面高度

以下规则适用于使用 PicoBaseGameMode 创建玩家的地图。

| 可用起点 | 出生行为 |
| --- | --- |
| 有 PicoBaseStart | 使用其位置和水平朝向，优先于普通 Player Start。 |
| 只有 Player Start | 使用 UE 选择的 Player Start，并将其原点 Z 坐标作为地面高度。 |
| 两种起点都没有 | UE 回退到 World Settings 的位置，通常为世界原点 (0, 0, 0)。 |
| 没有 PicoBaseStart，使用 Play From Here | 使用编辑器提供的出生位置，并将其 Z 坐标作为地面高度。 |

显式放置的 PicoBaseStart 也优先于 Play From Here。XY 定义虚拟出生位置，Z 定义虚拟地面高度，箭头定义初始水平朝向。俯仰、翻滚和缩放不改变追踪坐标系。

每张地图通常设置一个 PicoBaseStart。存在多个时，选择 Priority 最高者；数值相同时按对象路径确定顺序，并在日志中输出警告。

普通 Player Start 的原点也应放在地面上。原点悬在地面上方会使玩家的虚拟地面和相机随之升高。

桌面模式相机高度为地面原点加 Desktop Eye Height；VR 模式使用头显测得的真实眼高。UE 场景单位为厘米。

## 操作与移动

- 桌面：WASD 移动；鼠标上移抬头、下移低头，右移向右转、左移向左转。
- VR：头部和手柄随设备追踪；右摇杆上/下前进/后退，左/右平移，方向跟随头部的水平朝向。
- PicoBasePawn 的 Mouse Sensitivity 控制鼠标灵敏度，默认值为 1。
- 斜向移动速度归一化，VR 摇杆保留模拟输入力度。
- 虚拟移动保持玩家地面原点的 Z 坐标。

**Enable Collision** 关闭时，虚拟移动可以穿过障碍物。开启时，默认移动胶囊半径为 25 cm、半高为 90 cm，底部位于地面上方 2 cm。胶囊使用 **Pawn** 碰撞配置，障碍物需要阻挡该通道。移动路径检测会阻止穿越阻挡物，并允许沿墙水平滑动。出生时应避免胶囊与阻挡物相交。

碰撞作用于桌面和摇杆移动，真实头部追踪独立保留。此移动模式保持水平，不包含重力、爬楼梯或沿坡面移动。

项目可以在此框架上扩展左摇杆转向、手柄模型、抓取、传送和联网功能。

## 地面追踪

优先使用 Stage，备选为 LocalFloor。初始化期间等待有效追踪；两种地面原点都不可用时，输出错误、暂停摇杆移动并重试。

请在头显系统中校准真实地面和活动区域。PicoBaseStart 定义虚拟地面，XR 运行时提供真实地面追踪。初次对齐将头部 XY 和水平朝向对应到起点，同时保留测量到的眼高。重定位或重连时，以当前虚拟位置和朝向重新对齐。

## 项目集成

主要配置为 **UE 5.8 + OpenXR + PICOController**，PICO 原生应用使用 Android ARM64。项目负责 Android SDK、签名、渲染和 XR 运行时配置；插件负责玩家控制、追踪和输入。

桌面模式用于 Windows 编辑器 Play 和 Windows 应用；VR 模式在有效 XR 会话中启用。其他 PICO SDK 或串流运行时的兼容性，应在目标设备上确认。

源码插件需要 C++ 构建环境，更换引擎版本后应重新编译。此前 PicoUniversal 类名的序列化引用会重定向到 PicoBaseFramework 对应类。

