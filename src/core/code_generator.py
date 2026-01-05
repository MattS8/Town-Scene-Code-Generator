"""
Arduino Code Generator for ESP32 light control routines.
Generates .ino files from routine configurations.
"""

from pathlib import Path
from typing import List, Dict, Optional
import json


class CodeGenerator:
    """Generates Arduino code from routine configurations."""
    
    def __init__(self, template_path: Optional[Path] = None):
        self.template_path = template_path or Path(__file__).parent.parent / 'templates' / 'arduino_template.ino'
    
    def generate_code(self, routines: List[Dict], output_path: Path,
                     mp3_player_type: str = "DFPlayer",
                     mp3_rx_pin: int = 16,
                     mp3_tx_pin: int = 17) -> bool:
        """
        Generate Arduino code from routines.
        
        Args:
            routines: List of routine dictionaries
            output_path: Path to save the generated .ino file
            mp3_player_type: Type of MP3 player (default: DFPlayer)
            mp3_rx_pin: RX pin for MP3 player communication
            mp3_tx_pin: TX pin for MP3 player communication
            
        Returns:
            True if successful, False otherwise
        """
        try:
            # Load template
            if not self.template_path.exists():
                # Generate code without template if template doesn't exist
                code = self._generate_code_direct(routines, mp3_player_type, mp3_rx_pin, mp3_tx_pin)
            else:
                with open(self.template_path, 'r', encoding='utf-8') as f:
                    template = f.read()
                code = self._generate_from_template(template, routines, mp3_player_type, mp3_rx_pin, mp3_tx_pin)
            
            # Write to file
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write(code)
            
            return True
        except Exception as e:
            print(f"Error generating code: {e}")
            return False
    
    def _generate_from_template(self, template: str, routines: List[Dict],
                               mp3_player_type: str, mp3_rx_pin: int, mp3_tx_pin: int) -> str:
        """Generate code from template."""
        # Extract pin configurations
        all_pins = set()
        routine_data = []
        
        for routine in routines:
            pins = []
            marker_data = []
            
            # Collect pins from light config
            light_config = routine.get('light_config', {})
            for light in light_config.get('lights', []):
                pins.extend(light.get('pins', []))
                all_pins.update(light.get('pins', []))
            
            # Collect marker data
            for marker in routine.get('markers', []):
                marker_data.append({
                    'position_ms': marker.get('position_ms', 0),
                    'label': marker.get('label', ''),
                    'id': marker.get('id', 0)
                })
            
            routine_data.append({
                'name': routine.get('name', 'Routine'),
                'wav_file': Path(routine.get('wav_file', '')).name,
                'markers': marker_data,
                'pins': sorted(set(pins)),
                'light_config': light_config
            })
        
        # Replace template placeholders
        code = template
        code = code.replace('{{ROUTINES_JSON}}', json.dumps(routine_data, indent=2))
        code = code.replace('{{ALL_PINS}}', ', '.join(map(str, sorted(all_pins))) if all_pins else '')
        code = code.replace('{{MP3_PLAYER_TYPE}}', mp3_player_type)
        code = code.replace('{{MP3_RX_PIN}}', str(mp3_rx_pin))
        code = code.replace('{{MP3_TX_PIN}}', str(mp3_tx_pin))
        code = code.replace('{{NUM_ROUTINES}}', str(len(routines)))
        
        # Generate pin definitions
        pin_defs = '\n'.join([f"#define PIN_{pin} {pin}" for pin in sorted(all_pins)]) if all_pins else "// No pins defined"
        code = code.replace('{{PIN_DEFINITIONS}}', pin_defs)
        
        return code
    
    def _generate_code_direct(self, routines: List[Dict], mp3_player_type: str,
                             mp3_rx_pin: int, mp3_tx_pin: int) -> str:
        """Generate code directly without template."""
        # Collect all pins
        all_pins = set()
        for routine in routines:
            light_config = routine.get('light_config', {})
            for light in light_config.get('lights', []):
                pins = light.get('pins', [])
                all_pins.update(pins)
        
        # Generate pin definitions
        pin_defs = '\n'.join([f"#define PIN_{pin} {pin}" for pin in sorted(all_pins)])
        
        # Generate routine data
        routine_data = []
        for routine in routines:
            markers = []
            for marker in routine.get('markers', []):
                markers.append({
                    'position_ms': marker.get('position_ms', 0),
                    'label': marker.get('label', ''),
                    'id': marker.get('id', 0)
                })
            
            routine_data.append({
                'name': routine.get('name', 'Routine'),
                'wav_file': Path(routine.get('wav_file', '')).name,
                'markers': markers
            })
        
        code = f"""// Arduino Code Generated for ESP32 Town Scene Controller
// Generated from {len(routines)} routine(s)

#include <SoftwareSerial.h>

// Pin Definitions
{pin_defs}

// MP3 Player Configuration
#define MP3_RX_PIN {mp3_rx_pin}
#define MP3_TX_PIN {mp3_tx_pin}

// Routine Data
const int NUM_ROUTINES = {len(routines)};

// TODO: Implement full code generation
// This is a placeholder - full implementation requires template

void setup() {{
  Serial.begin(115200);
  // Initialize pins
  // Initialize MP3 player
}}

void loop() {{
  // Main loop implementation
}}
"""
        return code
    
    def get_pin_list(self, routines: List[Dict]) -> List[int]:
        """Extract all unique pins from routines."""
        pins = set()
        for routine in routines:
            light_config = routine.get('light_config', {})
            for light in light_config.get('lights', []):
                pins.update(light.get('pins', []))
        return sorted(list(pins))

