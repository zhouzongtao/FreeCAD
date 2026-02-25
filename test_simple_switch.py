"""
简单的后端切换测试

最小化测试，只验证基本功能
"""

import FreeCAD
import FreeCADGui
import time

print("=" * 50)
print("简单后端切换测试")
print("=" * 50)

# 创建文档和对象
doc = FreeCAD.newDocument("SimpleTest")
box = doc.addObject("Part::Box", "Box")
doc.recompute()
print("✓ 创建了 Box 对象")

# 显示
FreeCADGui.showMainWindow()
time.sleep(1)

# 切换
print("\n切换到 OsgVerse...")
try:
    FreeCADGui.switchRenderBackend(2)
    print("✓ 切换成功！")
    print("\n如果看到红色球体，说明 OsgVerse 正常工作")
    print("（真实几何体转换尚未实现）")
except Exception as e:
    print(f"✗ 失败: {e}")
