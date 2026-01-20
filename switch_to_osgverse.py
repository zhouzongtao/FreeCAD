"""
快速切换到 OsgVerse 后端
"""

import FreeCADGui

print("=" * 60)
print("切换到 OsgVerse 后端")
print("=" * 60)

# 后端类型
BACKEND_OSGVERSE = 2

try:
    # 获取当前后端
    current = FreeCADGui.getCurrentRenderBackend()
    backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
    print(f"当前后端: {backend_names.get(current, 'Unknown')}")
    
    if current == BACKEND_OSGVERSE:
        print("✓ 已经是 OsgVerse 后端")
    else:
        # 检查 OsgVerse 是否可用
        if not FreeCADGui.isRenderBackendAvailable(BACKEND_OSGVERSE):
            print("✗ OsgVerse 后端不可用")
            print("  请确保 OsgVerse 已正确编译和安装")
        else:
            # 切换到 OsgVerse
            print("正在切换到 OsgVerse...")
            success = FreeCADGui.switchRenderBackend(BACKEND_OSGVERSE)
            
            if success:
                print("✓ 成功切换到 OsgVerse")
                print(f"渲染器: {FreeCADGui.getRendererInfo()}")
                
                # 适应视图
                try:
                    FreeCADGui.SendMsgToActiveView("ViewFit")
                    print("✓ 视图已适应")
                except:
                    pass
                
                print("\n预期效果（Phase 1）：")
                print("- 所有对象显示为红色球体占位符")
                print("- 球体半径 5.0，完整可见")
                print("- Report View 中有 [OsgVerse] 前缀的日志")
                print("- 可以正常旋转、缩放视图")
            else:
                print("✗ 切换失败")
                
except Exception as e:
    print(f"错误: {e}")
    print("\n提示：确保 FreeCAD 已完全启动并且有活动文档")

print("=" * 60)
