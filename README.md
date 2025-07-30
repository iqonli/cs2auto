# CS2 Auto Message Sender

> 在CS2中，按下绑定的键，即可发送对应组内的句子
>
> 程序使用豆包编写

本项目使用[GNU General Public License v3.0](LICENSE) 许可证。
[哔哩哔哩介绍视频](https://www.bilibili.com/video/BV1oo8dz2Evt)

## 介绍

1. **程序思路**：
   
   通过在`\Counter-Strike Global Offensive\game\csgo\cfg`这个目录生成一堆句子的cfg以及一个选择器cfg，再进行按键绑定，即可一键发送句子

2. **工作方式**：
   
   - 分组发送
   - 每个组可以设置TEAM/ALL聊天，以及顺序/随机发送
   - 但是CS2控制台实现随机数太困难，所以随机发送时只是打乱了发送句子的顺序，发送完所有语句会从头开始循环发送
   - 仅支持在本地使用文本编辑器编辑`groups.txt`，不支持程序内编辑，编辑完成需要重启程序
   - 请勿用于非法用途，包括打广告、骚扰队友等
   - 在CS2控制台输入`exec autoexec.cfg`刷新后才能使用
   - 如果不想每次都输入，可以在Steam>库>CS2右键>属性>高级用户可以选择输入对启动选项的修改的框里面输入`+exec autoexec.cfg`
   - 感谢`cneicy/ImLag`项目提供的思路以及部分句子，[视频](https://www.bilibili.com/video/BV1gNTgzLEKV)

## 安装与使用

无需管理员，运行cs2auto.exe即可。

## groups-e.g..txt - 示例格式

```txt
;注释
;组名必须全英文！下划线也行。如果不按规定操作，出问题别找我。
:组名1,order,all
句子1
句子2
:组名2,random,team
句子A
句子B
句子C
;order=顺序发送
;random=随机发送
;all=全部聊天
;team=队伍聊天
```
有事没事请加QQ群：743278470
