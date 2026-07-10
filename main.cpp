#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

const size_t KEY_SIZE = 64;
// Increased bucket count to further reduce collisions
const size_t BUCKET_COUNT = 400009; 
const string INDEX_FILE = "index.bin";
const string DATA_FILE = "data.bin";

struct Entry {
    char key[KEY_SIZE];
    int value;
    long long next_offset;
};

class FileStorage {
    fstream data_fs;
    vector<long long> index_cache;

    size_t hash_key(const string& key) {
        size_t h = 5381;
        for (char c : key) {
            h = ((h << 5) + h) + c;
        }
        return h % BUCKET_COUNT;
    }

public:
    FileStorage() {
        index_cache.assign(BUCKET_COUNT, -1);
        
        ifstream index_in(INDEX_FILE, ios::in | ios::binary);
        if (index_in) {
            index_in.seekg(0, ios::end);
            long long size = index_in.tellg();
            index_in.seekg(0, ios::beg);
            // Only load if the file matches the current BUCKET_COUNT
            if (size == (long long)BUCKET_COUNT * sizeof(long long)) {
                index_in.read(reinterpret_cast<char*>(index_cache.data()), BUCKET_COUNT * sizeof(long long));
            }
            index_in.close();
        }

        data_fs.open(DATA_FILE, ios::in | ios::out | ios::binary);
        if (!data_fs) {
            ofstream create_data(DATA_FILE, ios::out | ios::binary);
            create_data.close();
            data_fs.open(DATA_FILE, ios::in | ios::out | ios::binary);
        }
    }

    ~FileStorage() {
        ofstream index_out(INDEX_FILE, ios::out | ios::binary);
        if (index_out) {
            index_out.write(reinterpret_cast<char*>(index_cache.data()), BUCKET_COUNT * sizeof(long long));
            index_out.close();
        }
        if (data_fs.is_open()) data_fs.close();
    }

    void insert(const string& key, int value) {
        size_t bucket = hash_key(key);
        long long head_offset = index_cache[bucket];
        long long current = head_offset;

        while (current != -1) {
            data_fs.seekg(current);
            Entry e;
            data_fs.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (e.value == value && strcmp(e.key, key.c_str()) == 0) {
                return; 
            }
            current = e.next_offset;
        }

        data_fs.seekp(0, ios::end);
        long long new_offset = data_fs.tellp();
        Entry new_entry;
        memset(new_entry.key, 0, KEY_SIZE);
        strncpy(new_entry.key, key.c_str(), KEY_SIZE - 1);
        new_entry.value = value;
        new_entry.next_offset = head_offset;

        data_fs.write(reinterpret_cast<char*>(&new_entry), sizeof(Entry));
        index_cache[bucket] = new_offset;
    }

    void remove(const string& key, int value) {
        size_t bucket = hash_key(key);
        long long head_offset = index_cache[bucket];

        long long current = head_offset;
        long long prev = -1;

        while (current != -1) {
            data_fs.seekg(current);
            Entry e;
            data_fs.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (e.value == value && strcmp(e.key, key.c_str()) == 0) {
                if (prev == -1) {
                    index_cache[bucket] = e.next_offset;
                } else {
                    data_fs.seekp(prev + offsetof(Entry, next_offset));
                    data_fs.write(reinterpret_cast<char*>(&e.next_offset), sizeof(long long));
                }
                return;
            }
            prev = current;
            current = e.next_offset;
        }
    }

    void find(const string& key) {
        size_t bucket = hash_key(key);
        long long current = index_cache[bucket];

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
            cout << "null\n";
        } else {
            sort(results.begin(), results.end());
            for (size_t i = 0; i < results.size(); ++i) {
                cout << results[i] << (i == results.size() - 1 ? "" : " ");
            }
            cout << "\n";
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
        if (!(cin >> cmd)) break;
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
