"""
快速切换到 Coin3D 后端
"""

import FreeCADGui

print("=" * 60)
print("切换到 Coin3D 后端")
print("=" * 60)

# 后端类型
BACKEND_COIN3D = 1

try:
    # 获取当前后端
    current = FreeCADGui.getCurrentRenderBackend()
    backend_names = {0: "None", 1: "Coin3D", 2: "OsgVerse"}
    print(f"当前后端: {backend_names.get(current, 'Unknown')}")
    
    if current == BACKEND_COIN3D:
        print("✓ 已经是 Coin3D 后端")
    else:
        # 切换到 Coin3D
        print("正在切换到 Coin3D...")
        success = FreeCADGui.switchRenderBackend(BACKEND_COIN3D)
        
        if success:
            print("✓ 成功切换到 Coin3D")
            print(f"渲染器: {FreeCADGui.getRendererInfo()}")
            
            # 适应视图
            try:
                FreeCADGui.SendMsgToActiveView("ViewFit")
                print("✓ 视图已适应")
            except:
                pass
            
            print("\n预期效果：")
            print("- 对象显示为真实的几何体（Box、Cylinder、Sphere 等）")
            print("- 对象有正确的颜色和材质")
            print("- 所有 Coin3D 功能正常工作")
        else:
            print("✗ 切换失败")
            
except Exception as e:
    print(f"错误: {e}")
    print("\n提示：确保 FreeCAD 已完全启动并且有活动文档")

print("=" * 60)
