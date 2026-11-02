# QML 尺度字面量静态检查（ctest）
#
# 背景：2026-11 出现"整屏绿块/月面轨迹不可见"，根因之一就是 QML 里散落的
# 尺度字面量（0.4 / 30 / ×0.01 / 127.42 / 34.74 / 80000）与点坐标空间不匹配。
# 本脚本把"禁止字面量"变成可执行的检查：新增 QML 渲染代码必须引用
# CoordinateSystem 常量，否则测试直接失败。
#
# 用法: cmake -DSRC_DIR=<repo> -P scripts/qml_unit_lint.cmake

if(NOT SRC_DIR)
    message(FATAL_ERROR "SRC_DIR not set")
endif()

set(QML_ROOT "${SRC_DIR}/resources/qml")
file(GLOB_RECURSE QML_FILES "${QML_ROOT}/*.qml")
list(LENGTH QML_FILES file_count)
if(file_count EQUAL 0)
    message(FATAL_ERROR "未找到 QML 文件: ${QML_ROOT}")
endif()

# 禁止的模式（CMake 正则）：
#   1. thickness: <数字>          → 必须用 CoordinateSystem.* 常量
#   2. 天体/摆位尺度字面量        → 127.42 / 34.74 / 128.42 / 80000
#   3. 手写换算 hack              → radius / 100、* 0.01 出现在半径/坐标上下文
set(FORBIDDEN
    "thickness:[ \t]*[0-9]"
    "127\\.42"
    "34\\.74"
    "128\\.42"
    "80000"
    "scaledRadius:[ \t]*radius[ \t]*/[ \t]*100"
)

set(ERRORS "")
foreach(qml_file ${QML_FILES})
    file(READ "${qml_file}" content)
    string(REPLACE "\n" ";" lines "${content}")
    set(lineno 0)
    foreach(line ${lines})
        math(EXPR lineno "${lineno} + 1")
        # 跳过注释行（// 或 * 开头）
        if(line MATCHES "^[ \t]*//" OR line MATCHES "^[ \t]*\\*")
            continue()
        endif()
        foreach(pat ${FORBIDDEN})
            # 先剥离十六进制颜色（#80000000 之类），避免把色值里的数字误判成尺度
            string(REGEX REPLACE "#[0-9A-Fa-f]+" "" code_line "${line}")
            if(code_line MATCHES "${pat}")
                file(RELATIVE_PATH rel "${SRC_DIR}" "${qml_file}")
                string(APPEND ERRORS "${rel}:${lineno}: 发现尺度字面量 [${pat}] -> ${code_line}\n")
            endif()
        endforeach()
    endforeach()
endforeach()

if(ERRORS)
    message(FATAL_ERROR
        "QML 尺度字面量检查失败。请改用 CoordinateSystem 常量"
        "（docs/坐标系与尺度规范.md）：\n${ERRORS}")
else()
    message(STATUS "QML 尺度字面量检查通过（${file_count} 个文件）")
endif()
