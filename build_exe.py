"""
PyInstaller build script for creating Windows executable.
"""

import PyInstaller.__main__
from pathlib import Path

# Get the project root directory
project_root = Path(__file__).parent
src_dir = project_root / "src"
templates_dir = src_dir / "templates"

# PyInstaller arguments
args = [
    'src/main.py',  # Main script
    '--name=TownSceneGenerator',  # Executable name
    '--onefile',  # Create single executable file
    '--windowed',  # No console window (GUI only)
    '--clean',  # Clean PyInstaller cache before building
    '--noconfirm',  # Overwrite output directory without asking
    
    # Add data files (templates)
    f'--add-data={templates_dir}{Path.pathsep}{templates_dir}',
    
    # Icon (if you have one)
    # '--icon=icon.ico',
    
    # Hidden imports (if needed)
    '--hidden-import=PyQt6.QtCore',
    '--hidden-import=PyQt6.QtGui',
    '--hidden-import=PyQt6.QtWidgets',
]

if __name__ == "__main__":
    print("Building executable with PyInstaller...")
    print(f"Project root: {project_root}")
    print(f"Source directory: {src_dir}")
    print("\nPyInstaller arguments:")
    for arg in args:
        print(f"  {arg}")
    print("\nStarting build...\n")
    
    PyInstaller.__main__.run(args)
    
    print("\nBuild complete!")
    print(f"Executable should be in: {project_root / 'dist' / 'TownSceneGenerator.exe'}")

