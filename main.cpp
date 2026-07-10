#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

const size_t KEY_SIZE = 64;
const size_t BUCKET_COUNT = 100003; // Prime number for hash table
const string INDEX_FILE = "index.bin";
const string DATA_FILE = "data.bin";

struct Entry {
    char key[KEY_SIZE];
    int value;
    long long next_offset;
};

class FileStorage {
    fstream index_fs;
    fstream data_fs;

    size_t hash_key(const string& key) {
        size_t h = 0;
        for (char c : key) h = h * 31 + c;
        return h % BUCKET_COUNT;
    }

public:
    FileStorage() {
        index_fs.open(INDEX_FILE, ios::in | ios::out | ios::binary);
        if (!index_fs) {
            index_fs.open(INDEX_FILE, ios::out | ios::binary);
            vector<long long> empty_buckets(BUCKET_COUNT, -1);
            index_fs.write(reinterpret_cast<char*>(empty_buckets.data()), BUCKET_COUNT * sizeof(long long));
            index_fs.close();
            index_fs.open(INDEX_FILE, ios::in | ios::out | ios::binary);
        }

        data_fs.open(DATA_FILE, ios::in | ios::out | ios::binary);
        if (!data_fs) {
            data_fs.open(DATA_FILE, ios::out | ios::binary);
            data_fs.close();
            data_fs.open(DATA_FILE, ios::in | ios::out | ios::binary);
        }
    }

    ~FileStorage() {
        if (index_fs.is_open()) index_fs.close();
        if (data_fs.is_open()) data_fs.close();
    }

    void insert(const string& key, int value) {
        size_t bucket = hash_key(key);
        long long head_offset;
        
        index_fs.seekg(bucket * sizeof(long long));
        index_fs.read(reinterpret_cast<char*>(&head_offset), sizeof(long long));

        long long current = head_offset;
        long long prev = -1;

        while (current != -1) {
            data_fs.seekg(current);
            Entry e;
            data_fs.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (strcmp(e.key, key.c_str()) == 0 && e.value == value) {
                return; // Already exists
            }
            prev = current;
            current = e.next_offset;
        }

        // Append new entry
        data_fs.seekp(0, ios::end);
        long long new_offset = data_fs.tellp();
        Entry new_entry;
        memset(new_entry.key, 0, KEY_SIZE);
        strncpy(new_entry.key, key.c_str(), KEY_SIZE - 1);
        new_entry.value = value;
        new_entry.next_offset = head_offset;

        data_fs.write(reinterpret_cast<char*>(&new_entry), sizeof(Entry));

        // Update index
        index_fs.seekp(bucket * sizeof(long long));
        index_fs.write(reinterpret_cast<char*>(&new_offset), sizeof(long long));
    }

    void remove(const string& key, int value) {
        size_t bucket = hash_key(key);
        long long head_offset;
        
        index_fs.seekg(bucket * sizeof(long long));
        index_fs.read(reinterpret_cast<char*>(&head_offset), sizeof(long long));

        long long current = head_offset;
        long long prev = -1;

        while (current != -1) {
            data_fs.seekg(current);
            Entry e;
            data_fs.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (strcmp(e.key, key.c_str()) == 0 && e.value == value) {
                if (prev == -1) {
                    head_offset = e.next_offset;
                    index_fs.seekp(bucket * sizeof(long long));
                    index_fs.write(reinterpret_cast<char*>(&head_offset), sizeof(long long));
                } else {
                    data_fs.seekp(prev);
                    Entry prev_e;
                    data_fs.read(reinterpret_cast<char*>(&prev_e), sizeof(Entry));
                    prev_e.next_offset = e.next_offset;
                    data_fs.seekp(prev);
                    data_fs.write(reinterpret_cast<char*>(&prev_e), sizeof(Entry));
                }
                return;
            }
            prev = current;
            current = e.next_offset;
        }
    }

    void find(const string& key) {
        size_t bucket = hash_key(key);
        long long current;
        
        index_fs.seekg(bucket * sizeof(long long));
        index_fs.read(reinterpret_cast<char*>(&current), sizeof(long long));

        vector<int> results;
        while (current != -1) {
            data_fs.seekg(current);
            Entry e;
            data_fs.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (strcmp(e.key, key.c_str()) == 0) {
                results.push_back(e.value);
            }
            current = e.next_offset;
        }

        if (results.empty()) {
            cout << "null" << endl;
        } else {
            sort(results.begin(), results.end());
            for (size_t i = 0; i < results.size(); ++i) {
                cout << results[i] << (i == results.size() - 1 ? "" : " ");
            }
            cout << endl;
        }
    }
};

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n;
    if (!(cin >> n)) return 0;

    FileStorage storage;
    string cmd, key;
    int val;

    for (int i = 0; i < n; ++i) {
        cin >> cmd;
        if (cmd == "insert") {
            cin >> key >> val;
            storage.insert(key, val);
        } else if (cmd == "delete") {
            cin >> key >> val;
            storage.remove(key, val);
        } else if (cmd == "find") {
            cin >> key;
            storage.find(key);
        }
    }

    return 0;
}
