# 📐 FreeCAD Draft 模块详细架构解析

## 一、总体架构概览

Draft模块是FreeCAD的核心工作台之一，用于2D绘图和注释。它采用了**分层架构**设计，遵循**MVC（Model-View-Controller）**模式的变体。

```
Draft 模块架构
├── 核心层（Core Layer）
│   ├── C++ 扩展模块（DraftUtils）
│   └── Python 基础类
├── 业务逻辑层（Business Logic Layer）
│   ├── 对象模型（Model）
│   ├── 创建函数（Factory）
│   └── 几何工具（Geometry Utils）
├── GUI 交互层（GUI Interaction Layer）
│   ├── 命令工具（Commands）
│   ├── 视图提供者（View Providers）
│   └── UI 组件（UI Components）
└── 支持层（Support Layer）
    ├── 工具函数（Utilities）
    ├── 导入导出（Import/Export）
    └── 测试模块（Tests）
```

## 二、目录结构详解

### 🔹 1. **App/** - C++扩展模块
```
App/
├── AppDraftUtils.cpp      # C++模块入口
├── AppDraftUtilsPy.cpp    # Python绑定
└── CMakeLists.txt         # 编译配置
```

**作用**：
- 提供性能关键的底层功能
- 作为Python模块`DraftUtils`被导入
- 主要用于加载Part模块依赖

**代码示例**（AppDraftUtils.cpp）：
```cpp
namespace DraftUtils {
extern PyObject* initModule();
}

PyMOD_INIT_FUNC(DraftUtils)
{
    // load dependent module
    try {
        Base::Interpreter().loadModule("Part");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }
    PyObject* mod = DraftUtils::initModule();
    Base::Console().log("Loading DraftUtils module… done\n");
    PyMOD_Return(mod);
}
```

### 🔹 2. **draftobjects/** - 对象模型层（Model）

**目录结构**：
```
draftobjects/
├── base.py              # 基类 DraftObject
├── wire.py              # 线对象
├── circle.py            # 圆对象
├── rectangle.py         # 矩形对象
├── array.py             # 阵列对象
├── dimension.py         # 标注对象
├── text.py              # 文本对象
├── bspline.py           # B样条对象
├── bezcurve.py          # 贝塞尔曲线对象
├── ellipse.py           # 椭圆对象
├── polygon.py           # 多边形对象
├── point.py             # 点对象
├── label.py             # 标签对象
├── layer.py             # 图层对象
├── clone.py             # 克隆对象
├── facebinder.py        # 面绑定器
├── hatch.py             # 填充对象
└── draft_annotation.py  # 注释基类
```

**核心概念**：

#### DraftObject基类
```python
class DraftObject(object):
    """所有Draft对象的基类"""
    
    def __init__(self, obj, tp="Unknown"):
        """初始化Draft对象
        
        Parameters
        ----------
        obj : Part::FeaturePython or similar
            FreeCAD基础C++对象
        tp : str
            对象类型标识
        """
        if obj:
            obj.Proxy = self  # 代理模式：将Python对象附加到C++对象
        self.Type = tp
    
    def onDocumentRestored(self, obj):
        """文档加载后恢复对象"""
        self.props_changed_clear()
    
    def dumps(self):
        """序列化对象状态"""
        return self.Type
    
    def loads(self, state):
        """反序列化对象状态"""
        if state:
            self.Type = state
```

#### 具体对象示例 - Wire（线/多段线）
```python
class Wire(DraftObject):
    """Draft Wire对象"""
    
    def __init__(self, obj):
        super().__init__(obj, "Wire")
        self._add_properties(obj)
    
    def _add_properties(self, obj):
        """添加对象属性"""
        # 几何属性
        obj.addProperty("App::PropertyVectorList", "Points",
                       "Draft", "顶点列表")
        obj.addProperty("App::PropertyBool", "Closed",
                       "Draft", "是否闭合")
        obj.addProperty("App::PropertyBool", "MakeFace",
                       "Draft", "是否创建面")
        # 显示属性
        obj.addProperty("App::PropertyLength", "Length",
                       "Draft", "总长度", 1)  # 只读
    
    def execute(self, obj):
        """重新计算对象几何"""
        import Part
        if len(obj.Points) < 2:
            return
        
        # 创建线段
        edges = []
        for i in range(len(obj.Points) - 1):
            edges.append(Part.LineSegment(
                obj.Points[i], obj.Points[i+1]
            ).toShape())
        
        if obj.Closed and len(obj.Points) > 2:
            edges.append(Part.LineSegment(
                obj.Points[-1], obj.Points[0]
            ).toShape())
        
        # 创建Wire
        wire = Part.Wire(edges)
        obj.Shape = wire
        
        # 更新长度
        obj.Length = wire.Length
        
        # 如果需要创建面
        if obj.MakeFace and obj.Closed:
            try:
                obj.Shape = Part.Face(wire)
            except:
                pass
```

**工作机制**：
```python
# 创建Draft对象的完整流程
import FreeCAD as App
import Draft

doc = App.ActiveDocument

# 方式1：使用make函数（推荐）
line = Draft.make_line(
    App.Vector(0, 0, 0),
    App.Vector(100, 100, 0)
)

# 方式2：手动创建（内部实现）
obj = doc.addObject("Part::Part2DObjectPython", "Wire")
Wire(obj)  # 附加Draft对象行为
ViewProviderWire(obj.ViewObject)  # 附加视图行为
obj.Points = [App.Vector(0,0,0), App.Vector(100,100,0)]
doc.recompute()
```

### 🔹 3. **draftviewproviders/** - 视图提供者层（View）

**目录结构**：
```
draftviewproviders/
├── view_base.py         # 基类 ViewProviderDraft
├── view_wire.py         # 线的显示
├── view_dimension.py    # 标注的显示
├── view_text.py         # 文本的显示
├── view_array.py        # 阵列的显示
├── view_layer.py        # 图层的显示
└── ...
```

**核心概念**：

#### ViewProviderDraft基类
```python
class ViewProviderDraft(object):
    """Draft视图提供者基类"""
    
    def __init__(self, vobj):
        """初始化视图提供者
        
        Parameters
        ----------
        vobj : Gui::ViewProviderDocumentObject
            视图对象
        """
        self.Object = vobj.Object
        self.texture = None
        self.texcoords = None
        self._set_properties(vobj)
        vobj.Proxy = self
    
    def attach(self, vobj):
        """附加到3D场景图
        
        在这里创建Coin3D节点
        """
        from pivy import coin
        
        self.Object = vobj.Object
        
        # 创建根节点
        self.coords = coin.SoCoordinate3()
        self.lines = coin.SoLineSet()
        
        # 构建场景图
        self.node = coin.SoGroup()
        self.node.addChild(self.coords)
        self.node.addChild(self.lines)
        
        vobj.addDisplayMode(self.node, "Flat Lines")
    
    def updateData(self, obj, prop):
        """对象数据变化时更新显示
        
        Parameters
        ----------
        obj : Draft对象
        prop : str
            变化的属性名称
        """
        if prop == "Points":
            # 更新坐标
            points = obj.Points
            self.coords.point.setValues(
                0, len(points), points
            )
    
    def getDisplayModes(self, vobj):
        """返回可用的显示模式"""
        return ["Flat Lines", "Wireframe", "Shaded"]
    
    def getDefaultDisplayMode(self):
        """返回默认显示模式"""
        return "Flat Lines"
    
    def setDisplayMode(self, mode):
        """设置显示模式"""
        return mode
    
    def onChanged(self, vobj, prop):
        """视图属性变化时调用"""
        if prop == "LineColor":
            # 更新线条颜色
            pass
    
    def getIcon(self):
        """返回工具栏图标路径"""
        return ":/icons/Draft_Wire.svg"
```

### 🔹 4. **draftguitools/** - GUI命令层（Controller）

这是Draft模块中最复杂也是最重要的部分，包含了所有用户交互逻辑。

**目录结构**：
```
draftguitools/
├── gui_base.py              # 新式基类
├── gui_base_original.py     # 原始基类（DraftTool, Creator, Modifier）
├── gui_lines.py             # 线条绘制工具
├── gui_circles.py           # 圆形绘制工具
├── gui_arcs.py              # 圆弧绘制工具
├── gui_rectangles.py        # 矩形绘制工具
├── gui_polygons.py          # 多边形绘制工具
├── gui_beziers.py           # 贝塞尔曲线工具
├── gui_splines.py           # 样条曲线工具
├── gui_dimensions.py        # 标注工具
├── gui_texts.py             # 文本工具
├── gui_move.py              # 移动工具
├── gui_rotate.py            # 旋转工具
├── gui_scale.py             # 缩放工具
├── gui_offset.py            # 偏移工具
├── gui_mirror.py            # 镜像工具
├── gui_edit.py              # 编辑工具
├── gui_snapper.py           # 捕捉系统 ⭐
├── gui_trackers.py          # 可视化追踪器
├── gui_tool_utils.py        # 工具辅助函数 ⭐
└── ...
```

#### 命令类继承体系

```python
# ===== 旧式继承体系（gui_base_original.py）=====
class DraftTool:
    """所有Draft工具的基类（旧版）"""
    
    def __init__(self):
        self.commitList = []
    
    def IsActive(self):
        """命令是否可用"""
        return bool(gui_utils.get_3d_view())
    
    def Activated(self, name="None", is_subtool=False):
        """命令激活时调用"""
        if App.activeDraftCommand and not is_subtool:
            App.activeDraftCommand.finish()
        
        App.activeDraftCommand = self
        self.call = None
        self.doc = App.ActiveDocument
        self.ui = Gui.draftToolBar
        self.view = gui_utils.get_3d_view()
        self.wp = WorkingPlane.get_working_plane()
        
        # 创建平面追踪器
        if params.get_param("showPlaneTracker"):
            self.planetrack = trackers.PlaneTracker()
        
        # 初始化捕捉器
        if hasattr(Gui, "Snapper"):
            Gui.Snapper.setTrackers()
    
    def finish(self, cont=False):
        """结束命令"""
        self.node = []
        App.activeDraftCommand = None
        
        # 清理UI
        if self.ui:
            self.ui.offUi()
        
        # 关闭捕捉器
        if hasattr(Gui, "Snapper"):
            Gui.Snapper.off()
        
        # 移除事件回调
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        
        # 提交命令
        if self.commitList:
            todo.ToDo.delayCommit(self.commitList)

class Creator(DraftTool):
    """创建类工具基类"""
    
    def Activated(self, name="None"):
        super().Activated(name)
        self.wp._save()  # 保存工作平面状态
        self.support = gui_tool_utils.get_support()

class Modifier(DraftTool):
    """修改类工具基类"""
    
    def __init__(self):
        super().__init__()
        self.copymode = False

# ===== 新式继承体系（gui_base.py）=====
class GuiCommandSimplest:
    """最简单的命令基类"""
    
    def __init__(self, name="None"):
        self.doc = None
        self.featureName = name
    
    def IsActive(self):
        return bool(App.activeDocument())
    
    def Activated(self):
        self.doc = App.activeDocument()

class GuiCommandNeedsSelection(GuiCommandSimplest):
    """需要选择的命令基类"""
    
    def IsActive(self):
        return bool(Gui.Selection.getSelection())

class GuiCommandBase:
    """完整功能的命令基类"""
    
    def __init__(self, name="None"):
        App.activeDraftCommand = None
        self.call = None
        self.commit_list = []
        self.featureName = name
        self.planetrack = None
        self.view = None
    
    def finish(self):
        """结束命令并提交"""
        App.activeDraftCommand = None
        if self.planetrack:
            self.planetrack.finalize()
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        if self.commit_list:
            todo.ToDo.delayCommit(self.commit_list)
```


