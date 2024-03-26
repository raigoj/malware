#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <openssl/aes.h>
#include <sys/stat.h>

void decrypt_file(const char* filename, const char* key);
void decrypt_folder(const char* folder_path, const char* key);

void decrypt_file(const char* filename, const char* key) {
    printf("Decrypting file: %s\n", filename);
    if (strcmp(filename, "./dec.exe") == 0 || strcmp(filename, "./enc.exe") == 0 || strcmp(filename, "./gen.exe") == 0) {
        printf("Skipping decryption for file: %s\n", filename);
        return;
    }
    const int chunk_size = 64 * 1024;  // 64 KB
    const char* output_extension = ".decrypted";
    char output_filename[FILENAME_MAX];
    strncpy(output_filename, filename, FILENAME_MAX - strlen(output_extension) - 1);
    strncat(output_filename, output_extension, FILENAME_MAX - strlen(output_extension) - 1);
    unsigned char iv[AES_BLOCK_SIZE];
    memset(iv, ' ', AES_BLOCK_SIZE);
    AES_KEY aes_key;
    AES_set_decrypt_key((const unsigned char*)key, 128, &aes_key);
    FILE* file_in = fopen(filename, "rb");
    FILE* file_out = fopen(output_filename, "wb");
    fseek(file_in, AES_BLOCK_SIZE, SEEK_SET);  // Skip the IV
    while (1) {
        unsigned char chunk[chunk_size];
        size_t bytesRead = fread(chunk, sizeof(unsigned char), chunk_size, file_in);
        if (bytesRead == 0) {
            break;
        }
        AES_cbc_encrypt(chunk, chunk, bytesRead, &aes_key, iv, AES_DECRYPT);
        // Remove padding
        if (bytesRead % AES_BLOCK_SIZE != 0) {
            size_t padding = AES_BLOCK_SIZE - (bytesRead % AES_BLOCK_SIZE);
            bytesRead -= padding;
        }
        fwrite(chunk, sizeof(unsigned char), bytesRead, file_out);
    }
    fclose(file_in);
    fclose(file_out);
    remove(filename);
    rename(output_filename, filename);
} 

void decrypt_folder(const char* folder_path, const char* key) {
    DIR* dir;
    struct dirent* entry;
    if ((dir = opendir(folder_path)) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char file_path[PATH_MAX];
                snprintf(file_path, PATH_MAX, "%s/%s", folder_path, entry->d_name);
                if (strcmp(file_path, __FILE__) != 0) {
                    struct stat file_stat;
                    if (stat(file_path, &file_stat) == 0) {
                        if (S_ISDIR(file_stat.st_mode)) {
                            decrypt_folder(file_path, key);
                        } else {
                            decrypt_file(file_path, key);  // Decrypt individual file
                        }
                    }
                }
            }
        }
        closedir(dir);
    }
}

char* read_encryption_key() {
    const char* desktop_path = getenv("USERPROFILE");
    const char* key_file_path = "/Desktop/encryption_key.txt";
    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s%s", desktop_path, key_file_path);
    FILE* key_file = fopen(full_path, "r");
    if (key_file != NULL) {
        printf("Reading encryption key from %s\n", full_path);
        fseek(key_file, 0, SEEK_END);
        long size = ftell(key_file);
        fseek(key_file, 0, SEEK_SET);
        char* file_contents = malloc(size + 1);
        if (file_contents != NULL) {
            fread(file_contents, sizeof(char), size, key_file);
            file_contents[size] = '\0';
            const char* key_start = strstr(file_contents, "key: ");
            if (key_start != NULL) {
                key_start += strlen("key: ");
                size_t key_length = strlen(key_start);
                char* key = malloc(key_length + 1);
                if (key != NULL) {
                    strncpy(key, key_start, key_length);
                    key[key_length] = '\0';
                    free(file_contents);
                    fclose(key_file);
                    return key;
                }
            }
            free(file_contents);
        }
        fclose(key_file);
    }
    return NULL;
}

int main() { 
    const char* encryption_key = read_encryption_key(); 
    printf("Encryption key: %s\n", encryption_key);
    const char* folder_path = "."; 
    decrypt_folder(folder_path, encryption_key); 
    free((void*)encryption_key); 
    return 0; 
    }