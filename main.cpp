#include <algorithm>
#include <iostream>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <memory>
#include <chrono>
#include <stack>

struct Node {
    char character;
    long frequency;
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;

    Node(char c, int f) : character(c), frequency(f), left(nullptr), right(nullptr) {}
};

using std::string, std::vector, std::ifstream, std::ofstream, std::cout, std::endl, std::unordered_map, std::shared_ptr;

struct BitCode {
    uint32_t bits = 0;
    int length = 0;
};

unordered_map<char, BitCode> codeMap;

void generateCodes(const shared_ptr<Node>& root) {
    if (!root) return;

    std::stack<std::pair<shared_ptr<Node>, BitCode>> st;
    st.push({ root, {} });

    while (!st.empty()) {
        auto [node, code] = st.top();
        st.pop();

        if (!node->left && !node->right) {
            codeMap[node->character] = code;
        } else {
            if (node->right) {
                st.push({ node->right, { (code.bits << 1) | 1, code.length + 1 } });
            }
            if (node->left) {
                st.push({ node->left, { (code.bits << 1), code.length + 1 } });
            }
        }
    }
}

void compressFile(const string& inputFilePath, const string& outputFilePath) {
    std::ifstream file(inputFilePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to read file!\n";
        return;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0);
    std::vector<char> data(fileSize);
    file.read(data.data(), fileSize);
    file.close();

    vector<long> frequencies(256, 0);
    for (char c : data) {
        frequencies[static_cast<unsigned char>(c)]++;
    }

    auto cmp = [](const shared_ptr<Node>& a, const shared_ptr<Node>& b) {
        return a->frequency > b->frequency;
    };
    std::priority_queue<shared_ptr<Node>, vector<shared_ptr<Node>>, decltype(cmp)> pq(cmp);

    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            pq.push(std::make_shared<Node>(static_cast<char>(i), frequencies[i]));
        }
    }

    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        auto merged = std::make_shared<Node>('\0', left->frequency + right->frequency);
        merged->left = left;
        merged->right = right;
        pq.push(merged);
    }

    shared_ptr<Node> root = pq.top();
    generateCodes(root);

    std::ofstream fout(outputFilePath, std::ios::binary);
    if (!fout) {
        std::cerr << "Failed to open output file\n";
        return;
    }

    int uniqueCount = std::count_if(frequencies.begin(), frequencies.end(), [](long f) { return f > 0; });
    fout.write(reinterpret_cast<const char*>(&uniqueCount), sizeof(uniqueCount));

    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            unsigned char symbol = static_cast<unsigned char>(i);
            int freq = static_cast<int>(frequencies[i]);
            fout.write(reinterpret_cast<const char*>(&symbol), sizeof(symbol));
            fout.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
        }
    }

    unsigned char buffer = 0;
    int bitCount = 0;

    for (char c : data) {
        BitCode code = codeMap[c];
        for (int i = code.length - 1; i >= 0; --i) {
            buffer <<= 1;
            if ((code.bits >> i) & 1) buffer |= 1;
            bitCount++;
            if (bitCount == 8) {
                fout.write(reinterpret_cast<const char*>(&buffer), 1);
                bitCount = 0;
                buffer = 0;
            }
        }
    }

    if (bitCount > 0) {
        buffer <<= (8 - bitCount);
        fout.write(reinterpret_cast<const char*>(&buffer), 1);
    }

    fout.close();
}

void decompressFile(const string& inputFilePath, const string& outputFilePath) {
    std::ifstream file(inputFilePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to read file\n";
        return;
    }

    int uniqueCount;
    file.read(reinterpret_cast<char*>(&uniqueCount), sizeof(uniqueCount));
    vector<int> frequencies(256, 0);

    for (int i = 0; i < uniqueCount; ++i) {
        unsigned char symbol;
        int freq;
        file.read(reinterpret_cast<char*>(&symbol), sizeof(symbol));
        file.read(reinterpret_cast<char*>(&freq), sizeof(freq));
        frequencies[symbol] = freq;
    }

    auto cmp = [](const shared_ptr<Node>& a, const shared_ptr<Node>& b) {
        return a->frequency > b->frequency;
    };
    std::priority_queue<shared_ptr<Node>, vector<shared_ptr<Node>>, decltype(cmp)> pq(cmp);

    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            pq.push(std::make_shared<Node>(static_cast<char>(i), frequencies[i]));
        }
    }

    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        auto merged = std::make_shared<Node>('\0', left->frequency + right->frequency);
        merged->left = left;
        merged->right = right;
        pq.push(merged);
    }

    shared_ptr<Node> root = pq.top();
    shared_ptr<Node> current = root;

    std::ofstream fout(outputFilePath, std::ios::binary);
    long totalChars = 0;
    for (int f : frequencies) totalChars += f;

    long decodedCount = 0;
    unsigned char byte;

    while (decodedCount < totalChars && file.read(reinterpret_cast<char*>(&byte), 1)) {
        for (int i = 7; i >= 0 && decodedCount < totalChars; --i) {
            bool bit = (byte >> i) & 1;
            current = bit ? current->right : current->left;
            if (!current->left && !current->right) {
                fout.put(current->character);
                current = root;
                decodedCount++;
            }
        }
    }

    fout.close();
    file.close();
}

int main() {
    string inputFilePath = "C:\\Users\\40124291\\Downloads\\archive(1)\\IMDB Dataset.csv";
    string outputFilePath = "C:\\Users\\40124291\\CLionProjects\\HuffmanCoding\\output.bin";
    string output2 = "C:\\Users\\40124291\\CLionProjects\\HuffmanCoding\\testoutput.csv";

    auto startCompress = std::chrono::high_resolution_clock::now();
    compressFile(inputFilePath, outputFilePath);
    auto endCompress = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> compressionTime = endCompress - startCompress;
    std::cout << "Compression Time: " << compressionTime.count() << " seconds\n";

    auto startDecompress = std::chrono::high_resolution_clock::now();
    decompressFile(outputFilePath, output2);
    auto endDecompress = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> decompressionTime = endDecompress - startDecompress;
    std::cout << "Decompression Time: " << decompressionTime.count() << " seconds\n";

    return 0;
}