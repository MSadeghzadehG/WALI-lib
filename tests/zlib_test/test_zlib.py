#!/usr/bin/env python3
"""
Simple zlib test script for WALI/WASM
Tests basic compression and decompression functionality
"""

import zlib

def test_compress_decompress():
    """Test basic compress/decompress"""
    original = b"Hello, WALI! This is a test of zlib compression in WebAssembly. " * 10
    print(f"Original size: {len(original)} bytes")
    
    # Compress
    compressed = zlib.compress(original)
    print(f"Compressed size: {len(compressed)} bytes")
    print(f"Compression ratio: {len(compressed)/len(original):.2%}")
    
    # Decompress
    decompressed = zlib.decompress(compressed)
    print(f"Decompressed size: {len(decompressed)} bytes")
    
    # Verify
    if original == decompressed:
        print("SUCCESS: Original and decompressed data match!")
        return True
    else:
        print("FAILURE: Data mismatch!")
        return False

def test_crc32():
    """Test CRC32 checksum"""
    data = b"Hello, WALI!"
    crc = zlib.crc32(data)
    print(f"CRC32 of '{data.decode()}': {crc} (0x{crc:08x})")
    return True

def test_adler32():
    """Test Adler32 checksum"""
    data = b"Hello, WALI!"
    adler = zlib.adler32(data)
    print(f"Adler32 of '{data.decode()}': {adler} (0x{adler:08x})")
    return True

def test_compress_levels():
    """Test different compression levels"""
    data = b"Test data for compression levels " * 50
    print(f"\nTesting compression levels on {len(data)} bytes:")
    
    for level in [1, 6, 9]:
        compressed = zlib.compress(data, level)
        print(f"  Level {level}: {len(compressed)} bytes ({len(compressed)/len(data):.1%})")
    return True

if __name__ == "__main__":
    print("=" * 50)
    print("WALI zlib Test Suite")
    print("=" * 50)
    
    tests = [
        ("Compress/Decompress", test_compress_decompress),
        ("CRC32", test_crc32),
        ("Adler32", test_adler32),
        ("Compression Levels", test_compress_levels),
    ]
    
    passed = 0
    for name, test_func in tests:
        print(f"\n--- {name} ---")
        try:
            if test_func():
                passed += 1
        except Exception as e:
            print(f"ERROR: {e}")
    
    print("\n" + "=" * 50)
    print(f"Results: {passed}/{len(tests)} tests passed")
    print("=" * 50)
