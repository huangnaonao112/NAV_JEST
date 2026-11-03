import re, io
toc = io.open(r'C:\c++\fangzhen_3d\fangzhen_3d\build\pyinstaller\tcp_transceiver_gui\Analysis-00.toc',
              encoding='utf-8', errors='replace').read()
# TOC 三元组: ('目标名', '源路径', '种类')
hits = re.findall(r"\('((?:[^'\\]|\\.)*)',\s*'((?:[^'\\]|\\.)*)',\s*'([A-Z]+)'\)", toc)
print('TOC 总条目:', len(hits))
shown = 0
for dest, src, kind in hits:
    d = dest.replace('\\\\', '\\').lower()
    if 'init.tcl' in d or 'tk86t' in d or 'tcl86t' in d:
        print(kind, '| dest:', dest[:80], '| src:', src[:110])
        shown += 1
        if shown > 8:
            break
# rthook 内容
hook = r'C:\c++\fangzhen_3d\fangzhen_3d\build\pyinstaller\tcp_transceiver_gui\localpycs'
import os
# 找运行钩子源文件
import glob
p = r'C:\c++\fangzhen_3d\fangzhen_3d\build\pyinstaller\tcp_transceiver_gui'
for root, dirs, files in os.walk(r'C:\Users\huangnaonao112\AppData\Local\Programs\Python\Python313\Lib\site-packages\PyInstaller\hooks\rthooks'):
    if 'tkinter' in root:
        rp = os.path.join(root, 'pyi_rth__tkinter.py')
        print('=== rthook 内容 ===')
        print(io.open(rp, encoding='utf-8').read())
