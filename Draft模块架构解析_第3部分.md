# FreeCAD Draft模块架构解析（第3部分）

## 四、核心系统深度解析

### 🎯 1. WorkingPlane（工作平面系统）

**文件**：`WorkingPlane.py`

**作用**：提供虚拟的2D工作平面，允许在任意方向绘图。

#### 核心类定义

```python
class PlaneBase:
    """工作平面基类"""
    
    def __init__(self, u=Vector(1,0,0), v=Vector(0,1,0), 
                 w=Vector(0,0,1), pos=Vector(0,0,0)):
        """初始化工作平面
        
        Parameters
        ----------
        u : Vector
            平面的X轴方向向量
        v : Vector
            平面的Y轴方向向量
        w : Vector
            平面的Z轴方向向量（法线）
        pos : Vector
            平面原点位置
        """
        if isinstance(u, PlaneBase):
            # 复制另一个平面
            self.match(u)
            return
        
        self.u = Vector(u)
        self.v = Vector(v)
        self.axis = Vector(w)
        self.position = Vector(pos)
    
    def __repr__(self):
        return f"Workplane x={self.u} y={self.v} z={self.axis} pos={self.position}"
    
    def copy(self):
        """创建副本"""
        return PlaneBase(self.u, self.v, self.axis, self.position)
    
    def match(self, other):
        """匹配另一个平面"""
        self.u = other.u
        self.v = other.v
        self.axis = other.axis
        self.position = other.position
    
    def alignToPointAndAxis(self, point, axis, offset=0):
        """对齐到点和轴
        
        Parameters
        ----------
        point : Vector
            对齐的点
        axis : Vector
            法线方向
        offset : float
            偏移距离
        """
        self.axis = axis
        self.axis.normalize()
        
        # 计算U和V向量
        if self.axis.isEqual(Vector(0, 0, 1), 1e-7):
            self.u = Vector(1, 0, 0)
            self.v = Vector(0, 1, 0)
        elif self.axis.isEqual(Vector(0, 0, -1), 1e-7):
            self.u = Vector(-1, 0, 0)
            self.v = Vector(0, 1, 0)
        else:
            self.v = axis.cross(Vector(0, 0, 1))
            self.v.normalize()
            self.u = self.v.cross(axis)
            self.u.normalize()
        
        # 设置位置
        self.position = point + axis * offset
    
    def alignToFace(self, face, offset=0):
        """对齐到面"""
        try:
            # 获取面的法线
            normal = face.normalAt(0, 0)
            center = face.CenterOfMass
            self.alignToPointAndAxis(center, normal, offset)
        except:
            pass
    
    def projectPoint(self, point, direction=None):
        """将点投影到工作平面
        
        Parameters
        ----------
        point : Vector
            要投影的3D点
        direction : Vector, optional
            投影方向，默认使用平面法线
        
        Returns
        -------
        Vector
            投影后的点
        """
        if direction is None:
            direction = self.axis
        
        # 计算从平面原点到点的向量
        vec = point - self.position
        
        # 投影到平面
        d = vec.dot(self.axis)
        projected = point - direction.normalize() * d
        
        return projected
    
    def getLocalCoords(self, point):
        """将全局坐标转换为平面局部坐标
        
        Parameters
        ----------
        point : Vector
            全局坐标点
        
        Returns
        -------
        Vector
            局部坐标 (x, y, z)
        """
        vec = point - self.position
        x = vec.dot(self.u)
        y = vec.dot(self.v)
        z = vec.dot(self.axis)
        return Vector(x, y, z)
    
    def getGlobalCoords(self, point):
        """将平面局部坐标转换为全局坐标
        
        Parameters
        ----------
        point : Vector
            局部坐标 (x, y, z)
        
        Returns
        -------
        Vector
            全局坐标点
        """
        return (self.position + 
                self.u * point.x + 
                self.v * point.y + 
                self.axis * point.z)
    
    def getPlacement(self):
        """获取平面的Placement对象"""
        import FreeCAD
        pl = FreeCAD.Placement()
        pl.Base = self.position
        
        # 构造旋转矩阵
        m = FreeCAD.Matrix()
        m.A11, m.A12, m.A13, m.A14 = self.u.x, self.v.x, self.axis.x, 0
        m.A21, m.A22, m.A23, m.A24 = self.u.y, self.v.y, self.axis.y, 0
        m.A31, m.A32, m.A33, m.A34 = self.u.z, self.v.z, self.axis.z, 0
        m.A41, m.A42, m.A43, m.A44 = 0, 0, 0, 1
        
        pl.Rotation = FreeCAD.Rotation(m)
        return pl

class Plane(PlaneBase):
    """带GUI支持的工作平面类"""
    
    def setup(self, direction=None, point=None, upvec=None, offset=0):
        """设置工作平面
        
        Parameters
        ----------
        direction : Vector or str
            法线方向，或预设方向 ("XY", "XZ", "YZ", "Front", "Top", "Side")
        point : Vector
            平面原点
        upvec : Vector
            上方向向量
        offset : float
            偏移距离
        """
        # 处理预设方向
        if isinstance(direction, str):
            if direction == "XY" or direction == "Top":
                self.alignToPointAndAxis(point or Vector(0,0,0), 
                                        Vector(0,0,1), offset)
            elif direction == "XZ" or direction == "Front":
                self.alignToPointAndAxis(point or Vector(0,0,0),
                                        Vector(0,1,0), offset)
            elif direction == "YZ" or direction == "Side":
                self.alignToPointAndAxis(point or Vector(0,0,0),
                                        Vector(1,0,0), offset)
            else:
                # 自动模式
                self.alignToSelection()
        elif direction:
            self.alignToPointAndAxis(point or Vector(0,0,0), direction, offset)

# 全局工作平面实例
_current_working_plane = None

def get_working_plane(update=True):
    """获取当前工作平面"""
    global _current_working_plane
    
    if _current_working_plane is None:
        if FreeCAD.GuiUp:
            _current_working_plane = Plane()
        else:
            _current_working_plane = PlaneBase()
    
    if update and FreeCAD.GuiUp:
        _current_working_plane.update()
    
    return _current_working_plane
```

#### 使用示例

```python
import WorkingPlane

# 获取工作平面
plane = WorkingPlane.get_working_plane()

# 设置为XY平面
plane.setup("XY")

# 设置为自定义方向
plane.alignToPointAndAxis(
    FreeCAD.Vector(0, 0, 100),
    FreeCAD.Vector(1, 1, 1)  # 法线方向
)

# 投影点到平面
point_3d = FreeCAD.Vector(10, 20, 30)
point_on_plane = plane.projectPoint(point_3d)

# 坐标转换
local = plane.getLocalCoords(point_3d)
global_back = plane.getGlobalCoords(local)
```

### 🎯 2. Snapper（捕捉系统）

**文件**：`draftguitools/gui_snapper.py`

**功能**：
- 自动捕捉到关键点（端点、中点、中心等）
- 显示捕捉标记
- 约束捕捉（水平、垂直等）

#### 核心类定义

```python
class Snapper:
    """Draft捕捉系统"""
    
    def __init__(self):
        # 捕捉类型
        self.snapStyle = 0  # 0=标准, 1=网格优先
        
        # 捕捉点类型标志
        self.snap_to_endpoint = True
        self.snap_to_midpoint = True
        self.snap_to_center = True
        self.snap_to_grid = True
        self.snap_to_perpendicular = True
        self.snap_to_extension = True
        self.snap_to_parallel = True
        self.snap_to_intersection = True
        
        # 当前捕捉状态
        self.snapInfo = None
        self.affinity = None
        
        # 可视化标记
        self.tracker = None
        self.extLine = None
        
        # 创建Coin节点
        self.createTrackers()
    
    def createTrackers(self):
        """创建捕捉标记的Coin节点"""
        from pivy import coin
        
        # 主标记
        self.tracker = coin.SoSeparator()
        self.tracker.setName("Snap_Tracker")
        
        # 不同捕捉类型的标记
        self.marker = coin.SoMarkerSet()
        self.marker.markerIndex = coin.SoMarkerSet.CIRCLE_LINE_9_9
        
        # 坐标
        self.coords = coin.SoCoordinate3()
        
        # 颜色
        self.color = coin.SoBaseColor()
        self.color.rgb = (1, 1, 0)  # 黄色
        
        # 构建场景图
        self.tracker.addChild(self.color)
        self.tracker.addChild(self.coords)
        self.tracker.addChild(self.marker)
    
    def snap(self, screenpos, lastpoint=None, active=True, 
             constrain=False, noTracker=False):
        """执行捕捉
        
        Parameters
        ----------
        screenpos : tuple
            屏幕坐标 (x, y)
        lastpoint : Vector
            上一个点，用于约束
        active : bool
            是否激活捕捉
        constrain : bool
            是否应用约束
        noTracker : bool
            是否隐藏追踪标记
        
        Returns
        -------
        Vector
            捕捉到的点
        """
        import FreeCADGui as Gui
        
        # 如果未激活，直接返回拾取点
        if not active:
            point = self.getPoint(screenpos)
            if not noTracker:
                self.hideTracker()
            return point
        
        # 获取鼠标下的对象
        view = Gui.ActiveDocument.ActiveView
        info = view.getObjectInfo(screenpos)
        
        snap_point = None
        snap_type = None
        
        if info:
            obj = info['Object']
            comp = info['Component']
            
            # 尝试不同的捕捉类型
            
            # 1. 端点捕捉
            if self.snap_to_endpoint:
                snap_point = self.snapToEndpoint(info)
                if snap_point:
                    snap_type = "endpoint"
            
            # 2. 中点捕捉
            if not snap_point and self.snap_to_midpoint:
                snap_point = self.snapToMidpoint(info)
                if snap_point:
                    snap_type = "midpoint"
            
            # 3. 中心捕捉
            if not snap_point and self.snap_to_center:
                snap_point = self.snapToCenter(info)
                if snap_point:
                    snap_type = "center"
            
            # 4. 垂直捕捉
            if not snap_point and self.snap_to_perpendicular and lastpoint:
                snap_point = self.snapToPerpendicular(info, lastpoint)
                if snap_point:
                    snap_type = "perpendicular"
        
        # 5. 网格捕捉
        if not snap_point and self.snap_to_grid:
            point = self.getPoint(screenpos)
            snap_point = self.snapToGrid(point)
            if snap_point:
                snap_type = "grid"
        
        # 应用约束
        if snap_point and constrain and lastpoint:
            snap_point = self.constrain(snap_point, lastpoint)
        
        # 更新标记
        if snap_point and not noTracker:
            self.showTracker(snap_point, snap_type)
        elif not noTracker:
            self.hideTracker()
        
        # 保存捕捉信息
        self.snapInfo = info
        self.affinity = snap_type
        
        return snap_point or self.getPoint(screenpos)
    
    def snapToEndpoint(self, info):
        """捕捉到端点"""
        try:
            obj = info['Object']
            comp = info['Component']
            
            if comp.startswith('Edge'):
                edge_index = int(comp[4:]) - 1
                edge = obj.Shape.Edges[edge_index]
                
                # 获取端点
                p1 = edge.Vertexes[0].Point
                p2 = edge.Vertexes[-1].Point
                
                # 返回最近的端点
                mp = info['x'], info['y'], info['z']
                mouse_point = FreeCAD.Vector(mp)
                
                if (mouse_point - p1).Length < (mouse_point - p2).Length:
                    return p1
                else:
                    return p2
        except:
            pass
        return None
    
    def snapToMidpoint(self, info):
        """捕捉到中点"""
        try:
            obj = info['Object']
            comp = info['Component']
            
            if comp.startswith('Edge'):
                edge_index = int(comp[4:]) - 1
                edge = obj.Shape.Edges[edge_index]
                
                # 返回中点
                param = (edge.FirstParameter + edge.LastParameter) / 2
                return edge.valueAt(param)
        except:
            pass
        return None
    
    def snapToCenter(self, info):
        """捕捉到中心"""
        try:
            obj = info['Object']
            comp = info['Component']
            
            if comp.startswith('Edge'):
                edge_index = int(comp[4:]) - 1
                edge = obj.Shape.Edges[edge_index]
                
                # 圆或圆弧的中心
                if hasattr(edge.Curve, 'Center'):
                    return edge.Curve.Center
        except:
            pass
        return None
    
    def snapToGrid(self, point):
        """捕捉到网格"""
        import WorkingPlane
        
        plane = WorkingPlane.get_working_plane(update=False)
        spacing = params.get_param("gridSpacing")
        
        # 转换到平面局部坐标
        local = plane.getLocalCoords(point)
        
        # 捕捉到网格
        local.x = round(local.x / spacing) * spacing
        local.y = round(local.y / spacing) * spacing
        local.z = 0  # 保持在平面上
        
        # 转换回全局坐标
        return plane.getGlobalCoords(local)
    
    def constrain(self, point, basepoint):
        """应用约束（水平/垂直）
        
        Parameters
        ----------
        point : Vector
            当前点
        basepoint : Vector
            基准点
        
        Returns
        -------
        Vector
            约束后的点
        """
        import WorkingPlane
        
        plane = WorkingPlane.get_working_plane(update=False)
        
        # 转换到平面局部坐标
        local_point = plane.getLocalCoords(point)
        local_base = plane.getLocalCoords(basepoint)
        
        # 计算差值
        dx = abs(local_point.x - local_base.x)
        dy = abs(local_point.y - local_base.y)
        
        # 约束到最近的轴
        if dx < dy:
            # 垂直约束
            local_point.x = local_base.x
        else:
            # 水平约束
            local_point.y = local_base.y
        
        # 转换回全局坐标
        return plane.getGlobalCoords(local_point)
    
    def showTracker(self, point, snap_type):
        """显示捕捉标记"""
        self.coords.point.setValue(point.x, point.y, point.z)
        
        # 根据捕捉类型设置标记样式
        if snap_type == "endpoint":
            self.marker.markerIndex = coin.SoMarkerSet.CIRCLE_FILLED_9_9
        elif snap_type == "midpoint":
            self.marker.markerIndex = coin.SoMarkerSet.TRIANGLE_FILLED_9_9
        elif snap_type == "center":
            self.marker.markerIndex = coin.SoMarkerSet.CIRCLE_LINE_9_9
        
        # 显示
        if not self.tracker.isVisible():
            self.tracker.setVisible(True)
    
    def hideTracker(self):
        """隐藏捕捉标记"""
        if self.tracker.isVisible():
            self.tracker.setVisible(False)
    
    def getPoint(self, screenpos):
        """从屏幕坐标获取3D点"""
        view = Gui.ActiveDocument.ActiveView
        point = view.getPoint(screenpos)
        return FreeCAD.Vector(point)
```

### 🎯 3. DraftToolBar（工具栏/任务面板系统）

**文件**：`DraftGui.py`

```python
class DraftToolBar:
    """Draft统一的用户界面
    
    这个类管理Draft命令的任务面板UI，
    包括坐标输入、约束控制、选项设置等。
    """
    
    def __init__(self):
        # 当前活动命令
        self.sourceCmd = None
        
        # 鼠标输入控制 ⭐
        self.mouse = True
        self.mouse_delay_input_start = time.time()
        
        # UI状态
        self.continueMode = False
        
        # 创建UI组件
        self.setupUi()
    
    def setupUi(self):
        """创建UI组件"""
        # 创建坐标输入框
        self.xValue = QtGui.QLineEdit()
        self.yValue = QtGui.QLineEdit()
        self.zValue = QtGui.QLineEdit()
        
        # 创建约束控制
        self.isRelative = QtGui.QCheckBox("Relative")
        self.isContinue = QtGui.QCheckBox("Continue")
        self.isFilled = QtGui.QCheckBox("Fill")
        
        # 连接信号
        self.xValue.textChanged.connect(self.checkSpecialChars)
        self.yValue.textChanged.connect(self.checkSpecialChars)
    
    def pointUi(self, title="Point", cancel=None, extra=None):
        """显示点输入界面"""
        # 设置标题
        self.setTitle(title)
        
        # 显示坐标输入
        self.xValue.setEnabled(True)
        self.yValue.setEnabled(True)
        self.zValue.setEnabled(True)
        
        # 重置鼠标模式
        self.mouse = True
        self.mouse_delay_input_start = time.time()
    
    def lineUi(self, title="Line", icon="Draft_Line"):
        """显示线条输入界面"""
        self.pointUi(title)
        self.extraLineUi()
    
    def wireUi(self, title="Wire", icon="Draft_Wire"):
        """显示多段线输入界面"""
        self.pointUi(title)
        # 添加闭合和填充选项
        self.closeButton.show()
        self.makeFace.show()
    
    def setMouseMode(self, mode=True, recorded_input_start=0.0):
        """设置鼠标输入模式 ⭐⭐⭐
        
        这个方法控制鼠标输入的启用/禁用。
        当用户在输入框中输入数字时，会临时禁用鼠标，
        避免意外的鼠标移动修改输入值。
        
        Parameters
        ----------
        mode : bool
            True=启用鼠标, False=禁用鼠标
        recorded_input_start : float
            输入开始时间戳，用于防止过时的定时器
        """
        # 检查是否是过时的定时器回调
        if recorded_input_start and recorded_input_start != self.mouse_delay_input_start:
            return
        
        if mode:
            # 启用鼠标
            self.mouse = True
        elif self.mouse:
            # 禁用鼠标
            delay = params.get_param("MouseDelay")
            if delay:
                self.mouse = False
                recorded_input_start = self.mouse_delay_input_start
                
                # 设置定时器，延迟后重新启用
                QtCore.QTimer.singleShot(
                    delay * 1000,
                    lambda: self.setMouseMode(True, recorded_input_start)
                )
    
    def checkSpecialChars(self, txt):
        """检查输入的特殊字符
        
        当用户在坐标输入框中输入时调用。
        如果是数字，则临时禁用鼠标输入。
        """
        if txt == "":
            return
        
        # 如果是数字，禁用鼠标
        if txt[0] in "0123456789.,-":
            self.updateSnapper()
            self.setMouseMode(mode=False)  # ⭐ 禁用鼠标
            return
        
        # 处理快捷键
        txt = txt[0].upper()
        
        if txt == "R":  # Relative
            self.isRelative.toggle()
        elif txt == "X":  # X约束
            self.constrain("x")
        elif txt == "Y":  # Y约束
            self.constrain("y")
        elif txt == "Z":  # Z约束
            self.constrain("z")
    
    def displayPoint(self, point, last=None, plane=None, mask=None):
        """显示点坐标
        
        Parameters
        ----------
        point : Vector
            要显示的点
        last : Vector
            上一个点（用于相对坐标）
        plane : WorkingPlane
            工作平面
        mask : str
            约束掩码
        """
        if plane is None:
            import WorkingPlane
            plane = WorkingPlane.get_working_plane(update=False)
        
        # 转换到平面局部坐标
        local = plane.getLocalCoords(point)
        
        # 如果是相对模式，计算相对坐标
        if self.isRelative.isChecked() and last:
            local_last = plane.getLocalCoords(last)
            local = local - local_last
        
        # 更新输入框
        self.xValue.setText(f"{local.x:.2f}")
        self.yValue.setText(f"{local.y:.2f}")
        self.zValue.setText(f"{local.z:.2f}")
```

## 五、数据流与交互流程

### 📊 1. 创建对象的完整流程

```
┌─────────────┐
│  用户点击   │ 工具栏按钮
│  工具按钮   │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ GUI命令激活  │ Line.Activated()
│             │ - 初始化UI
│             │ - 创建临时对象
│             │ - 注册事件回调
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 事件循环等待│ 用户交互
│             │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 鼠标移动事件│ SoLocation2Event
│             │ ├─ getPoint()
│             │ ├─ Snapper.snap()
│             │ └─ 更新预览
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 鼠标点击事件│ SoMouseButtonEvent
│             │ ├─ 记录点
│             │ ├─ 更新几何
│             │ └─ 判断是否完成
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  完成命令   │ Line.finish()
│             │ ├─ 移除事件回调
│             │ ├─ 删除临时对象
│             │ └─ 提交命令列表
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 延迟执行    │ ToDo.delayCommit()
│             │ ├─ 打开事务
│             │ ├─ 执行命令字符串
│             │ └─ 提交事务
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 创建对象    │ make_line()
│             │ ├─ addObject()
│             │ ├─ Wire(obj)
│             │ └─ ViewProviderWire()
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 对象重计算  │ obj.execute()
│             │ ├─ 计算几何
│             │ └─ 更新Shape
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 视图更新    │ vobj.updateData()
│             │ ├─ 更新Coin节点
│             │ └─ 触发重绘
└─────────────┘
```

### 🔄 2. 修改对象的流程

```
┌─────────────┐
│ 用户修改属性│ obj.Length = 100
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 触发器启动  │ onChanged()
│             │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 标记重计算  │ obj.touch()
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 文档重计算  │ doc.recompute()
│             │ ├─ 拓扑排序
│             │ └─ 依次执行
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 对象执行    │ obj.Proxy.execute()
│             │ ├─ 读取属性
│             │ ├─ 计算几何
│             │ └─ 更新Shape
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ 视图更新    │ vobj.Proxy.updateData()
│             │ ├─ 更新显示
│             │ └─ 触发重绘
└─────────────┘
```

## 六、关键设计模式

### 1️⃣ **代理模式（Proxy Pattern）**

Draft大量使用代理模式来扩展C++对象：

```python
# 对象代理
obj = doc.addObject("Part::Part2DObjectPython", "Circle")
obj.Proxy = Circle(obj)  # Python对象作为C++对象的代理

# 视图代理
vobj = obj.ViewObject
vobj.Proxy = ViewProviderCircle(vobj)  # Python视图对象作为代理
```

**优势**：
- 无需修改C++代码即可扩展功能
- Python脚本化对象灵活性高
- 支持序列化和文档保存

### 2️⃣ **工厂模式（Factory Pattern）**

`draftmake/`模块提供工厂函数：

```python
# 使用工厂函数创建对象
line = Draft.make_line(p1, p2)
circle = Draft.make_circle(radius)
array = Draft.make_array(obj, count)

# 内部实现
def make_line(p1, p2):
    obj = doc.addObject(...)  # 创建C++对象
    Wire(obj)                 # 附加Python行为
    ViewProviderWire(vobj)    # 附加视图行为
    return obj
```

### 3️⃣ **命令模式（Command Pattern）**

所有GUI工具实现统一的命令接口：

```python
class Line:
    def GetResources(self):
        """命令资源"""
        return {'Pixmap': '...', 'MenuText': '...'}
    
    def IsActive(self):
        """命令是否可用"""
        return True
    
    def Activated(self):
        """执行命令"""
        pass
```

### 4️⃣ **观察者模式（Observer Pattern）**

属性变化自动触发更新：

```python
class Wire:
    def onChanged(self, obj, prop):
        """属性变化时调用"""
        if prop == "Points":
            obj.touch()  # 标记需要重计算
```

### 5️⃣ **单例模式（Singleton Pattern）**

全局共享的对象：

```python
# 工作平面单例
plane = WorkingPlane.get_working_plane()

# 捕捉器单例
FreeCADGui.Snapper = gui_snapper.Snapper()

# 工具栏单例
FreeCADGui.draftToolBar = DraftToolBar()
```

## 七、最佳实践与注意事项

### ✅ 设计优点

1. **模块化设计**：各层职责清晰，易于维护
2. **可扩展性**：通过代理模式易于添加新对象类型
3. **代码重用**：工具函数高度可复用
4. **脚本友好**：API设计良好，易于编程
5. **分离关注点**：数据、显示、控制完全分离

### ⚠️ 注意事项

1. **Coin回调限制**：不能在事件回调中直接修改场景图
   ```python
   # ❌ 错误
   def action(self, arg):
       doc.addObject(...)  # 会崩溃
   
   # ✅ 正确
   def action(self, arg):
       self.commit_list.append(...)
   def finish(self):
       todo.ToDo.delayCommit(self.commit_list)
   ```

2. **鼠标延迟机制**：理解`ui.mouse`标志
   ```python
   # 检查鼠标是否启用
   if not ui.mouse:
       return None, None, None  # 不处理鼠标输入
   ```

3. **工作平面坐标系**：注意全局/局部坐标转换
   ```python
   local = plane.getLocalCoords(global_point)
   global_back = plane.getGlobalCoords(local)
   ```

4. **对象生命周期**：注意临时对象的清理
   ```python
   def finish(self):
       if self.obj:
           doc.removeObject(self.obj.Name)
   ```

### 🐛 常见问题

**问题1：鼠标点击无响应**
- **原因**：`ui.mouse = False`
- **诊断**：
  ```python
  print(FreeCADGui.draftToolBar.mouse)
  print(params.get_param("MouseDelay"))
  ```
- **解决**：
  ```python
  FreeCADGui.draftToolBar.mouse = True
  # 或设置MouseDelay=0
  ```

**问题2：在Coin回调中崩溃**
- **原因**：直接修改场景图
- **解决**：使用延迟执行
  ```python
  todo.delay(func, arg)
  todo.delayCommit(commands)
  ```

**问题3：坐标系混乱**
- **原因**：未正确转换坐标系
- **解决**：始终通过工作平面转换

## 八、总结

### 架构特点

Draft模块采用了经典的**分层架构**和**MVC模式**：

```
┌─────────────────────────────────────┐
│        GUI交互层 (Controller)        │
│  draftguitools/ - 命令和事件处理    │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│        业务逻辑层 (Model)            │
│  draftobjects/ - 对象数据和逻辑     │
│  draftmake/ - 对象创建               │
│  draftgeoutils/ - 几何算法          │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│         视图层 (View)                │
│  draftviewproviders/ - 显示渲染     │
└─────────────────────────────────────┘
```

### 核心优势

1. **关注点分离**：数据、显示、交互完全解耦
2. **高度可扩展**：通过代理模式易于扩展
3. **脚本友好**：良好的API设计
4. **稳定可靠**：通过延迟执行避免Coin3D限制

### 学习建议

如果要深入理解Draft模块，建议按以下顺序：

1. **基础对象**：`draftobjects/wire.py`
2. **视图提供者**：`draftviewproviders/view_wire.py`
3. **简单工具**：`draftguitools/gui_points.py`
4. **复杂工具**：`draftguitools/gui_lines.py`
5. **核心系统**：`WorkingPlane.py`, `gui_snapper.py`
6. **辅助系统**：`draftutils/todo.py`, `gui_tool_utils.py`

---

**文档完成时间**：2024年
**FreeCAD版本**：基于当前主分支
**作者**：AI Assistant

希望这份文档能帮助您理解Draft模块的架构！🎉


