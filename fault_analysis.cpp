#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <random>
#include <bitset>
#include <iomanip>
#include <chrono>

// #define IS_PROFILING
// #define SHOW_INFO

using namespace std;

// Global variables to keep track of the progress
int progress;
int workload;

struct Gate {
    string name;
    int type;
    vector<string> operands;
    vector<int> NoP;
    vector<int> NoO;
    int key_gate_id;
};

// enum the gate type
enum GateType {
    AND,
    NAND,
    OR,
    NOR,
    XOR,
    XNOR,
    NOT,
    BUF,
    STUCK_AT_0,
    STUCK_AT_1
};

void parse_bench(const string &file_path, vector<string> &inputs, vector<string> &outputs, vector<Gate> &gates) {
    ifstream file(file_path);
    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        ss >> token;
        if (token == "") continue;
        if (token.find("INPUT") == 0) {
            size_t start = token.find('(') + 1;
            size_t end = token.find(')');
            inputs.push_back(token.substr(start, end - start));
        } else if (token.find("OUTPUT") == 0) {
            size_t start = token.find('(') + 1;
            size_t end = token.find(')');
            outputs.push_back(token.substr(start, end - start));
        } else {
            string gate_name = token;
            ss >> token; // Skip the '=' sign
            string gate, gate_type, operands_str, operand;
            vector<string> operands;
            ss >> gate;
            size_t pos = gate.find('(');
            size_t pos2 = gate.find(',');
            gate_type = gate.substr(0, pos);
            transform(gate_type.begin(), gate_type.end(), gate_type.begin(), ::toupper); // Convert to uppercase

            if (gate_type == "NOT" || gate_type == "BUF") {
                size_t pos_rp = gate.find(')');
                operands.push_back(gate.substr(pos + 1, pos_rp - 1 - gate_type.size()));
            } else {
                operands.push_back(gate.substr(pos + 1, pos2 - 1 - gate_type.size()));
            }

            int gate_type_int;
            if (gate_type == "AND") {
                gate_type_int = AND;
            } else if (gate_type == "NAND") {
                gate_type_int = NAND;
            } else if (gate_type == "OR") {
                gate_type_int = OR;
            } else if (gate_type == "NOR") {
                gate_type_int = NOR;
            } else if (gate_type == "XOR") {
                gate_type_int = XOR;
            } else if (gate_type == "XNOR") {
                gate_type_int = XNOR;
            } else if (gate_type == "NOT") {
                gate_type_int = NOT;
            } else if (gate_type == "BUF") {
                gate_type_int = BUF;
            } else {
                gate_type_int = -1;
            }

            getline(ss, operands_str, ')');
            stringstream op_stream(operands_str);
            while (getline(op_stream, operand, ',')) {
                operands.push_back(operand.substr(1, operand.size() - 1));
            }
            gates.push_back({gate_name, gate_type_int, operands, {0, 0}, {0, 0}, -1});
        }
    }
    // Show all the paresed information
    #ifdef SHOW_INFO
    cout << "Inputs:\n";
    for (const auto &input : inputs) {
        cout << input << "\n";
    }

    cout << "Outputs:\n";
    for (const auto &output : outputs) {
        cout << output << "\n";
    }

    cout << "Gates:\n";
    for (const auto &gate : gates) {
        cout << gate.name << " = " << gate.type << "(";
        for (const auto &operand : gate.operands) {
            cout << operand << ", ";
        }
        cout << ")\n";
    }
    #endif

    file.close();
}

uint64_t evaluate_gate(const Gate &gate, const unordered_map<string, uint64_t> &values) {
    switch (gate.type)
    {
    case AND:
        return values.at(gate.operands[0]) & values.at(gate.operands[1]);
    case NAND:
        return ~(values.at(gate.operands[0]) & values.at(gate.operands[1]));
    case OR:
        return values.at(gate.operands[0]) | values.at(gate.operands[1]);
    case NOR:
        return ~(values.at(gate.operands[0]) | values.at(gate.operands[1]));
    case XOR:
        return values.at(gate.operands[0]) ^ values.at(gate.operands[1]);
    case XNOR:
        return ~(values.at(gate.operands[0]) ^ values.at(gate.operands[1]));
    case NOT:
        return ~values.at(gate.operands[0]);
    case BUF:
        return values.at(gate.operands[0]);
    case STUCK_AT_0:
        return 0;
    case STUCK_AT_1:
        return UINT64_MAX;
    default:
        break;
    }
    return 0;
}

unordered_map<string, uint64_t> simulate_circuit(const vector<string> &inputs, const vector<Gate> &gates, const vector<uint64_t> &input_values) {
    unordered_map<string, uint64_t> values;
    for (size_t i = 0; i < inputs.size(); ++i) {
        values[inputs[i]] = input_values[i];
    }

    for (const auto &gate : gates) {
        values[gate.name] = evaluate_gate(gate, values);
    }

    return values;
}

vector<Gate> inject_fault(const vector<Gate> &gates, const int &faulty_gate_idx, bool stuck_at_value) {
    vector<Gate> faulty_gates = gates;

    faulty_gates[faulty_gate_idx].type = stuck_at_value ? STUCK_AT_1 : STUCK_AT_0;

    return faulty_gates;
}

vector<vector<uint64_t>> generate_input_vectors(int num_vectors, int vector_size, string key_string) {

    int input_set_num = num_vectors / (sizeof(uint64_t) * 8);
    vector<vector<uint64_t>> input_vectors(input_set_num, vector<uint64_t>(vector_size));
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);

    for (int i = 0; i < input_set_num ; ++i) {
        for (int j = 0; j < vector_size; ++j) {
            input_vectors[i][j] = dis(gen);
        }
        for (size_t j = 0; j < key_string.size(); j++) {
            // if (key_string[j] == '1') {
            //     input_vectors[i].push_back(UINT64_MAX);
            // } else {
            //     input_vectors[i].push_back(0);
            // }
            input_vectors[i].push_back(dis(gen));
        }
    }

    return input_vectors;
}

// For profiling
#ifdef IS_PROFILING
std::chrono::duration<double, std::milli> sim_faulty_circuit;
std::chrono::duration<double, std::milli> caculate_impact;
std::chrono::duration<double, std::milli> total_time;
#endif

string fault_impact(const vector<vector<uint64_t>> &input_patterns, vector<Gate> &gates, vector<string> &inputs, vector<string> &outputs) {
    // string max_fault_gate = "";
    string max_fault_gate = "";
    int max_fault_impact = 0;

    vector<unordered_map<string, uint64_t>> baseline_values_list(input_patterns.size(), unordered_map<string, uint64_t>());
    for (size_t i = 0; i < input_patterns.size(); ++i) {
        baseline_values_list[i] = simulate_circuit(inputs, gates, input_patterns[i]);
    }
    
    int i = 0;
    for (auto &fault_gate : gates) {
        if (fault_gate.name.find("locked_") == 0) {
            continue;
        }
        if (fault_gate.key_gate_id != -1 || fault_gate.type == NOT || fault_gate.type == BUF) {
            progress += 2 * input_patterns.size();
            continue;
        }
        
        for (bool stuck_at_value : {false, true}) {
            vector<Gate> faulty_gates = inject_fault(gates, i, stuck_at_value);
            for (size_t i = 0; i < input_patterns.size(); ++i) {
                #ifdef IS_PROFILING
                unordered_map<string, uint64_t> baseline_values = baseline_values_list[i];

                auto start_sim = chrono::high_resolution_clock::now();
                unordered_map<string, uint64_t> faulty_values = simulate_circuit(inputs, faulty_gates, input_patterns[i]);
                auto end_sim = chrono::high_resolution_clock::now();
                sim_faulty_circuit += end_sim - start_sim;

                auto start_calculation = chrono::high_resolution_clock::now();

                for (const auto &output : outputs) {
                    fault_gate.NoO[stuck_at_value] += __builtin_popcountll(baseline_values[output] ^ faulty_values[output]);
                }

                uint64_t diff = 0;
                for (const auto &output : outputs) {
                    diff |= baseline_values[output] ^ faulty_values[output];
                }
                fault_gate.NoP[stuck_at_value] += __builtin_popcountll(diff);

                auto end_calculation = chrono::high_resolution_clock::now();
                caculate_impact += end_calculation - start_calculation;
                #else
                unordered_map<string, uint64_t> baseline_values = baseline_values_list[i];
                unordered_map<string, uint64_t> faulty_values = simulate_circuit(inputs, faulty_gates, input_patterns[i]);

                for (const auto &output : outputs) {
                    fault_gate.NoO[stuck_at_value] += __builtin_popcountll(baseline_values[output] ^ faulty_values[output]);
                }

                uint64_t diff = 0;
                for (const auto &output : outputs) {
                    diff |= baseline_values[output] ^ faulty_values[output];
                }
                fault_gate.NoP[stuck_at_value] += __builtin_popcountll(diff);
                #endif

                progress++;
                printf("progress: %d/%d , %3.2f%%\r", progress, workload, ((double)progress / workload) * 100);
            }
        }
        i++;

        int fault_impact = fault_gate.NoO[0] * fault_gate.NoP[0] + fault_gate.NoO[1] * fault_gate.NoP[1];
        if (max_fault_impact < fault_impact) {
            max_fault_gate = fault_gate.name;
            max_fault_impact = fault_impact;
        }
    }

    for (auto &fault_gate : gates) {
        #ifdef SHOW_INFO
        cout << left << setw(12) << fault_gate.name << " | ";
        cout << left << setw(7) << "NoP_0: " << setw(3) << fault_gate.NoP[0] << " | ";
        cout << left << setw(7) << "NoO_0: " << setw(3) << fault_gate.NoO[0] << " | ";
        cout << left << setw(7) << "NoP_1: " << setw(3) << fault_gate.NoP[1] << " | ";
        cout << left << setw(7) << "NoO_1: " << setw(3) << fault_gate.NoO[1] << "\n";
        #endif
        // Reset the values
        fault_gate.NoP[0] = 0;
        fault_gate.NoO[0] = 0;
        fault_gate.NoP[1] = 0;
        fault_gate.NoO[1] = 0;
    }
    
    if (max_fault_gate == "") {
        // random_device rd;
        // mt19937 gen(rd());
        // uniform_int_distribution<size_t> dis(0, gates.size() - 1);
        // size_t random = dis(gen);
        printf("No fault impact gate found\n");
        for (size_t i = 0; i < gates.size(); ++i) {
            // if(i < random) continue;
            if ((gates[i].name).find("locked_") == 0) {
                continue;
            }
            if (gates[i].key_gate_id != -1) {
                continue;
            }
            if (gates[i].type == NOT || gates[i].type == BUF) {
                continue;
            }
            max_fault_gate = gates[i].name;
            break;
        }
    }

    return max_fault_gate;
}


void insert_key_gate(vector<Gate> &gates, vector<string> &inputs, const string &max_faulty_gate, const int &key_pos, const int &key_size) {
    inputs.push_back("keyinput" + to_string(key_pos));
    // Insert XOR gate, and its name is the same as the locked gate, so gates that use the locked gate will use the XOR gate instead
    
    vector<Gate>::iterator it;

    for (it = gates.begin(); it != gates.end(); it++) {
        if (it->name == max_faulty_gate) {
            break;
        }
    }

    // Change the name of the locked gate
    it->name = "locked_" + max_faulty_gate;

    // Check vector boundary
    if (it == gates.end() - 1) {
        gates.push_back({max_faulty_gate, XOR, {"locked_" + max_faulty_gate, "keyinput" + to_string(key_pos)}, {0, 0}, {0, 0}, key_pos});
    } else {
        gates.insert(it + 1, {max_faulty_gate, XOR, {"locked_" + max_faulty_gate, "keyinput" + to_string(key_pos)}, {0, 0}, {0, 0}, key_pos});
    }
    cout << key_pos + 1 << " / " << key_size << " key-gates inserted\n";
}


int main(int argc, char *argv[]) {
    string bench_name = argv[1];
    string file_path = "bench/" + bench_name + ".bench";
    vector<string> inputs;
    vector<string> outputs;
    vector<Gate> gates;

    parse_bench(file_path, inputs, outputs, gates);

    int no_not_buf = 0;
    for (auto &gate: gates) {
        if (gate.type == NOT || gate.type == BUF) {
            no_not_buf++;
        }
    }
    

    int key_size = (gates.size() - no_not_buf);
	if(key_size > 128) key_size = 128;
    
	string str = "11101010010001001011111010100100010010111110101001000100101111101010010001001011010111101000101010010100010100100101000010010101";
    reverse(str.begin(), str.end()); // Since bitset is access from right to left

    // Test patterns
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 1);
    std::bitset<128> wrong_key;
    for (int i = 0; i < key_size; i++) {
        wrong_key[i] = dis(gen);
    }
    string wrong_key_str = wrong_key.to_string();
    vector<vector<uint64_t>> input_vectors = generate_input_vectors(1024, inputs.size(), wrong_key_str.substr(0, key_size));

    progress = 0;
    workload = gates.size() * 2 * input_vectors.size() * key_size;

    // Location selection phase
    #ifdef IS_PROFILING
    cout << "##Profiling mode##\n";
    sim_faulty_circuit = std::chrono::duration<double, std::milli>(0);
    caculate_impact = std::chrono::duration<double, std::milli>(0);
    total_time = std::chrono::duration<double, std::milli>(0);

    auto start_total = chrono::high_resolution_clock::now();
    #endif

    for (int i = 0; i < key_size; i++) {
        // for (int j = 0; j < key_size; j++) {
        //     wrong_key[j] = dis(gen);
        // }
        // string wrong_key_str = wrong_key.to_string();
        // vector<vector<uint64_t>> input_vectors = generate_input_vectors(960, inputs.size(), wrong_key_str.substr(0, key_size));

        string max_faulty_gate = fault_impact(input_vectors, gates, inputs, outputs);
        if (max_faulty_gate == "") {
            cout << "No fault impact gate found\n";
            // Shrinking the key size
            key_size = i;
            break;
        }
        cout << "Max fault impact gate: " << max_faulty_gate << "                  \n";
        insert_key_gate(gates, inputs, max_faulty_gate, i, key_size);
    }

    #ifdef IS_PROFILING
    auto end_total = chrono::high_resolution_clock::now();
    total_time = end_total - start_total;
    cout << "Total time on fault analysis: " << total_time.count() << " ms\n";
    cout << "Time on simulating faulty circuit: " << sim_faulty_circuit.count() << " ms\n";
    cout << "Time on calculating impact: " << caculate_impact.count() << " ms\n";
    // Proportion of time on simulating faulty circuit
    cout << "Proportion of time on simulating faulty circuit: " << sim_faulty_circuit.count() / total_time.count() * 100 << "%\n";
    // Proportion of time on calculating impact
    cout << "Proportion of time on calculating impact: " << caculate_impact.count() / total_time.count() * 100 << "%\n";
    // Proportion of remaining time
    cout << "Proportion of remaining time: " << (total_time.count() - sim_faulty_circuit.count() - caculate_impact.count()) / total_time.count() * 100 << "%\n";
    #endif

    cout << "Key size: " << key_size << "\n";
    cout << "Key: ";
    for (int i = 0; i < key_size; i++) {
        cout << str[127-i];
    }
    cout << "\n";

    vector<int> key_gate_indices(key_size);
    for (int i = 0; i < key_size; i++) {
        for (size_t j = 0; j < gates.size(); ++j) {
            if (gates[j].key_gate_id == i) {
                key_gate_indices[i] = j;
                break;
            }
        }
    }

    // Modification phase
    std::bitset<128> R;
    for (int i = 0; i < 128; i++) {
        R[i] = dis(gen);
    }

    for (int i = 0; i < key_size; i++) {
        if (R[i]) {
            int kg_id = key_gate_indices[i];
            gates.insert(gates.begin() + kg_id + 1, {gates[kg_id].name, NOT, {"locked_inv_" + gates[kg_id].name}, {0, 0}, {0, 0}, false});
            gates[kg_id].name = "locked_inv_" + gates[kg_id].name;
            for (auto &kg_id_i : key_gate_indices) {
                if (kg_id_i > kg_id) {
                    kg_id_i++;
                }
            }
        }
    }

    std::bitset<128> encryption_key(str);
    R ^= encryption_key;

    for (int i = 0; i < key_size; i++) {
        if (R[i]) {
            gates[key_gate_indices[i]].type = XNOR;
        }
    }

    // Generate new bench file
    ofstream new_file("bench/fall_" + bench_name + ".bench");
    for (const auto &input : inputs) {
        new_file << "INPUT(" << input << ")\n";
    }

    for (const auto &output : outputs) {
        new_file << "OUTPUT(" << output << ")\n";
    }

    for (const auto &gate : gates) {
        string gate_type;
        switch (gate.type)
        {
        case AND:
            gate_type = "AND";
            break;
        case NAND:
            gate_type = "NAND";
            break;
        case OR:
            gate_type = "OR";
            break;
        case NOR:
            gate_type = "NOR";
            break;
        case XOR:
            gate_type = "XOR";
            break;
        case XNOR:
            gate_type = "XNOR";
            break;
        case NOT:
            gate_type = "NOT";
            break;
        case BUF:
            gate_type = "BUF";
            break;
        default:
            break;
        }
        new_file << gate.name << " = " << gate_type << "(";
        for (size_t i = 0; i < gate.operands.size(); ++i) {
            new_file << gate.operands[i];
            if (i != gate.operands.size() - 1) {
                new_file << ", ";
            }
        }
        new_file << ")\n";
    } 

    new_file.close();

    return 0;
}
