#!/usr/bin/env python3
"""
Fix Base::Vector3f explicit constructor issues in OsgVerse code
"""

import re
import sys

def fix_file(filename):
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Fix push_back with initializer lists: vertices.push_back({x, y, z});
    # -> vertices.push_back(Vec3f(x, y, z));
    content = re.sub(
        r'(vertices|normals)\.push_back\(\{([^}]+)\}\);',
        r'\1.push_back(Vec3f(\2));',
        content
    )
    
    # Fix vector initialization with nested initializer lists
    # std::vector<Vec3f> vertices = {{x,y,z}, {x,y,z}, ...};
    # This is more complex, need to handle line by line
    lines = content.split('\n')
    fixed_lines = []
    in_vector_init = False
    vector_init_buffer = []
    
    for line in lines:
        # Check if this line starts a vector initialization
        if re.search(r'std::vector<Vec3f>\s+\w+\s*=\s*\{', line):
            in_vector_init = True
            vector_init_buffer = [line]
        elif in_vector_init:
            vector_init_buffer.append(line)
            if '};' in line:
                # End of vector initialization
                full_init = '\n'.join(vector_init_buffer)
                # Replace {x,y,z} with Vec3f(x,y,z)
                full_init = re.sub(r'\{(-?[\d.]+f?),\s*(-?[\d.]+f?),\s*(-?[\d.]+f?)\}', 
                                   r'Vec3f(\1, \2, \3)', full_init)
                fixed_lines.append(full_init)
                in_vector_init = False
                vector_init_buffer = []
        else:
            fixed_lines.append(line)
    
    content = '\n'.join(fixed_lines)
    
    with open(filename, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Fixed {filename}")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python fix_vec3f_explicit.py <file>")
        sys.exit(1)
    
    fix_file(sys.argv[1])
