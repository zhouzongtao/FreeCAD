#!/usr/bin/env python3
"""
批量修复 OsgVerse 代码中的初始化列表问题
Fix initialization list issues in OsgVerse code
"""

import re
import sys

def fix_vec3f_init(content):
    """修复 Vec3f 初始化列表"""
    # 匹配模式: variable = {x, y, z};
    pattern = r'(\w+)\s*=\s*\{([^}]+)\};'
    
    def replace_func(match):
        var = match.group(1)
        values = match.group(2)
        return f'{var} = Vec3f({values});'
    
    content = re.sub(pattern, replace_func, content)
    
    # 修复 vector 中的初始化列表
    # normals[i] = {n.x(), n.y(), n.z()};
    pattern2 = r'(\w+)\[(\w+)\]\s*=\s*\{([^}]+)\};'
    
    def replace_func2(match):
        var = match.group(1)
        index = match.group(2)
        values = match.group(3)
        # 判断是 Vec3f 还是 Color
        if 'color' in var.lower() or values.count(',') == 3:
            return f'{var}[{index}] = Color({values});'
        else:
            return f'{var}[{index}] = Vec3f({values});'
    
    content = re.sub(pattern2, replace_func2, content)
    
    return content

def fix_color_init(content):
    """修复 Color 初始化列表"""
    # colors[i] = {c.r(), c.g(), c.b(), c.a()};
    pattern = r'(colors\[\w+\])\s*=\s*\{([^}]+)\};'
    
    def replace_func(match):
        var = match.group(1)
        values = match.group(2)
        return f'{var} = Color({values});'
    
    content = re.sub(pattern, replace_func, content)
    return content

def fix_vec2f_init(content):
    """修复 Vec2f 初始化列表"""
    # texCoords[i] = {tc.x(), tc.y()};
    pattern = r'(texCoords\[\w+\])\s*=\s*\{([^}]+)\};'
    
    def replace_func(match):
        var = match.group(1)
        values = match.group(2)
        return f'{var} = Vec2f({values});'
    
    content = re.sub(pattern, replace_func, content)
    return content

def fix_boundbox(content):
    """修复 BoundingBox API"""
    # bbox.min = ... -> bbox.MinX = ...
    content = re.sub(r'bbox\.min', 'bbox.Min', content)
    content = re.sub(r'bbox\.max', 'bbox.Max', content)
    return content

def fix_make_unique(content):
    """修复 std::make_unique 调用"""
    # return std::make_unique<OsgVerseGeometry>(geom, false);
    # -> return std::unique_ptr<OsgVerseGeometry>(new OsgVerseGeometry(geom, false));
    pattern = r'return\s+std::make_unique<(\w+)>\(([^)]+)\);'
    
    def replace_func(match):
        class_name = match.group(1)
        args = match.group(2)
        return f'return std::unique_ptr<{class_name}>(new {class_name}({args}));'
    
    content = re.sub(pattern, replace_func, content)
    
    # auto geom = std::make_unique<OsgVerseGeometry>();
    # -> auto geom = std::unique_ptr<OsgVerseGeometry>(new OsgVerseGeometry());
    pattern2 = r'(\w+)\s*=\s*std::make_unique<(\w+)>\(\);'
    
    def replace_func2(match):
        var = match.group(1)
        class_name = match.group(2)
        return f'{var} = std::unique_ptr<{class_name}>(new {class_name}());'
    
    content = re.sub(pattern2, replace_func2, content)
    
    return content

def main():
    if len(sys.argv) < 2:
        print("Usage: python fix_osgverse_init_lists.py <file>")
        sys.exit(1)
    
    filename = sys.argv[1]
    
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 应用所有修复
    content = fix_vec3f_init(content)
    content = fix_color_init(content)
    content = fix_vec2f_init(content)
    content = fix_boundbox(content)
    content = fix_make_unique(content)
    
    with open(filename, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Fixed {filename}")

if __name__ == '__main__':
    main()
