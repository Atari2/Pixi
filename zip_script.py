from zipfile import ZipFile
import glob
from pathlib import Path

executable_path = 'build/Pixi/MinSizeRel/Pixi.exe'
cfg_editor_path = 'CFGEditor/CFG Editor/bin/Release/CFG Editor.exe'
to_zip = [(file, file.split('\\', 1)[-1]) for file in glob.glob('Resources/**/*', recursive=True)]

print(to_zip)

with ZipFile('pixi.zip', 'w') as zpf:
    for real, zipname in to_zip:
        zpf.write(real, arcname=zipname)
    zpf.write(cfg_editor_path, arcname=Path(cfg_editor_path).name)
    zpf.write(executable_path, arcname=Path(executable_path).name)


