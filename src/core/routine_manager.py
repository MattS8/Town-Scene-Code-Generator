"""
Routine Manager for storing and managing light routine configurations.
Handles JSON serialization and deserialization of routine data.
"""

import json
from pathlib import Path
from typing import List, Dict, Optional
from datetime import datetime


class RoutineManager:
    """Manages light routine configurations."""
    
    def __init__(self):
        self.routines: List[Dict] = []
        self.project_path: Optional[Path] = None
    
    def create_routine(self, name: str, wav_file: str, markers: List[Dict], 
                      light_config: Dict) -> Dict:
        """
        Create a new routine.
        
        Args:
            name: Routine name
            wav_file: Path to WAV file
            markers: List of marker dictionaries
            light_config: Light configuration dictionary
            
        Returns:
            Created routine dictionary
        """
        routine = {
            'id': len(self.routines),
            'name': name,
            'wav_file': wav_file,
            'markers': markers,
            'light_config': light_config,
            'created_at': datetime.now().isoformat(),
            'updated_at': datetime.now().isoformat()
        }
        self.routines.append(routine)
        return routine
    
    def update_routine(self, routine_id: int, name: Optional[str] = None,
                      wav_file: Optional[str] = None, markers: Optional[List[Dict]] = None,
                      light_config: Optional[Dict] = None) -> Optional[Dict]:
        """
        Update an existing routine.
        
        Args:
            routine_id: ID of routine to update
            name: New name (optional)
            wav_file: New WAV file path (optional)
            markers: New markers list (optional)
            light_config: New light configuration (optional)
            
        Returns:
            Updated routine dictionary or None if not found
        """
        routine = self.get_routine(routine_id)
        if not routine:
            return None
        
        if name is not None:
            routine['name'] = name
        if wav_file is not None:
            routine['wav_file'] = wav_file
        if markers is not None:
            routine['markers'] = markers
        if light_config is not None:
            routine['light_config'] = light_config
        
        routine['updated_at'] = datetime.now().isoformat()
        return routine
    
    def get_routine(self, routine_id: int) -> Optional[Dict]:
        """Get a routine by ID."""
        for routine in self.routines:
            if routine['id'] == routine_id:
                return routine
        return None
    
    def get_routine_by_filepath(self, filepath: str) -> Optional[Dict]:
        """Get a routine by WAV file path."""
        for routine in self.routines:
            if routine.get('wav_file') == filepath:
                return routine
        return None
    
    def delete_routine_by_filepath(self, filepath: str) -> bool:
        """Delete a routine by WAV file path."""
        for i, routine in enumerate(self.routines):
            if routine.get('wav_file') == filepath:
                self.routines.pop(i)
                return True
        return False
    
    def delete_routine(self, routine_id: int) -> bool:
        """Delete a routine by ID."""
        for i, routine in enumerate(self.routines):
            if routine['id'] == routine_id:
                self.routines.pop(i)
                return True
        return False
    
    def get_all_routines(self) -> List[Dict]:
        """Get all routines."""
        return self.routines.copy()
    
    def clear_routines(self):
        """Clear all routines."""
        self.routines = []
    
    def to_dict(self) -> Dict:
        """Convert routines to dictionary for serialization."""
        return {
            'version': '1.0',
            'created_at': datetime.now().isoformat(),
            'routines': self.routines
        }
    
    def from_dict(self, data: Dict):
        """Load routines from dictionary."""
        if 'routines' in data:
            self.routines = data['routines']
        else:
            self.routines = []
    
    def save_project(self, file_path: Path):
        """
        Save project to JSON file.
        
        Args:
            file_path: Path to save the project file
        """
        data = self.to_dict()
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        self.project_path = file_path
    
    def load_project(self, file_path: Path):
        """
        Load project from JSON file.
        
        Args:
            file_path: Path to the project file
        """
        with open(file_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        self.from_dict(data)
        self.project_path = file_path
    
    def export_routine(self, routine_id: int, file_path: Path):
        """
        Export a single routine to JSON file.
        
        Args:
            routine_id: ID of routine to export
            file_path: Path to save the routine file
        """
        routine = self.get_routine(routine_id)
        if routine:
            with open(file_path, 'w', encoding='utf-8') as f:
                json.dump(routine, f, indent=2, ensure_ascii=False)
    
    def import_routine(self, file_path: Path) -> Optional[Dict]:
        """
        Import a routine from JSON file.
        
        Args:
            file_path: Path to the routine file
            
        Returns:
            Imported routine dictionary or None if failed
        """
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                routine = json.load(f)
            
            # Assign new ID
            routine['id'] = len(self.routines)
            routine['created_at'] = datetime.now().isoformat()
            routine['updated_at'] = datetime.now().isoformat()
            
            self.routines.append(routine)
            return routine
        except Exception:
            return None


# Default light configuration structure
def create_default_light_config() -> Dict:
    """Create a default light configuration structure."""
    return {
        'lights': [],
        'global_pins': {}
    }


def create_light_config(label: str, name: str, pin: str) -> Dict:
    """
    Create a light configuration.
    
    Args:
        label: Human-readable label for the light
        name: Variable name for the light (used in code generation)
        pin: Pin number/identifier (string like "A2" or "3")
        
    Returns:
        Light configuration dictionary
    """
    return {
        'label': label,
        'name': name,
        'pin': pin,
        'marker_actions': {}  # Maps marker ID to pin actions (on/off)
    }

