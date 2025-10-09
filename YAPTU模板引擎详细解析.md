# YAPTU模板引擎详细解析

## 1. YAPTU概念和起源

**YAPTU**（Yet Another Python Templating Utility）是一个**轻量级的Python模板引擎**，专门设计用于代码生成任务。它的名称体现了Python社区的幽默传统，类似于YACC（Yet Another Compiler Compiler）。

### 1.1 YAPTU的设计理念

```python
"Yet Another Python Templating Utility, Version 1.2"
```

YAPTU的核心设计理念：
- **简洁性**: 最小化的语法，易于学习和使用
- **强大性**: 支持复杂的控制结构和表达式
- **嵌入性**: 可以嵌入到任何Python程序中
- **灵活性**: 可自定义语法模式和处理逻辑

### 1.2 与其他模板引擎的对比

| 特性 | YAPTU | Jinja2 | Django Templates | Mako |
|------|-------|--------|------------------|------|
| **复杂度** | 极简 | 中等 | 中等 | 高 |
| **语法** | @变量@ + 控制符 | {{ 变量 }} | {{ 变量 }} | ${ 变量 } |
| **控制结构** | + if / = else / - | {% if %} | {% if %} | % if |
| **代码生成** | ✅ 专门优化 | ✅ 支持 | ❌ 不适合 | ✅ 支持 |
| **学习曲线** | 极低 | 低 | 低 | 中 |

## 2. YAPTU核心架构

### 2.1 核心类：copier

YAPTU的核心是`copier`类，它是一个**智能复制器**：

```python
class copier:
    """Smart-copier (YAPTU) class"""
    
    def __init__(self, 
                 regex=_never,      # 变量替换正则表达式
                 dict=None,         # 全局变量字典
                 restat=_never,     # 语句开始正则
                 restend=_never,    # 语句结束正则  
                 recont=_never,     # 语句继续正则
                 preproc=identity,  # 预处理函数
                 handle=nohandle,   # 错误处理函数
                 ouf=sys.stdout):   # 输出文件
        
        self.regex = regex          # @变量@的匹配模式
        self.globals = dict or {}   # 全局变量空间
        self.locals = {"_cb": self.copyblock}  # 局部变量空间
        self.restat = restat        # + if, + for 的匹配模式
        self.restend = restend      # - 的匹配模式
        self.recont = recont        # = else, = elif 的匹配模式
        self.preproc = preproc      # 预处理器
        self.handle = handle        # 异常处理器
        self.ouf = ouf             # 输出流
```

### 2.2 正则表达式模式

```python
# FreeCAD中使用的YAPTU配置
import re

# 变量替换模式：@变量名@
rex = re.compile(r"@([^@]+)@")

# 控制语句模式
rbe = re.compile(r"\+")        # 语句开始：+ if, + for
ren = re.compile(r"-")         # 语句结束：-
rco = re.compile(r"= ")        # 语句继续：= else, = elif

# 创建YAPTU处理器
cop = copier(rex, template_vars, rbe, ren, rco)
```

## 3. YAPTU语法详解

### 3.1 变量替换语法

```python
# 模板语法：@变量@
template = """
类名: @className@
命名空间: @namespace@
包含文件: @includeFile@
完整类型: @namespace@::@className@
"""

# 变量字典
variables = {
    'className': 'DocumentObjectPy',
    'namespace': 'App', 
    'includeFile': 'App/DocumentObject.h'
}

# 处理结果
"""
类名: DocumentObjectPy
命名空间: App
包含文件: App/DocumentObject.h
完整类型: App::DocumentObjectPy
"""
```

### 3.2 表达式计算

YAPTU支持在`@`符号内执行Python表达式：

```python
# 复杂表达式示例
template = """
大写类名: @className.upper()@
命名空间路径: @namespace.replace("::", "/")@
头文件保护: @namespace.upper().replace("::", "_")@_@className.upper()@_H
条件值: @"yes" if hasConstructor else "no"@
"""

variables = {
    'className': 'DocumentObjectPy',
    'namespace': 'App::Gui',
    'hasConstructor': True
}

# 结果
"""
大写类名: DOCUMENTOBJECTPY
命名空间路径: App/Gui
头文件保护: APP_GUI_DOCUMENTOBJECTPY_H
条件值: yes
"""
```

### 3.3 控制结构语法

#### A. 条件语句

```python
# 条件语句模板
template = """
+ if (hasConstructor):
    // 构造函数实现
    @className@(@cppClassName@ *obj) : @parentClass@(obj) {}
= else:
    // 禁止构造
    static PyObject *PyMake(PyTypeObject *, PyObject *, PyObject *) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot create @className@ directly");
        return nullptr;
    }
-
"""

variables = {
    'hasConstructor': True,
    'className': 'DocumentObjectPy',
    'cppClassName': 'DocumentObject', 
    'parentClass': 'TransactionalObjectPy'
}
```

#### B. 循环语句

```python
# 循环语句模板
template = """
// 方法声明
+ for method in methods:
    PyObject* @method.name@(@method.paramString@)@" const" if method.isConst else ""@;
-

// 方法表
PyMethodDef @className@::Methods[] = {
+ for method in methods:
    {"@method.name@", (PyCFunction) staticCallback_@method.name@, @method.flags@,
     "@method.docString@"},
-
    {nullptr, nullptr, 0, nullptr}
};
"""

variables = {
    'className': 'DocumentObjectPy',
    'methods': [
        {'name': 'getName', 'paramString': 'PyObject *args', 'isConst': True, 'flags': 'METH_NOARGS', 'docString': 'Get object name'},
        {'name': 'touch', 'paramString': 'PyObject *args', 'isConst': False, 'flags': 'METH_VARARGS', 'docString': 'Mark as touched'}
    ]
}
```

#### C. 嵌套控制结构

```python
# 嵌套控制结构
template = """
+ for cls in classes:
    class @cls.name@ {
    + if cls.hasProperties:
        // 属性定义
        + for prop in cls.properties:
            @prop.type@ @prop.name@;
        -
    -
    + if cls.hasMethods:
        // 方法定义  
        + for method in cls.methods:
            + if method.isVirtual:
                virtual @method.returnType@ @method.name@(@method.params@) = 0;
            = else:
                @method.returnType@ @method.name@(@method.params@);
            -
        -
    -
    };
-
"""
```

## 4. YAPTU处理流程详解

### 4.1 模板处理的核心算法

```python
def copyblock(self, cur_line=0, last=None):
    """YAPTU的核心处理算法"""
    
    # 变量替换函数
    def repl(match, self=self):
        expr = self.preproc(match.group(1), "eval")  # 获取@...@中的表达式
        try:
            # 在指定的上下文中执行Python表达式
            return str(eval(expr, self.globals, self.locals))
        except Exception:
            return str(self.handle(expr))
    
    block = self.locals["_bl"]  # 模板行列表
    if last is None:
        last = len(block)
    
    while cur_line < last:
        line = block[cur_line]
        
        # 检查是否是控制语句（以+开始）
        match = self.restat.match(line)
        if match:
            # 这是一个控制语句：+ if, + for等
            stat = match.string[match.end(0):].strip()  # 提取控制语句内容
            
            # 查找对应的结束标记(-)
            j = cur_line + 1
            nest = 1  # 嵌套层级计数
            
            while j < last:
                line = block[j]
                
                if self.restend.match(line):  # 找到结束标记-
                    nest -= 1
                    if nest == 0:
                        break  # 找到匹配的结束标记
                        
                elif self.restat.match(line):  # 找到嵌套的开始标记+
                    nest += 1
                    
                elif nest == 1:  # 在当前层级查找继续语句
                    cont_match = self.recont.match(line)
                    if cont_match:  # 找到= else, = elif
                        nestat = cont_match.string[cont_match.end(0):].strip()
                        # 构建复合控制语句
                        stat = f"{stat} _cb({cur_line + 1},{j})\n{nestat}"
                        cur_line = j
                j += 1
            
            # 执行控制语句
            stat = self.preproc(stat, "exec")
            stat = f"{stat} _cb({cur_line + 1},{j})"
            exec(stat, self.globals, self.locals)
            cur_line = j + 1
            
        else:
            # 普通行：进行变量替换
            processed_line = self.regex.sub(repl, line)
            self.ouf.write(processed_line.encode("utf8"))
            cur_line += 1
```

### 4.2 递归回调机制

YAPTU使用`_cb`（callback）函数实现递归处理：

```python
# _cb函数的作用
self.locals["_cb"] = self.copyblock  # 将copyblock注册为_cb

# 当执行控制语句时：
stat = f"{stat} _cb({cur_line + 1},{j})"  # 添加递归调用

# 例如：+ if condition: 变成 if condition: _cb(start_line, end_line)
# 这样可以递归处理控制块内的内容
```

## 5. FreeCAD中的YAPTU应用实例

### 5.1 实际模板示例

```cpp
// FreeCAD中的实际模板片段
#ifndef @self.export.Namespace.upper().replace("::", "_")@_@self.export.Name.upper()@_H
#define @self.export.Namespace.upper().replace("::", "_")@_@self.export.Name.upper()@_H

#include <@self.export.FatherInclude@>
#include <@self.export.Include@>

@self.export.ForwardDeclarations@

namespace @self.export.Namespace.replace("::"," { namespace ")@ {

class @self.export.Namespace.replace("::","_")@Export @self.export.Name@ : public @self.export.FatherNamespace@::@self.export.Father@
{
    Py_Header

public:
    static PyTypeObject Type;
    static PyMethodDef Methods[];
+ if (self.export.Attribute):
    static PyGetSetDef GetterSetter[];
-
    PyTypeObject *GetType() const override {return &Type;}

public:
    @self.export.Name@(@self.export.TwinPointer@ *pcObject, PyTypeObject *T = &Type);
    
+ if (self.export.Constructor):
    static PyObject *PyMake(PyTypeObject *, PyObject *, PyObject *);
-
    
    using PointerType = @self.export.TwinPointer@*;
    
    // 方法回调声明
+ for i in self.export.Methode:
+ if i.Keyword:
    static PyObject * staticCallback_@i.Name@ (PyObject *self, PyObject *args, PyObject *kwd);
= else:
    static PyObject * staticCallback_@i.Name@ (PyObject *self, PyObject *args);
-
-

    // 方法实现声明
+ for i in self.export.Methode:
+ if i.Const:
    PyObject* @i.Name@(@i.ParameterString@) const;
= else:
    PyObject* @i.Name@(@i.ParameterString@);
-
-

    // 获取C++对象指针
    @self.export.TwinPointer@ *get@self.export.Twin@Ptr() const;
};

} // namespace @self.export.Namespace@

#endif
```

### 5.2 变量上下文

```python
# FreeCAD模板处理时的变量上下文
template_context = {
    'self': {
        'export': {
            'Name': 'DocumentObjectPy',
            'Twin': 'DocumentObject', 
            'TwinPointer': 'DocumentObject',
            'Include': 'App/DocumentObject.h',
            'FatherInclude': 'App/TransactionalObjectPy.h',
            'Namespace': 'App',
            'FatherNamespace': 'App',
            'Father': 'TransactionalObjectPy',
            'Constructor': False,
            'Attribute': [
                {'Name': 'Name', 'Type': 'String', 'ReadOnly': True},
                {'Name': 'Label', 'Type': 'String', 'ReadOnly': False}
            ],
            'Methode': [
                {
                    'Name': 'getName',
                    'Const': True,
                    'Keyword': False,
                    'ParameterString': 'PyObject *args'
                },
                {
                    'Name': 'touch', 
                    'Const': False,
                    'Keyword': False,
                    'ParameterString': 'PyObject *args'
                }
            ]
        }
    }
}
```

## 6. YAPTU语法深度解析

### 6.1 变量替换机制

```python
# 变量替换的内部实现
def repl(match, self=self):
    """变量替换函数"""
    expr = self.preproc(match.group(1), "eval")  # 提取@...@中的内容
    try:
        # 在模板上下文中执行Python表达式
        result = eval(expr, self.globals, self.locals)
        return str(result)
    except Exception:
        return str(self.handle(expr))

# 示例替换过程
"""
模板: @self.export.Name@
表达式: self.export.Name
上下文: {'self': {'export': {'Name': 'DocumentObjectPy'}}}
结果: DocumentObjectPy
"""
```

### 6.2 控制结构解析

```python
# 控制结构的解析逻辑
def parse_control_structure(self, cur_line, block):
    """解析控制结构"""
    
    line = block[cur_line]
    match = self.restat.match(line)  # 匹配+ if, + for等
    
    if match:
        # 提取控制语句
        stat = match.string[match.end(0):].strip()
        print(f"发现控制语句: {stat}")
        
        # 查找结束标记和继续语句
        j = cur_line + 1
        nest = 1
        continue_statements = []
        
        while j < len(block):
            line = block[j]
            
            if self.restend.match(line):  # 找到-
                nest -= 1
                if nest == 0:
                    break
                    
            elif self.restat.match(line):  # 找到嵌套的+
                nest += 1
                
            elif nest == 1:  # 在当前层级
                cont_match = self.recont.match(line)
                if cont_match:  # 找到= else, = elif
                    continue_stat = cont_match.string[cont_match.end(0):].strip()
                    continue_statements.append((j, continue_stat))
            j += 1
        
        return stat, continue_statements, j

# 示例控制结构解析
"""
+ if (condition):
    内容1
= elif (condition2):  
    内容2
= else:
    内容3
-

解析结果:
- 主语句: if (condition)
- 继续语句: [(行号, "elif (condition2)"), (行号, "else")]
- 结束行: -的行号
"""
```

### 6.3 嵌套处理

```python
# 嵌套控制结构的处理
def handle_nested_structure():
    """处理嵌套的控制结构"""
    
    template = """
+ for cls in classes:
    class @cls.name@ {
    + if cls.hasMethods:
        + for method in cls.methods:
            @method.returnType@ @method.name@();
        -
    -
    };
-
    """
    
    # 嵌套层级追踪
    nest_level = 0
    for line in template.split('\n'):
        if line.strip().startswith('+'):
            nest_level += 1
            print(f"{'  ' * (nest_level-1)}开始控制块: {line.strip()}")
        elif line.strip() == '-':
            print(f"{'  ' * (nest_level-1)}结束控制块")
            nest_level -= 1
        elif line.strip().startswith('='):
            print(f"{'  ' * (nest_level-1)}继续控制块: {line.strip()}")
        else:
            print(f"{'  ' * nest_level}内容: {line}")
```

## 7. 高级特性

### 7.1 自定义预处理器

```python
def custom_preprocessor(expr, mode):
    """自定义预处理器"""
    
    # 添加常用的导入和函数
    if mode == "eval":
        # 为表达式求值添加常用函数
        enhanced_expr = f"""
import re, os
def camelToSnake(name):
    return re.sub(r'(?<!^)(?=[A-Z])', '_', name).lower()

def upperFirst(name):
    return name[0].upper() + name[1:] if name else ""

{expr}
"""
        return enhanced_expr
    
    elif mode == "exec":
        # 为语句执行添加辅助函数
        return expr
    
    return expr

# 使用自定义预处理器
cop = copier(regex=rex, dict=template_vars, 
            restat=rbe, restend=ren, recont=rco,
            preproc=custom_preprocessor)
```

### 7.2 错误处理机制

```python
def custom_error_handler(expr):
    """自定义错误处理器"""
    print(f"模板表达式执行失败: {expr}")
    
    # 尝试提供有用的错误信息
    try:
        # 重新执行以获取详细错误
        eval(expr)
    except NameError as e:
        return f"<!-- 变量未定义: {e} -->"
    except SyntaxError as e:
        return f"<!-- 语法错误: {e} -->"
    except Exception as e:
        return f"<!-- 未知错误: {e} -->"

# 使用自定义错误处理器
cop = copier(regex=rex, dict=template_vars,
            handle=custom_error_handler)
```

## 8. 性能优化和最佳实践

### 8.1 正则表达式优化

```python
# 优化的正则表达式模式
import re

# 编译一次，多次使用
VARIABLE_PATTERN = re.compile(r"@([^@]+)@", re.MULTILINE)
STATEMENT_START = re.compile(r"^\+\s*", re.MULTILINE)
STATEMENT_END = re.compile(r"^-\s*$", re.MULTILINE)  
STATEMENT_CONTINUE = re.compile(r"^=\s*", re.MULTILINE)

class OptimizedCopier(copier):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # 使用预编译的正则表达式
        self.regex = VARIABLE_PATTERN
        self.restat = STATEMENT_START
        self.restend = STATEMENT_END
        self.recont = STATEMENT_CONTINUE
```

### 8.2 模板缓存

```python
class TemplateCache:
    """模板缓存系统"""
    
    def __init__(self):
        self.parsed_templates = {}
        self.template_mtimes = {}
    
    def get_processed_template(self, template_file, variables):
        """获取处理后的模板，使用缓存"""
        
        # 检查文件修改时间
        current_mtime = os.path.getmtime(template_file)
        cached_mtime = self.template_mtimes.get(template_file, 0)
        
        cache_key = (template_file, hash(str(sorted(variables.items()))))
        
        if cache_key in self.parsed_templates and current_mtime <= cached_mtime:
            return self.parsed_templates[cache_key]
        
        # 重新处理模板
        with open(template_file, 'r') as f:
            template_content = f.read()
        
        import io
        output = io.StringIO()
        
        cop = copier(regex=VARIABLE_PATTERN, dict=variables, 
                    restat=STATEMENT_START, restend=STATEMENT_END, 
                    recont=STATEMENT_CONTINUE, ouf=output)
        
        template_lines = [line + '\n' for line in template_content.split('\n')]
        cop.copy(template_lines)
        
        result = output.getvalue()
        
        # 更新缓存
        self.parsed_templates[cache_key] = result
        self.template_mtimes[template_file] = current_mtime
        
        return result
```

## 9. 调试和开发工具

### 9.1 模板调试器

```python
class YAPTUDebugger:
    """YAPTU模板调试器"""
    
    def __init__(self, template_content, variables):
        self.template_content = template_content
        self.variables = variables
        self.debug_output = []
    
    def debug_process(self):
        """调试模板处理过程"""
        
        class DebugCopier(copier):
            def __init__(self, debugger, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.debugger = debugger
            
            def copyblock(self, cur_line=0, last=None):
                self.debugger.debug_output.append(f"处理块: 行{cur_line}-{last}")
                
                def debug_repl(match, self=self):
                    expr = match.group(1)
                    self.debugger.debug_output.append(f"  替换: @{expr}@")
                    try:
                        result = eval(expr, self.globals, self.locals)
                        self.debugger.debug_output.append(f"    结果: {result}")
                        return str(result)
                    except Exception as e:
                        self.debugger.debug_output.append(f"    错误: {e}")
                        return f"ERROR:{e}"
                
                # 临时替换替换函数
                original_sub = self.regex.sub
                self.regex.sub = debug_repl
                
                # 调用原始处理逻辑
                super().copyblock(cur_line, last)
                
                # 恢复原始函数
                self.regex.sub = original_sub
        
        import io
        output = io.StringIO()
        template_lines = self.template_content.split('\n')
        
        debugger = DebugCopier(self, 
                              regex=VARIABLE_PATTERN,
                              dict=self.variables,
                              restat=STATEMENT_START,
                              restend=STATEMENT_END,
                              recont=STATEMENT_CONTINUE,
                              ouf=output)
        
        debugger.copy([line + '\n' for line in template_lines])
        
        return output.getvalue(), self.debug_output

# 使用调试器
debugger = YAPTUDebugger(template_content, template_variables)
result, debug_info = debugger.debug_process()

print("调试信息:")
for info in debug_info:
    print(info)
```

### 9.2 语法验证器

```python
def validate_yaptu_syntax(template_content):
    """验证YAPTU模板语法"""
    
    lines = template_content.split('\n')
    errors = []
    stack = []  # 控制结构栈
    
    for line_num, line in enumerate(lines, 1):
        stripped = line.strip()
        
        # 检查控制语句
        if stripped.startswith('+'):
            # 开始控制语句
            control_type = stripped[1:].strip().split()[0]
            stack.append((line_num, control_type, stripped))
            
        elif stripped == '-':
            # 结束控制语句
            if not stack:
                errors.append(f"行{line_num}: 意外的结束标记'-'，没有对应的开始标记")
            else:
                stack.pop()
                
        elif stripped.startswith('='):
            # 继续控制语句
            if not stack:
                errors.append(f"行{line_num}: 意外的继续标记'='，没有对应的开始标记")
        
        # 检查变量语法
        import re
        var_matches = re.findall(r'@([^@]*)@', line)
        for var_expr in var_matches:
            if not var_expr.strip():
                errors.append(f"行{line_num}: 空的变量表达式 @@")
    
    # 检查未闭合的控制结构
    for line_num, control_type, statement in stack:
        errors.append(f"行{line_num}: 未闭合的控制结构 '{statement}'")
    
    return errors

# 使用语法验证器
template = """
+ if condition:
    @variable@
= else:
    @other_variable@
// 缺少结束标记 -
"""

errors = validate_yaptu_syntax(template)
if errors:
    print("模板语法错误:")
    for error in errors:
        print(f"  {error}")
```

## 10. YAPTU与现代模板引擎的比较

### 10.1 语法对比

```python
# YAPTU语法
"""
+ if condition:
    Hello @name@!
= else:
    Goodbye @name@!
-
"""

# Jinja2语法
"""
{% if condition %}
    Hello {{ name }}!
{% else %}
    Goodbye {{ name }}!
{% endif %}
"""

# Mako语法
"""
% if condition:
    Hello ${name}!
% else:
    Goodbye ${name}!
% endif
"""
```

### 10.2 优势分析

| 方面 | YAPTU优势 | 其他引擎优势 |
|------|-----------|-------------|
| **学习成本** | 极低，只需掌握几个符号 | 功能更丰富但复杂 |
| **代码生成** | 专门为代码生成优化 | 通用模板功能 |
| **嵌入性** | 轻量，易于集成 | 功能完整但较重 |
| **定制性** | 高度可定制语法 | 固定语法规则 |
| **调试** | 简单直接 | 更好的调试工具 |

## 11. 实际应用场景

### 11.1 代码生成

```python
# 生成C++类的模板
class_template = """
// 自动生成的@className@类
class @className@ {
private:
+ for member in privateMembers:
    @member.type@ @member.name@;
-

public:
    // 构造函数
    @className@();
    
    // 方法
+ for method in publicMethods:
+ if method.isVirtual:
    virtual @method.returnType@ @method.name@(@method.parameters@)@" = 0" if method.isPure else ""@;
= else:
    @method.returnType@ @method.name@(@method.parameters@);
-
-
};
"""
```

### 11.2 配置文件生成

```python
# 生成CMake配置文件
cmake_template = """
# 自动生成的CMake文件
cmake_minimum_required(VERSION @minCMakeVersion@)

project(@projectName@)

# 源文件
set(SOURCES
+ for source in sourceFiles:
    @source@
-
)

# 头文件
set(HEADERS  
+ for header in headerFiles:
    @header@
-
)

# 创建库
add_library(@projectName@ ${SOURCES} ${HEADERS})

# 链接库
+ if linkLibraries:
target_link_libraries(@projectName@
+ for lib in linkLibraries:
    @lib@
-
)
-
"""
```

## 12. 总结

YAPTU是一个**精巧而强大的模板引擎**，特别适合代码生成任务：

### 12.1 核心特点

1. **极简语法**: 只需掌握`@变量@`、`+`、`=`、`-`四个符号
2. **强大功能**: 支持变量替换、条件语句、循环、嵌套结构
3. **Python集成**: 可以执行任意Python表达式
4. **高度定制**: 所有语法模式都可以自定义
5. **轻量级**: 核心实现不到200行代码

### 12.2 在FreeCAD中的价值

- **自动化代码生成**: 生成数千行Python绑定代码
- **模板驱动开发**: 通过模板确保代码一致性
- **维护简化**: 修改模板即可更新所有生成的代码
- **错误减少**: 自动生成避免手工编码错误

### 12.3 设计智慧

YAPTU体现了**Unix哲学**的精髓：
- **做好一件事**: 专注于模板处理
- **简单高效**: 最小化的语法和实现
- **可组合**: 可以与其他工具链组合使用
- **可扩展**: 支持自定义和扩展

YAPTU虽然名为"Yet Another"，但它在**代码生成领域**确实是一个**独特而优秀的解决方案**，为FreeCAD的跨语言绑定系统提供了强大而灵活的基础！

---

**文档版本**: 1.0  
**分析基于**: FreeCAD YAPTU实现源代码  
**最后更新**: 2024年10月8日
