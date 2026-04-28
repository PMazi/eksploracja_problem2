#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <chrono>

using namespace std;

void calculateTime(
    std::chrono::high_resolution_clock::time_point start,
    std::chrono::high_resolution_clock::time_point end,
    std::string message = "Czas"
) {
    std::chrono::duration<double> duration = end - start;
    std::cerr << message << ": " << duration.count() << " s" << std::endl;
}

struct FPNode {
    string item;
    int count;
    FPNode* parent;
    unordered_map<string, FPNode*> children;
    FPNode* next_link;

    FPNode(const string& item, int count, FPNode* parent)
        : item(item), count(count), parent(parent), next_link(nullptr) {}
};

struct HeaderEntry {
    int count;
    FPNode* first;
};

struct Rule {
    vector<string> A;
    vector<string> B;
    double supp;
    double conf;
};

using HeaderTable = unordered_map<string, HeaderEntry>;
using Itemset = set<string>;
using FrequentItemsets = map<Itemset, int>;

vector<string> split(const string& line, char delimiter) {
    vector<string> parts;
    string part;
    stringstream ss(line);

    while (getline(ss, part, delimiter)) {
        parts.push_back(part);
    }

    return parts;
}

bool is_number(const string& s) {
    if (s.empty()) return false;

    for (char c : s) {
        if (!isdigit(c)) return false;
    }

    return true;
}

void insert_tree(const vector<string>& items, int index, FPNode* node, HeaderTable& header_table) {
    if (index >= static_cast<int>(items.size())) {
        return;
    }

    const string& current_item = items[index];

    if (node->children.find(current_item) != node->children.end()) {
        node->children[current_item]->count += 1;
    } else {
        FPNode* new_node = new FPNode(current_item, 1, node);
        node->children[current_item] = new_node;

        if (header_table[current_item].first == nullptr) {
            header_table[current_item].first = new_node;
        } else {
            FPNode* current = header_table[current_item].first;
            while (current->next_link != nullptr) {
                current = current->next_link;
            }
            current->next_link = new_node;
        }
    }

    insert_tree(items, index + 1, node->children[current_item], header_table);
}

void mine_tree(
    HeaderTable& header_table,
    double min_supp_count,
    Itemset prefix,
    FrequentItemsets& frequent_itemsets
) {
    vector<pair<string, HeaderEntry>> sorted_items;

    for (const auto& entry : header_table) {
        sorted_items.push_back(entry);
    }

    sort(sorted_items.begin(), sorted_items.end(),
         [](const auto& a, const auto& b) {
             return a.second.count < b.second.count;
         });

    for (const auto& entry : sorted_items) {
        string item = entry.first;

        Itemset new_frequent_set = prefix;
        new_frequent_set.insert(item);

        frequent_itemsets[new_frequent_set] = header_table[item].count;

        vector<pair<vector<string>, int>> cond_patterns;

        FPNode* node = header_table[item].first;

        while (node != nullptr) {
            vector<string> prefix_path;

            FPNode* parent = node->parent;
            while (parent != nullptr && !parent->item.empty()) {
                prefix_path.push_back(parent->item);
                parent = parent->parent;
            }

            if (!prefix_path.empty()) {
                cond_patterns.push_back({prefix_path, node->count});
            }

            node = node->next_link;
        }

        unordered_map<string, int> cond_header_counts;

        for (const auto& pattern : cond_patterns) {
            const vector<string>& path = pattern.first;
            int count = pattern.second;

            for (const string& p_item : path) {
                cond_header_counts[p_item] += count;
            }
        }

        HeaderTable cond_header;

        for (const auto& ch : cond_header_counts) {
            if (ch.second >= min_supp_count) {
                cond_header[ch.first] = {ch.second, nullptr};
            }
        }

        if (!cond_header.empty()) {
            FPNode* cond_root = new FPNode("", 0, nullptr);

            for (const auto& pattern : cond_patterns) {
                const vector<string>& path = pattern.first;
                int count = pattern.second;

                vector<string> filtered_path;

                for (const string& p_item : path) {
                    if (cond_header.find(p_item) != cond_header.end()) {
                        filtered_path.push_back(p_item);
                    }
                }

                if (!filtered_path.empty()) {
                    FPNode* curr = cond_root;

                    for (auto it = filtered_path.rbegin(); it != filtered_path.rend(); ++it) {
                        const string& p_item = *it;

                        if (curr->children.find(p_item) != curr->children.end()) {
                            curr->children[p_item]->count += count;
                        } else {
                            FPNode* new_node = new FPNode(p_item, count, curr);
                            curr->children[p_item] = new_node;

                            if (cond_header[p_item].first == nullptr) {
                                cond_header[p_item].first = new_node;
                            } else {
                                FPNode* tmp = cond_header[p_item].first;
                                while (tmp->next_link != nullptr) {
                                    tmp = tmp->next_link;
                                }
                                tmp->next_link = new_node;
                            }
                        }

                        curr = curr->children[p_item];
                    }
                }
            }

            mine_tree(cond_header, min_supp_count, new_frequent_set, frequent_itemsets);
        }
    }
}

void generate_combinations_recursive(
    const vector<string>& items,
    int start,
    int target_size,
    vector<string>& current,
    vector<vector<string>>& result
) {
    if (static_cast<int>(current.size()) == target_size) {
        result.push_back(current);
        return;
    }

    for (int i = start; i < static_cast<int>(items.size()); i++) {
        current.push_back(items[i]);
        generate_combinations_recursive(items, i + 1, target_size, current, result);
        current.pop_back();
    }
}

vector<vector<string>> combinations(const vector<string>& items, int size) {
    vector<vector<string>> result;
    vector<string> current;

    generate_combinations_recursive(items, 0, size, current, result);

    return result;
}

vector<Rule> solve(
    const string& datapath,
    double min_support,
    double min_confidence,
    bool verbose = false
) {
    vector<vector<string>> raw_transactions;

    try {
        ifstream file(datapath);

        if (!file.is_open()) {
            throw runtime_error("Nie mozna otworzyc pliku.");
        }

        string line;
        getline(file, line);

        unordered_map<string, vector<string>> trans_dict;
        // Pomiar czasu wczytywania danych
        {
        auto start = std::chrono::high_resolution_clock::now();
        while (getline(file, line)) {
            // wersja wolna
            // vector<string> parts = split(line, ',');

            // if (parts.size() >= 2 && is_number(parts[0])) {
            //     trans_dict[parts[0]].push_back(parts[1]);
            // }

            
            // wersja szybsza z walidacją
            size_t comma1 = line.find(',');
            if (comma1 == string::npos) continue;

            size_t comma2 = line.find(',', comma1 + 1);

            string col1 = line.substr(0, comma1);

            string col2;
            if (comma2 == string::npos) {
                col2 = line.substr(comma1 + 1);
            } else {
                col2 = line.substr(comma1 + 1, comma2 - comma1 - 1);
            }

            if (is_number(col1)) {
                trans_dict[col1].push_back(col2);
            }


            // wersja szybsza bez walidacji
            // size_t comma1 = line.find(',');
            // size_t comma2 = line.find(',', comma1 + 1);
            // string col1 = line.substr(0, comma1);
            // string col2 = line.substr(comma1 + 1, comma2 - comma1 - 1);
            // trans_dict[col1].push_back(col2);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        calculateTime(start, end, "Czas wczytywania danych");
        }   

        for (const auto& entry : trans_dict) {
            raw_transactions.push_back(entry.second);
        }

    } catch (...) {
        raw_transactions.clear();
    }

    int n_trans = static_cast<int>(raw_transactions.size());
    double min_supp_count = min_support * n_trans;

    
    // Pomiar czasu liczenia wsparcia pojedynczych elementów
    unordered_map<string, int> item_counts;
    {
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto& trans : raw_transactions) {
        for (const string& item : trans) {
            item_counts[item] += 1;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    calculateTime(start, end, "Czas liczenia wsparcia pojedynczych elementów");
    }
    
    // Pomiar czasu filtrowania elementów niespełniających wsparcia
    unordered_map<string, int> frequent_items;
    {
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto& entry : item_counts) {
        if (entry.second >= min_supp_count) {
            frequent_items[entry.first] = entry.second;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    calculateTime(start, end, "Czas filtrowania elementów niespełniających wsparcia");
    }

    vector<string> sorted_items;

    for (const auto& entry : frequent_items) {
        sorted_items.push_back(entry.first);
    }

    // Pomiar czasu sortowania elementów według wsparcia
    {
    auto start = std::chrono::high_resolution_clock::now();
    sort(sorted_items.begin(), sorted_items.end(),
         [&frequent_items](const string& a, const string& b) {
             return frequent_items[a] > frequent_items[b];
         });
    auto end = std::chrono::high_resolution_clock::now();
    calculateTime(start, end, "Czas sortowania elementów według wsparcia");
    }

    FPNode* root = new FPNode("", 0, nullptr);

    HeaderTable header_table;

    for (const auto& entry : frequent_items) {
        header_table[entry.first] = {entry.second, nullptr};
    }

    // Pomiar czasu wstawiania transakcji do drzewa FP
    {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::unordered_map<std::string, int> item_order;
    item_order.reserve(sorted_items.size());

    for (int i = 0; i < static_cast<int>(sorted_items.size()); ++i) {
        item_order[sorted_items[i]] = i;
    }

    for (const auto& trans : raw_transactions) {
        vector<string> filtered_trans;
        filtered_trans.reserve(trans.size());

        std::unordered_set<std::string> seen;
        seen.reserve(trans.size());

        for (const auto& item : trans) {
            if (item_order.find(item) != item_order.end()) {
                if (seen.insert(item).second) {
                    filtered_trans.push_back(item);
                }
            }
        }

        sort(filtered_trans.begin(), filtered_trans.end(),
            [&item_order](const string& a, const string& b) {
                return item_order.at(a) < item_order.at(b);
            });

        if (!filtered_trans.empty()) {
            insert_tree(filtered_trans, 0, root, header_table);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    calculateTime(start, end, "Czas wstawiania transakcji do drzewa FP");
    }

    FrequentItemsets frequent_itemsets;
    Itemset empty_prefix;
    // Pomiar czasu wydobywania częstych itemsetów z drzewa FP
    {
    auto start = std::chrono::high_resolution_clock::now();
    mine_tree(header_table, min_supp_count, empty_prefix, frequent_itemsets);
    auto end = std::chrono::high_resolution_clock::now();
    calculateTime(start, end, "Czas wydobywania częstych itemsetów z drzewa FP");
    }

    vector<Rule> rules;

    // Pomiar czasu generowania reguł asocjacyjnych z częstych itemsetów
    {
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto& entry : frequent_itemsets) {
        const Itemset& itemset = entry.first;
        int count = entry.second;

        if (itemset.size() > 1) {
            double support = static_cast<double>(count) / n_trans;

            vector<string> itemset_vec(itemset.begin(), itemset.end());

            for (int i = 1; i < static_cast<int>(itemset_vec.size()); i++) {
                vector<vector<string>> antecedents = combinations(itemset_vec, i);

                for (const auto& antecedent_vec : antecedents) {
                    Itemset antecedent(antecedent_vec.begin(), antecedent_vec.end());

                    Itemset consequent;

                    for (const string& item : itemset) {
                        if (antecedent.find(item) == antecedent.end()) {
                            consequent.insert(item);
                        }
                    }

                    auto found = frequent_itemsets.find(antecedent);

                    if (found != frequent_itemsets.end()) {
                        int supp_a = found->second;
                        double confidence = static_cast<double>(count) / supp_a;

                        if (confidence >= min_confidence) {
                            Rule rule;

                            rule.A = vector<string>(antecedent.begin(), antecedent.end());
                            rule.B = vector<string>(consequent.begin(), consequent.end());
                            rule.supp = support;
                            rule.conf = confidence;

                            rules.push_back(rule);
                        }
                    }
                }
            }
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    calculateTime(start, end, "Czas generowania reguł asocjacyjnych z częstych itemsetów");
    }

    if (verbose) {
        cout << "Znaleziono " << rules.size() << " regul." << endl;
    }

    if (verbose) {
        cout << "Wygenerowano " << rules.size() << " regul." << endl;

        for (const Rule& rule : rules) {
            cout << "[";

            for (size_t i = 0; i < rule.A.size(); i++) {
                cout << rule.A[i];
                if (i + 1 < rule.A.size()) cout << ", ";
            }

            cout << "]=>[";

            for (size_t i = 0; i < rule.B.size(); i++) {
                cout << rule.B[i];
                if (i + 1 < rule.B.size()) cout << ", ";
            }

            cout << "] Support: " << rule.supp
                 << ", Confidence: " << rule.conf << endl;
        }
    }

    return rules;
}

string join_items(const vector<string>& items) {
    string result;

    for (size_t i = 0; i < items.size(); i++) {
        if (i > 0) {
            result += '\x1f';
        }

        result += items[i];
    }

    return result;
}

int main(int argc, char* argv[]) {
    string datapath = "C:\\Users\\pawma\\.cache\\kagglehub\\datasets\\mashlyn\\online-retail-ii-uci\\versions\\3\\online_retail_II.csv";
    double min_support = 0.4;
    double min_confidence = 0.7;
    bool verbose = false;

    if (argc >= 2) {
        datapath = argv[1];
    }

    if (argc >= 3) {
        min_support = atof(argv[2]);
    }

    if (argc >= 4) {
        min_confidence = atof(argv[3]);
    }

    if (argc >= 5) {
        string verbose_arg = argv[4];
        verbose = verbose_arg == "1" || verbose_arg == "true" || verbose_arg == "True";
    }

    vector<Rule> rules = solve(datapath, min_support, min_confidence, verbose);

    if (!verbose) {
        for (const Rule& rule : rules) {
            cout << "RULE\t"
                 << join_items(rule.A) << "\t"
                 << join_items(rule.B) << "\t"
                 << rule.supp << "\t"
                 << rule.conf << endl;
        }
    }

    return 0;
}
