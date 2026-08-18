# UI

当前包含 `UGobulinPlayerStatusWidget`：为 M1 单机战斗闭环提供原生 C++ 生命值、受伤红闪、死亡遮罩和重开入口。

它是临时反馈层，不拥有伤害或死亡规则。正式 UI 可以创建该类的 WBP 子类，以 WidgetTree 和蓝图表现事件替换视觉；菜单、结算、建造 UI 仍按后续里程碑接入。
