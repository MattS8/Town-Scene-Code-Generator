"""
WAV File Parser for extracting CUE point markers from WAV files.
Based on the RIFF chunk format and CUE chunk specification.
"""

import struct
import re
from pathlib import Path
from typing import List, Dict, Optional, Tuple

# Debug flag - set to True to enable detailed debugging output
# Can also be set via environment variable: WAV_PARSER_DEBUG=1
import os
# DEBUG_WAV_PARSER = os.getenv('WAV_PARSER_DEBUG', '0').lower() in ('1', 'true', 'yes')
DEBUG_WAV_PARSER = True


def _debug_print(message: str):
    """Print debug message if debugging is enabled."""
    if DEBUG_WAV_PARSER:
        print(f"[WAV_PARSER_DEBUG] {message}")


class WAVParser:
    """Parser for WAV files with CUE point markers."""
    
    def __init__(self, debug: bool = False):
        """
        Initialize WAV parser.
        
        Args:
            debug: Enable debug output for this parser instance
        """
        self.sample_rate = 0
        self.bits_per_sample = 0
        self.channels = 0
        self.duration_ms = 0
        self.debug = debug or DEBUG_WAV_PARSER
    
    def parse_wav_file(self, file_path: Path) -> Dict:
        """
        Parse a WAV file and extract CUE point markers and lights.
        
        Args:
            file_path: Path to the WAV file
            
        Returns:
            Dictionary containing filename, duration, markers, and lights
        """
        markers = []
        
        if self.debug:
            _debug_print(f"Starting parse of file: {file_path}")
        
        try:
            with open(file_path, 'rb') as f:
                # Read RIFF header
                riff_header = f.read(12)
                if len(riff_header) < 12:
                    raise ValueError("Invalid WAV file: too short")
                
                riff_id, file_size, wave_id = struct.unpack('<4sI4s', riff_header)
                
                if riff_id != b'RIFF' or wave_id != b'WAVE':
                    raise ValueError("Invalid WAV file: not a RIFF WAVE file")
                
                # Read format chunk
                fmt_chunk = self._find_chunk(f, b'fmt ')
                if not fmt_chunk:
                    raise ValueError("Invalid WAV file: fmt chunk not found")
                
                fmt_data = f.read(16)
                audio_format, self.channels, self.sample_rate, byte_rate, \
                    block_align, self.bits_per_sample = struct.unpack('<HHIIHH', fmt_data)
                
                # Read data chunk to get file duration
                data_chunk = self._find_chunk(f, b'data')
                if not data_chunk:
                    raise ValueError("Invalid WAV file: data chunk not found")
                
                data_size = data_chunk[1]
                bytes_per_sample = (self.bits_per_sample // 8) * self.channels
                if bytes_per_sample > 0:
                    total_samples = data_size // bytes_per_sample
                    self.duration_ms = int((total_samples / self.sample_rate) * 1000)
                
                # Read CUE chunk
                f.seek(0)  # Reset to beginning
                cue_chunk = self._find_chunk(f, b'cue ')
                if cue_chunk:
                    if self.debug:
                        _debug_print(f"Found CUE chunk at offset {cue_chunk[0]}, size {cue_chunk[1]}")
                    markers = self._parse_cue_chunk(f, cue_chunk)
                else:
                    if self.debug:
                        _debug_print("No CUE chunk found in file")
                
                # Calculate start and end times for each marker
                markers = self._calculate_marker_times(markers)
                
        except Exception as e:
            if self.debug:
                _debug_print(f"Exception during parsing: {str(e)}")
            raise ValueError(f"Error parsing WAV file: {str(e)}")
        
        if self.debug:
            _debug_print(f"Parsed {len(markers)} markers from file")
            for i, marker in enumerate(markers):
                _debug_print(f"  Marker {i}: id={marker['id']}, label='{marker['label']}', "
                           f"pos={marker['position_ms']}ms, start={marker.get('start_ms', 'N/A')}ms, "
                           f"end={marker.get('end_ms', 'N/A')}ms")
        
        # Extract lights from marker labels
        lights = self._extract_lights_from_markers(markers)
        
        if self.debug:
            _debug_print(f"Extracted {len(lights)} unique lights")
            for i, light in enumerate(lights):
                _debug_print(f"  Light {i}: name='{light['name']}', pin='{light['pin']}', special={light['is_special']}")
        
        return {
            'filename': file_path.name,
            'filepath': str(file_path),
            'duration_ms': self.duration_ms,
            'sample_rate': self.sample_rate,
            'channels': self.channels,
            'bits_per_sample': self.bits_per_sample,
            'markers': markers,
            'lights': lights
        }
    
    def _find_chunk(self, file_handle, chunk_id: bytes) -> Optional[Tuple[int, int]]:
        """
        Find a chunk in the WAV file.
        
        Args:
            file_handle: Open file handle
            chunk_id: 4-byte chunk identifier
            
        Returns:
            Tuple of (chunk_offset, chunk_size) or None if not found
        """
        return self._find_chunk_from_offset(file_handle, chunk_id, 12)
    
    def _find_chunk_from_offset(self, file_handle, chunk_id: bytes, start_offset: int) -> Optional[Tuple[int, int]]:
        """
        Find a chunk in the WAV file starting from a specific offset.
        
        Args:
            file_handle: Open file handle
            chunk_id: 4-byte chunk identifier
            start_offset: Offset to start searching from
            
        Returns:
            Tuple of (chunk_offset, chunk_size) or None if not found
        """
        file_handle.seek(start_offset)
        
        while True:
            chunk_header = file_handle.read(8)
            if len(chunk_header) < 8:
                return None
            
            current_id, chunk_size = struct.unpack('<4sI', chunk_header)
            
            if current_id == chunk_id:
                return (file_handle.tell() - 8, chunk_size)
            
            # Skip to next chunk (handle odd-sized chunks)
            file_handle.seek(chunk_size + (chunk_size % 2), 1)
    
    def _parse_cue_chunk(self, file_handle, cue_chunk: Tuple[int, int]) -> List[Dict]:
        """
        Parse the CUE chunk to extract marker information.
        
        Args:
            file_handle: Open file handle
            cue_chunk: Tuple of (offset, size) for the CUE chunk
            
        Returns:
            List of marker dictionaries
        """
        markers = []
        chunk_offset, chunk_size = cue_chunk
        
        file_handle.seek(chunk_offset + 8)  # Skip chunk header
        cue_data = file_handle.read(chunk_size)
        
        if len(cue_data) < 4:
            if self.debug:
                _debug_print("CUE chunk data too small, returning empty markers")
            return markers
        
        # Read number of cue points
        num_cues = struct.unpack('<I', cue_data[0:4])[0]
        
        if self.debug:
            _debug_print(f"CUE chunk contains {num_cues} cue points")
        
        # CUE point structure: dwIdentifier (4), dwPosition (4), fccChunk (4),
        # dwChunkStart (4), dwBlockStart (4), dwSampleOffset (4)
        cue_point_size = 24
        
        for i in range(num_cues):
            offset = 4 + (i * cue_point_size)
            if offset + cue_point_size > len(cue_data):
                break
            
            cue_point_data = cue_data[offset:offset + cue_point_size]
            cue_id, position, fcc_chunk, chunk_start, block_start, sample_offset = \
                struct.unpack('<IIIIII', cue_point_data)
            
            # Convert sample offset to milliseconds
            position_ms = int((sample_offset / self.sample_rate) * 1000) if self.sample_rate > 0 else 0
            
            markers.append({
                'id': cue_id,
                'position_ms': position_ms,
                'sample_offset': sample_offset,
                'label': f'Marker {cue_id}',  # Default label, will be updated if LABL chunk exists
                'description': ''
            })
            
            if self.debug:
                _debug_print(f"  Created marker: id={cue_id}, position={position_ms}ms, sample_offset={sample_offset}")
        
        # Try to find LIST chunk with LABL subchunks for labels
        # Note: There might be multiple LIST chunks, so we search for all of them
        file_handle.seek(0)
        list_chunks_found = 0
        search_start = 12  # Start after RIFF header
        
        while True:
            # Search for LIST chunk starting from search_start
            list_chunk = self._find_chunk_from_offset(file_handle, b'LIST', search_start)
            if list_chunk:
                list_chunks_found += 1
                chunk_offset, chunk_size = list_chunk
                
                if self.debug:
                    _debug_print(f"Found LIST chunk #{list_chunks_found} at offset {chunk_offset}, size {chunk_size}")
                
                # Check if this is an adtl (Associated Data List) chunk
                file_handle.seek(chunk_offset + 8)
                list_type = file_handle.read(4)
                
                if self.debug:
                    _debug_print(f"  LIST chunk type: '{list_type.decode('ascii', errors='ignore')}'")
                
                # Only parse LIST chunks of type 'adtl' (Associated Data List)
                if list_type == b'adtl':
                    markers = self._parse_list_chunk(file_handle, list_chunk, markers)
                else:
                    if self.debug:
                        _debug_print(f"  Skipping LIST chunk (not 'adtl' type)")
                
                # Continue searching after this chunk
                search_start = chunk_offset + 8 + chunk_size
                # Align to even boundary (RIFF spec)
                if search_start % 2 == 1:
                    search_start += 1
            else:
                break
        
        if self.debug:
            _debug_print(f"Found {list_chunks_found} LIST chunk(s) total")
        
        return markers
    
    def _calculate_marker_times(self, markers: List[Dict]) -> List[Dict]:
        """
        Calculate start and end times for each marker.
        
        For each marker:
        - start_ms: The marker's position_ms (when the light turns on)
        - end_ms: The next marker's position_ms (when the light turns off),
                  or the file duration if it's the last marker
        
        Args:
            markers: List of marker dictionaries with position_ms
            
        Returns:
            Updated list of markers with start_ms and end_ms fields
        """
        if not markers:
            return markers
        
        # Sort markers by position to ensure correct order
        markers = sorted(markers, key=lambda m: m.get('position_ms', 0))
        
        if self.debug:
            _debug_print(f"Calculating start/end times for {len(markers)} markers")
        
        # Calculate start and end times
        for i, marker in enumerate(markers):
            start_ms = marker.get('position_ms', 0)
            
            # End time is the start of the next marker, or file duration if last marker
            if i < len(markers) - 1:
                end_ms = markers[i + 1].get('position_ms', self.duration_ms)
            else:
                end_ms = self.duration_ms
            
            marker['start_ms'] = start_ms
            marker['end_ms'] = end_ms
            
            if self.debug:
                _debug_print(f"  Marker {i} (id={marker.get('id')}): "
                           f"start={start_ms}ms, end={end_ms}ms, duration={end_ms - start_ms}ms")
        
        return markers
    
    def _parse_list_chunk(self, file_handle, list_chunk: Tuple[int, int], markers: List[Dict]) -> List[Dict]:
        """
        Parse LIST chunk to extract LABL (labels) and NOTE (descriptions) for markers.
        
        Args:
            file_handle: Open file handle
            list_chunk: Tuple of (offset, size) for the LIST chunk
            markers: List of marker dictionaries to update
            
        Returns:
            Updated list of markers with labels and descriptions
        """
        chunk_offset, chunk_size = list_chunk
        list_data_start = chunk_offset + 8 + 4  # LIST header (8) + list type (4) = start of subchunks
        list_data_end = chunk_offset + 8 + chunk_size  # End of LIST chunk
        
        # Create a dictionary for quick lookup
        marker_dict = {m['id']: m for m in markers}
        
        if self.debug:
            _debug_print(f"  Parsing LIST chunk subchunks (data range: {list_data_start} to {list_data_end})")
        
        # Parse LABL chunks - seek back to start and find all LABL chunks
        # (Matches C++ code: fseek(fp, ListChunkInfo.dwDataOffset + 4, SEEK_SET))
        labl_count = 0
        file_handle.seek(list_data_start)
        
        while True:
            labl_chunk = self._find_chunk_within_parent(file_handle, b'labl', list_data_start, list_data_end)
            if not labl_chunk:
                break
            
            labl_offset, labl_size = labl_chunk
            labl_count += 1
            
            # Read LABL chunk data (dwIdentifier + text)
            # Note: The chunk header (8 bytes) was already read by _find_chunk_within_parent
            file_handle.seek(labl_offset + 8)
            label_data = file_handle.read(labl_size)
            
            if len(label_data) >= 4:
                cue_id = struct.unpack('<I', label_data[0:4])[0]
                # Extract text (null-terminated string)
                text = label_data[4:].rstrip(b'\x00').decode('utf-8', errors='ignore')
                
                if self.debug:
                    _debug_print(f"    LABL #{labl_count}: offset={labl_offset}, cue_id={cue_id}, label='{text}'")
                
                if cue_id in marker_dict:
                    marker_dict[cue_id]['label'] = text
                    if self.debug:
                        _debug_print(f"      Updated marker {cue_id} with label '{text}'")
                else:
                    if self.debug:
                        _debug_print(f"      WARNING: cue_id {cue_id} not found in marker dictionary")
            else:
                if self.debug:
                    _debug_print(f"    LABL #{labl_count}: data too small ({len(label_data)} bytes)")
            
            # Continue searching after this chunk
            file_handle.seek(labl_offset + 8 + labl_size)
            if labl_size % 2 == 1:  # Skip padding byte if odd-sized
                file_handle.seek(1, 1)
        
        # Parse NOTE chunks - seek back to start and find all NOTE chunks
        note_count = 0
        file_handle.seek(list_data_start)
        
        while True:
            note_chunk = self._find_chunk_within_parent(file_handle, b'note', list_data_start, list_data_end)
            if not note_chunk:
                break
            
            note_offset, note_size = note_chunk
            note_count += 1
            
            # Read NOTE chunk data
            file_handle.seek(note_offset + 8)
            note_data = file_handle.read(note_size)
            
            if len(note_data) >= 4:
                cue_id = struct.unpack('<I', note_data[0:4])[0]
                text = note_data[4:].rstrip(b'\x00').decode('utf-8', errors='ignore')
                
                if self.debug:
                    _debug_print(f"    NOTE #{note_count}: offset={note_offset}, cue_id={cue_id}, description='{text}'")
                
                if cue_id in marker_dict:
                    marker_dict[cue_id]['description'] = text
            else:
                if self.debug:
                    _debug_print(f"    NOTE #{note_count}: data too small ({len(note_data)} bytes)")
            
            # Continue searching after this chunk
            file_handle.seek(note_offset + 8 + note_size)
            if note_size % 2 == 1:  # Skip padding byte if odd-sized
                file_handle.seek(1, 1)
        
        if self.debug:
            _debug_print(f"  Parsed {labl_count} LABL chunk(s) and {note_count} NOTE chunk(s)")
        
        return list(marker_dict.values())
    
    def _find_chunk_within_parent(self, file_handle, chunk_id: bytes, parent_start: int, parent_end: int) -> Optional[Tuple[int, int]]:
        """
        Find a chunk within a parent chunk's boundaries (matches C++ FindChunk logic).
        
        Args:
            file_handle: Open file handle
            chunk_id: 4-byte chunk identifier to find
            parent_start: Start offset of parent chunk data
            parent_end: End offset of parent chunk data
            
        Returns:
            Tuple of (chunk_offset, chunk_size) or None if not found
        """
        current_pos = file_handle.tell()
        search_start = max(current_pos, parent_start)
        file_handle.seek(search_start)
        
        bytes_read_so_far = 0
        n_bytes_max = parent_end - file_handle.tell()
        n_bytes_to_read = 8  # Standard chunk header size
        
        while True:
            # Check if we have enough bytes left
            if bytes_read_so_far + n_bytes_to_read > n_bytes_max:
                return None
            
            chunk_header = file_handle.read(n_bytes_to_read)
            
            if len(chunk_header) < n_bytes_to_read:
                return None
            
            # Check if first byte is a padding byte (0x00) - matches C++ logic
            # C++ checks: (pChunk->ckid & 0xff) == 0
            current_id_test = struct.unpack('<4s', chunk_header[:4])[0]
            if (current_id_test[0] & 0xFF) == 0:
                # Padding byte detected - skip it and re-read
                file_handle.seek(1 - n_bytes_to_read, 1)
                bytes_read_so_far += 1
                
                if bytes_read_so_far + n_bytes_to_read > n_bytes_max:
                    return None
                
                chunk_header = file_handle.read(n_bytes_to_read)
                if len(chunk_header) < n_bytes_to_read:
                    return None
            
            bytes_read_so_far += n_bytes_to_read
            
            current_id, chunk_size = struct.unpack('<4sI', chunk_header)
            chunk_pos = file_handle.tell() - n_bytes_to_read
            
            if current_id == chunk_id:
                return (chunk_pos, chunk_size)
            
            # Skip to next chunk
            if chunk_size <= 0:
                return None
            
            # Calculate bytes to skip: chunk_size + (already read header) - n_bytes_to_read
            bytes_to_skip = chunk_size + 8 - n_bytes_to_read
            
            if bytes_read_so_far + bytes_to_skip > n_bytes_max:
                return None
            
            file_handle.seek(bytes_to_skip, 1)
            bytes_read_so_far += bytes_to_skip
            
            if file_handle.tell() >= parent_end:
                return None
    
    def _extract_lights_from_markers(self, markers: List[Dict]) -> List[Dict]:
        """
        Extract unique lights from marker labels.
        
        Light tag conventions:
        - Format: "LightName PinNumber" (e.g., "SomeLight A2")
        - Special tags: "ALL_ON" and "ALL_OFF" (no pin number)
        - Light names contain no spaces or special characters
        - Pin number follows the light name after a space
        
        Args:
            markers: List of marker dictionaries with label information
            
        Returns:
            List of unique light dictionaries with name and pin
        """
        lights = []
        seen_lights = set()  # Track unique light names
        
        if self.debug:
            _debug_print(f"Extracting lights from {len(markers)} markers")
        
        for i, marker in enumerate(markers):
            label = marker.get('label', '').strip()
            
            if self.debug:
                _debug_print(f"  Processing marker {i}: id={marker.get('id')}, label='{label}'")
            
            if not label:
                if self.debug:
                    _debug_print(f"    Skipping marker {i} (empty label)")
                continue
            
            # Check for special tags
            if label == 'ALL_ON':
                # Special tag - no pin, but we might want to track it
                if 'ALL_ON' not in seen_lights:
                    lights.append({
                        'name': 'ALL_ON',
                        'pin': None,
                        'is_special': True
                    })
                    seen_lights.add('ALL_ON')
                continue
            elif label == 'ALL_OFF':
                # Special tag - no pin
                if 'ALL_OFF' not in seen_lights:
                    lights.append({
                        'name': 'ALL_OFF',
                        'pin': None,
                        'is_special': True
                    })
                    seen_lights.add('ALL_OFF')
                continue
            
            # Parse regular light tag: "LightName PinNumber"
            # Split on space - last part should be pin number
            parts = label.rsplit(' ', 1)
            
            if len(parts) == 2:
                light_name = parts[0].strip()
                pin_number = parts[1].strip()
                
                # Validate format
                if not light_name:
                    print(f"WARNING: Marker tag '{label}' has empty light name (missing name before pin number)")
                    continue
                
                if not pin_number:
                    print(f"WARNING: Marker tag '{label}' has empty pin number (missing pin after space)")
                    continue
                
                # Validate light name (no spaces or special characters)
                # Light name should only contain alphanumeric and underscore
                if not re.match(r'^[a-zA-Z0-9_]+$', light_name):
                    print(f"WARNING: Marker tag '{label}' has invalid light name '{light_name}'. "
                          f"Light names must contain only alphanumeric characters and underscores (no spaces or special characters)")
                    continue
                
                # Only add if we haven't seen this light name before
                if light_name not in seen_lights:
                    lights.append({
                        'name': light_name,
                        'pin': pin_number,
                        'is_special': False
                    })
                    seen_lights.add(light_name)
                    if self.debug:
                        _debug_print(f"    Added new light: '{light_name}' with pin '{pin_number}'")
                else:
                    if self.debug:
                        _debug_print(f"    Skipping duplicate light name: '{light_name}'")
            else:
                # No space found - not a special tag and not in expected format
                print(f"WARNING: Marker tag '{label}' does not conform to expected conventions. "
                      f"Expected format: 'LightName PinNumber' (e.g., 'SomeLight A2') or special tags 'ALL_ON'/'ALL_OFF'")
                if self.debug:
                    _debug_print(f"    Label does not match expected format (no space found)")
        
        if self.debug:
            _debug_print(f"Final result: {len(lights)} unique lights extracted")
        
        return lights


def parse_wav_file(file_path: Path, debug: bool = False) -> Dict:
    """
    Convenience function to parse a WAV file.
    
    Args:
        file_path: Path to the WAV file
        debug: Enable debug output
        
    Returns:
        Dictionary with file information, markers, and lights
    """
    parser = WAVParser(debug=debug)
    return parser.parse_wav_file(file_path)

