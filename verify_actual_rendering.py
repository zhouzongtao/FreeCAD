"""
验证实际渲染是否使用 OsgVerse
"""

import FreeCADGui
import FreeCAD

print("=" * 70)
print("验证实际渲染后端")
print("=" * 70)

# 1. 检查 RenderManager 状态
print("\n1. RenderManager 状态:")
print(f"   当前后端: {FreeCADGui.getCurrentRenderBackend()}")
print(f"   渲染器信息: {FreeCADGui.getRendererInfo()}")

# 2. 检查是否有活动文档和视图
print("\n2. 检查活动视图:")
if FreeCADGui.ActiveDocument:
    print(f"   活动文档: {FreeCADGui.ActiveDocument.Document.Name}")
    
    # 获取活动视图
    view = FreeCADGui.ActiveDocument.ActiveView
    if view:
        print(f"   活动视图类型: {type(view)}")
        print(f"   视图对象: {view}")
        
        # 尝试获取视图的渲染器信息
        if hasattr(view, 'getViewer'):
            viewer = view.getViewer()
            print(f"   Viewer 类型: {type(viewer)}")
            print(f"   Viewer 对象: {viewer}")
        
        # 检查是否有 Coin3D 相关的属性
        if hasattr(view, 'getSceneGraph'):
            sg = view.getSceneGraph()
            print(f"   SceneGraph 类型: {type(sg)}")
            print(f"   这表明可能仍在使用 Coin3D")
    else:
        print("   没有活动视图")
else:
    print("   没有活动文档")
    print("   提示: 创建一个新文档和 3D 视图来测试")

# 3. 检查统计信息
print("\n3. 渲染统计:")
try:
    stats = FreeCADGui.getRenderStats()
    print(f"   帧数: {stats.get('frameCount', 0)}")
    print(f"   FPS: {stats.get('fps', 0):.2f}")
    print(f"   绘制调用: {stats.get('drawCalls', 0)}")
except Exception as e:
    print(f"   无法获取统计: {e}")

# 4. 建议
print("\n4. 验证建议:")
print("   - 创建一个新文档: FreeCAD.newDocument()")
print("   - 添加一个对象: Part.makeBox(10,10,10)")
print("   - 查看 Report View 中的日志")
print("   - 检查是否有 OsgVerse 相关的初始化消息")

print("\n" + "=" * 70)
print("注意: RenderManager 显示后端是 OsgVerse")
print("但实际的 3D 视图可能仍在使用 Coin3D")
print("需要检查视图创建和渲染的实际代码路径")
print("=" * 70)
