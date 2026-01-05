"""
MP3 Player File Manager for detecting drives and copying files to MP3 player.
"""

import os
import shutil
import string
from pathlib import Path
from typing import List, Optional, Callable


class MP3FileManager:
    """Manages file operations for MP3 player."""
    
    def __init__(self):
        self.current_drive: Optional[str] = None
    
    def get_available_drives(self) -> List[str]:
        """
        Get list of available Windows drive letters.
        
        Returns:
            List of drive letters (e.g., ['C:', 'D:', 'E:'])
        """
        drives = []
        for letter in string.ascii_uppercase:
            drive = f"{letter}:"
            if os.path.exists(drive):
                drives.append(drive)
        return drives
    
    def validate_drive(self, drive_letter: str) -> bool:
        """
        Validate that a drive letter exists and is accessible.
        
        Args:
            drive_letter: Drive letter (e.g., 'E:' or 'E')
            
        Returns:
            True if drive is valid and accessible
        """
        # Normalize drive letter
        if not drive_letter.endswith(':'):
            drive_letter = f"{drive_letter}:"
        drive_letter = drive_letter.upper()
        
        if not os.path.exists(drive_letter):
            return False
        
        # Try to access the drive
        try:
            os.listdir(drive_letter)
            return True
        except (OSError, PermissionError):
            return False
    
    def get_drive_path(self, drive_letter: str) -> Path:
        """
        Get Path object for drive letter.
        
        Args:
            drive_letter: Drive letter (e.g., 'E:' or 'E')
            
        Returns:
            Path object for the drive
        """
        if not drive_letter.endswith(':'):
            drive_letter = f"{drive_letter}:"
        return Path(drive_letter)
    
    def copy_file_to_mp3(self, source_file: Path, drive_letter: str,
                         destination_folder: str = "", overwrite: bool = True,
                         progress_callback: Optional[Callable[[int, int], None]] = None) -> bool:
        """
        Copy a file to the MP3 player.
        
        Args:
            source_file: Path to source file
            drive_letter: Target drive letter
            destination_folder: Optional subfolder on MP3 player (e.g., 'music')
            overwrite: Whether to overwrite existing files
            progress_callback: Optional callback function(bytes_copied, total_bytes)
            
        Returns:
            True if successful, False otherwise
        """
        if not source_file.exists():
            return False
        
        if not self.validate_drive(drive_letter):
            return False
        
        try:
            drive_path = self.get_drive_path(drive_letter)
            
            # Create destination path
            if destination_folder:
                dest_dir = drive_path / destination_folder
            else:
                dest_dir = drive_path
            
            # Create destination directory if it doesn't exist
            dest_dir.mkdir(parents=True, exist_ok=True)
            
            dest_file = dest_dir / source_file.name
            
            # Check if file exists and handle overwrite
            if dest_file.exists() and not overwrite:
                return False
            
            # Copy file with progress callback if provided
            if progress_callback:
                self._copy_with_progress(source_file, dest_file, progress_callback)
            else:
                shutil.copy2(source_file, dest_file)
            
            return True
        except Exception:
            return False
    
    def _copy_with_progress(self, source: Path, dest: Path,
                           progress_callback: Callable[[int, int], None]):
        """
        Copy file with progress reporting.
        
        Args:
            source: Source file path
            dest: Destination file path
            progress_callback: Callback function(bytes_copied, total_bytes)
        """
        total_size = source.stat().st_size
        bytes_copied = 0
        chunk_size = 1024 * 1024  # 1MB chunks
        
        with open(source, 'rb') as src, open(dest, 'wb') as dst:
            while True:
                chunk = src.read(chunk_size)
                if not chunk:
                    break
                dst.write(chunk)
                bytes_copied += len(chunk)
                progress_callback(bytes_copied, total_size)
    
    def copy_files_to_mp3(self, source_files: List[Path], drive_letter: str,
                       destination_folder: str = "", overwrite: bool = True,
                       progress_callback: Optional[Callable[[int, int, str], None]] = None) -> Dict[str, bool]:
        """
        Copy multiple files to the MP3 player.
        
        Args:
            source_files: List of source file paths
            drive_letter: Target drive letter
            destination_folder: Optional subfolder on MP3 player
            overwrite: Whether to overwrite existing files
            progress_callback: Optional callback(file_index, total_files, filename)
            
        Returns:
            Dictionary mapping file paths to success status
        """
        results = {}
        total_files = len(source_files)
        
        for i, source_file in enumerate(source_files):
            if progress_callback:
                progress_callback(i + 1, total_files, source_file.name)
            
            success = self.copy_file_to_mp3(
                source_file, drive_letter, destination_folder, overwrite
            )
            results[str(source_file)] = success
        
        return results
    
    def get_drive_space(self, drive_letter: str) -> Optional[Dict[str, int]]:
        """
        Get available and total space on drive.
        
        Args:
            drive_letter: Drive letter to check
            
        Returns:
            Dictionary with 'total' and 'free' bytes, or None if error
        """
        if not self.validate_drive(drive_letter):
            return None
        
        try:
            drive_path = self.get_drive_path(drive_letter)
            stat = shutil.disk_usage(drive_path)
            return {
                'total': stat.total,
                'free': stat.free,
                'used': stat.used
            }
        except Exception:
            return None
    
    def check_file_exists(self, drive_letter: str, filename: str,
                         destination_folder: str = "") -> bool:
        """
        Check if a file exists on the MP3 player.
        
        Args:
            drive_letter: Drive letter
            filename: Name of file to check
            destination_folder: Optional subfolder
            
        Returns:
            True if file exists
        """
        if not self.validate_drive(drive_letter):
            return False
        
        try:
            drive_path = self.get_drive_path(drive_letter)
            if destination_folder:
                file_path = drive_path / destination_folder / filename
            else:
                file_path = drive_path / filename
            return file_path.exists()
        except Exception:
            return False

