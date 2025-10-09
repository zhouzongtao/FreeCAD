# FreeCAD Draft模块架构解析（第2部分）

## 三、GUI工具详细解析（续）

### 🎯 具体工具示例 - Line工具

**文件**：`draftguitools/gui_lines.py`

```python
class Line(gui_base_original.Creator):
    """线条绘制工具"""
    
    def __init__(self, mode="line"):
        super().__init__()
        self.mode = mode  # "line" 或 "wire"
    
    def GetResources(self):
        """定义命令资源"""
        return {
            'Pixmap': 'Draft_Line',
            'Accel': "L,I",  # 快捷键
            'MenuText': "Line",
            'ToolTip': "Creates a 2-point line"
        }
    
    def Activated(self, name="Line", icon="Draft_Line", task_title=None):
        """命令激活"""
        # 1. 调用父类初始化
        super().Activated(name)
        
        # 2. 设置UI
        if self.mode == "wire":
            self.ui.wireUi(title=title, icon=icon)
        else:
            self.ui.lineUi(title=title, icon=icon)
        
        # 3. 创建临时对象用于预览
        self.obj = self.doc.addObject("Part::Feature", self.featureName)
        gui_utils.format_object(self.obj)
        self.obj.ViewObject.ShowInTree = False
        
        # 4. 注册事件回调 ⭐ 关键步骤
        self.call = self.view.addEventCallback("SoEvent", self.action)
        
        _toolmsg("Pick first point")
    
    def action(self, arg):
        """处理3D视图事件 ⭐ 核心方法
        
        Parameters
        ----------
        arg : dict
            事件参数字典
            - Type: 事件类型 (SoKeyboardEvent, SoMouseButtonEvent, SoLocation2Event)
            - Key: 按键 (for keyboard events)
            - State: 状态 (UP, DOWN)
            - Button: 鼠标按钮 (BUTTON1, BUTTON2, BUTTON3)
            - Position: 屏幕位置
        """
        # === 处理键盘事件 ===
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
            return
        
        # === 处理鼠标移动事件 ===
        if arg["Type"] == "SoLocation2Event":
            # 获取鼠标位置对应的3D点
            self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
            gui_tool_utils.redraw3DView()
            return
        
        # === 处理鼠标按钮事件 ===
        if arg["Type"] != "SoMouseButtonEvent":
            return
        
        # 鼠标抬起 - 恢复对象可选择性
        if arg["State"] == "UP":
            self.obj.ViewObject.Selectable = True
            return
        
        # 鼠标按下 - 添加点
        if arg["State"] == "DOWN" and arg["Button"] == "BUTTON1":
            # 临时禁用对象选择，避免干扰
            self.obj.ViewObject.Selectable = False
            
            # 检查是否点击了相同位置（双击完成）
            if arg["Position"] == self.pos:
                self.finish(cont=None)
                return
            
            # 第一次点击 - 获取支持面
            if (not self.node) and (not self.support):
                gui_tool_utils.getSupport(arg)
                self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
            
            # 添加点到节点列表
            if self.point:
                self.ui.redraw()
                self.pos = arg["Position"]
                self.node.append(self.point)
                self.drawUpdate(self.point)
                
                # 如果是line模式且已有2点，完成绘制
                if self.mode == "line" and len(self.node) == 2:
                    self.finish(cont=None, closed=False)
                
                # 检查是否闭合（回到起点）
                if len(self.node) > 2:
                    if (self.point - self.node[0]).Length < utils.tolerance():
                        self.undolast()
                        if len(self.node) > 2:
                            self.finish(cont=None, closed=True)
                        else:
                            self.finish(cont=None, closed=False)
    
    def drawUpdate(self, point):
        """更新临时几何预览"""
        import Part
        
        if len(self.node) == 1:
            _toolmsg("Pick next point")
        elif len(self.node) == 2:
            # 创建第一条线段
            last = self.node[-2]
            newseg = Part.LineSegment(last, point).toShape()
            self.obj.Shape = newseg
            self.obj.ViewObject.Visibility = True
        else:
            # 添加新的线段到现有形状
            currentshape = self.obj.Shape.copy()
            last = self.node[-2]
            newseg = Part.LineSegment(last, point).toShape()
            newshape = currentshape.fuse(newseg)
            self.obj.Shape = newshape
    
    def finish(self, cont=False, closed=False):
        """完成绘制
        
        Parameters
        ----------
        cont : bool
            是否继续创建下一个对象
        closed : bool
            是否闭合线条
        """
        # 1. 移除事件回调
        self.end_callbacks(self.call)
        
        # 2. 删除临时对象
        self.removeTemporaryObject()
        
        # 3. 创建最终对象
        if len(self.node) > 1:
            Gui.addModule("Draft")
            
            # 构建命令字符串列表
            rot, sup, pts, fil = self.getStrings()
            
            _base = DraftVecUtils.toString(self.node[0])
            _cmd = 'Draft.make_wire'
            _cmd += '('
            _cmd += 'points, '
            _cmd += 'placement=pl, '
            _cmd += 'closed=' + str(closed) + ', '
            _cmd += 'face=' + fil + ', '
            _cmd += 'support=' + sup
            _cmd += ')'
            
            _cmd_list = [
                'pl = FreeCAD.Placement()',
                'pl.Rotation.Q = ' + rot,
                'pl.Base = ' + _base,
                'points = ' + pts,
                'line = ' + _cmd,
                'Draft.autogroup(line)',
                'FreeCAD.ActiveDocument.recompute()'
            ]
            
            # 提交命令到延迟执行队列
            self.commit("Create Wire", _cmd_list)
        
        # 4. 调用父类finish
        super().finish()
        
        # 5. 如果设置了继续模式，重新激活
        if cont or (cont is None and self.ui and self.ui.continueMode):
            self.Activated()
```

### 🔹 5. **draftmake/** - 对象创建工厂

**目录结构**：
```
draftmake/
├── make_line.py
├── make_wire.py
├── make_circle.py
├── make_arc_3points.py
├── make_rectangle.py
├── make_polygon.py
├── make_dimension.py
├── make_text.py
├── make_array.py
└── ...
```

**设计模式**：工厂模式

**示例 - make_line.py**：
```python
def make_line(p1, p2=None):
    """创建线对象
    
    Parameters
    ----------
    p1 : Base::Vector3
        起点，或点列表
    p2 : Base::Vector3, optional
        终点
    
    Returns
    -------
    obj : Draft Wire对象
    """
    import FreeCAD as App
    import Draft
    
    # 获取当前文档
    if not App.ActiveDocument:
        App.Console.PrintError("No active document\n")
        return None
    
    doc = App.ActiveDocument
    
    # 处理参数
    if p2 is None:
        if isinstance(p1, list):
            points = p1
        else:
            App.Console.PrintError("Need 2 points\n")
            return None
    else:
        points = [p1, p2]
    
    # 创建对象
    obj = doc.addObject("Part::Part2DObjectPython", "Line")
    
    # 附加Draft对象行为
    from draftobjects.wire import Wire
    Wire(obj)
    
    # 设置属性
    obj.Points = points
    obj.Closed = False
    
    # 附加视图提供者
    if App.GuiUp:
        from draftviewproviders.view_wire import ViewProviderWire
        ViewProviderWire(obj.ViewObject)
    
    # 添加到当前组
    Draft.autogroup(obj)
    
    # 选择新对象
    Draft.select(obj)
    
    return obj
```

### 🔹 6. **draftgeoutils/** - 几何工具库

**目录结构**：
```
draftgeoutils/
├── geometry.py          # 通用几何函数
├── intersections.py     # 交点计算
├── arcs.py              # 圆弧操作
├── circles.py           # 圆操作
├── edges.py             # 边操作
├── wires.py             # 线操作
├── faces.py             # 面操作
├── offsets.py           # 偏移计算
├── fillets.py           # 圆角计算
├── linear_algebra.py    # 线性代数
├── sort_edges.py        # 边排序
└── geo_arrays.py        # 几何阵列
```

**特点**：
- 纯数学/几何函数
- 不依赖FreeCAD文档对象
- 高度可重用

**示例函数**：
```python
# intersections.py
def findIntersection(edge1, edge2, infinite1=False, infinite2=False):
    """查找两条边的交点
    
    Parameters
    ----------
    edge1, edge2 : Part.Edge
        要计算交点的边
    infinite1, infinite2 : bool
        是否将边视为无限延伸
    
    Returns
    -------
    list of Base::Vector3
        交点列表
    """
    # 实现交点计算算法
    pass

# offsets.py
def offset(edge, vector):
    """偏移边
    
    Parameters
    ----------
    edge : Part.Edge
        要偏移的边
    vector : Base::Vector3
        偏移向量
    
    Returns
    -------
    Part.Edge
        偏移后的边
    """
    pass
```

### 🔹 7. **draftutils/** - 工具函数库

**目录结构**：
```
draftutils/
├── utils.py             # 通用工具
├── gui_utils.py         # GUI工具
├── params.py            # 参数管理
├── todo.py              # 延迟执行 ⭐
├── messages.py          # 消息输出
├── translate.py         # 国际化
├── units.py             # 单位转换
├── groups.py            # 组管理
├── init_tools.py        # 工具初始化
└── init_draft_statusbar.py  # 状态栏初始化
```

#### 🔧 关键组件详解

**1. todo.py - 延迟执行系统** ⭐⭐⭐
```python
class ToDo:
    """延迟执行命令，避免Coin3D回调崩溃
    
    Coin3D不允许在事件回调中修改场景图。
    这个类通过Qt的定时器机制，将命令延迟到
    下一个事件循环中执行。
    """
    
    itinerary = []  # 命令队列
    
    @staticmethod
    def delay(f, arg):
        """延迟执行单个函数
        
        Parameters
        ----------
        f : callable
            要执行的函数
        arg : any
            函数参数
        """
        from PySide import QtCore
        
        if ToDo.itinerary:
            # 如果队列不为空，添加到队列
            ToDo.itinerary.append((f, arg))
        else:
            # 队列为空，立即添加并启动定时器
            ToDo.itinerary.append((f, arg))
            QtCore.QTimer.singleShot(0, ToDo.runCommands)
    
    @staticmethod
    def delayCommit(commands):
        """延迟提交命令列表
        
        Parameters
        ----------
        commands : list of (name, cmd_list)
            命令列表，每个命令是(名称, 命令字符串列表)元组
        """
        # 打开撤销事务
        doc = FreeCAD.ActiveDocument
        for name, cmd_list in commands:
            doc.openTransaction(name)
            for cmd in cmd_list:
                FreeCADGui.doCommand(cmd)
            doc.commitTransaction()
    
    @staticmethod
    def runCommands():
        """执行队列中的所有命令"""
        try:
            for f, arg in ToDo.itinerary:
                try:
                    if arg:
                        f(arg)
                    else:
                        f()
                except Exception as e:
                    App.Console.PrintError(str(e))
        finally:
            ToDo.itinerary = []

# 使用示例
def action(self, arg):
    # 不能在这里直接创建对象（会崩溃）
    # doc.addObject(...)  # ❌ 错误！
    
    # 正确做法：延迟执行
    todo.delay(doc.addObject, "Part::Feature")  # ✅ 正确
```

**2. params.py - 参数管理**
```python
def get_param(param, path="User parameter:BaseApp/Preferences/Mod/Draft"):
    """从用户参数数据库读取参数
    
    Parameters
    ----------
    param : str
        参数名称
    path : str
        参数路径
    
    Returns
    -------
    value
        参数值
    """
    p = FreeCAD.ParamGet(path)
    
    # 根据参数名称确定类型
    if param == "UiMode":
        return p.GetInt(param, 1)
    elif param == "gridEvery":
        return p.GetInt(param, 10)
    elif param == "snapRange":
        return p.GetInt(param, 8)
    elif param == "constructioncolor":
        return p.GetUnsigned(param, 746455039)
    elif param == "alwaysSnap":
        return p.GetBool(param, True)
    # ... 更多参数

def set_param(param, value):
    """设置参数值"""
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    
    if isinstance(value, bool):
        p.SetBool(param, value)
    elif isinstance(value, int):
        p.SetInt(param, value)
    elif isinstance(value, float):
        p.SetFloat(param, value)
    elif isinstance(value, str):
        p.SetString(param, value)
```

**3. gui_tool_utils.py - GUI工具函数** ⭐
```python
def get_point(target, args, noTracker=False):
    """获取约束的3D点
    
    这是Draft工具中最重要的函数之一，
    负责将鼠标位置转换为3D坐标，
    并应用捕捉和约束。
    
    Parameters
    ----------
    target : Draft工具对象
        包含node属性的工具对象（通常是self）
    args : dict
        Coin事件参数
    noTracker : bool
        是否禁用追踪线
    
    Returns
    -------
    point : Base::Vector3
        捕捉后的点
    ctrlPoint : Base::Vector3
        控制点
    info : dict
        对象信息
    """
    ui = Gui.draftToolBar
    
    # ⭐ 关键：检查鼠标输入是否启用
    if not ui.mouse:
        return None, None, None
    
    # 获取最后一个点（用于约束）
    if target.node:
        last = target.node[-1]
    else:
        last = None
    
    # 检查修饰键
    smod = has_mod(args, get_mod_snap_key())    # Shift
    cmod = has_mod(args, get_mod_constrain_key())  # Ctrl
    
    point = None
    
    # 使用捕捉器获取点
    if hasattr(Gui, "Snapper"):
        point = Gui.Snapper.snap(
            args["Position"],
            lastpoint=last,
            active=smod,
            constrain=cmod,
            noTracker=noTracker
        )
        info = Gui.Snapper.snapInfo
        mask = Gui.Snapper.affinity
    
    # 如果捕捉器没有返回点，使用视图拾取
    if not point:
        p = Gui.ActiveDocument.ActiveView.getCursorPos()
        point = Gui.ActiveDocument.ActiveView.getPoint(p)
        info = Gui.ActiveDocument.ActiveView.getObjectInfo(p)
        mask = None
    
    ctrlPoint = App.Vector(point)
    wp = WorkingPlane.get_working_plane(update=False)
    
    # 更新UI显示
    if target.node:
        ui.displayPoint(point, target.node[-1], plane=wp, mask=mask)
    else:
        ui.displayPoint(point, plane=wp, mask=mask)
    
    return point, ctrlPoint, info
```


