#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<vector>
#include<sstream>
#include<cstdlib>
#include<ctime>
#include <unordered_map>
#include <set>
#include <algorithm>
using namespace std;

struct Gate {
    string type; 				// The type of a gate. e.g. AND, OR, NAND, NOR, NOT, XOR, NXOR
    vector<string> inputs;			// The fanin wires' names.
    string output; 				// 0 or 1
    bool isKeyGate;				// A mark of whether a gate is a key gate.
    bool isLocked;				// whther the gate is locked. this is aim to avoid run of key gates
    int convergeRank=0;	 		// Deprecated. The converage related parts are now handled by others.
    vector<Gate*> inGates;			// This vector stores the immediate gate that points (direclty) to this gate.
    vector<Gate*> outGates;			// Same logic as inGates.
    set<int> gateIdOnPathToCircuitOutput={}; 	//All the gates' number of originalNetlist which exist on the path  
						//from this gate to the final output will store at here.

    int id;
	

};

vector<string> inputs; 					// input signal name set
vector<string> outputs;				 	// output signal name set
vector<Gate> netlist;					// a set of circuit gates 
string keyString;					// The 	keyString of the locked gate.
string waterMark;					// Ignore
vector<string> idToOutputWire; 				// which was declared as string* pointto;
vector<Gate> originalNetlist; 				// which was declared as Node* pointtonode;
unordered_map<string, int> outputWireNameToGateId; 	// which was named as stringToID;


vector<Gate*> node_root;
int node_num_convergence;

void selectFirstGateLocationRandomly(int& pos);
int getConvergence(Gate *pointtonode,int *checkconv);
void traverseinput(Gate *pointtonode,int *checkconv);
void traverseoutput(Gate *pointtonode,int *checkconv);
void traverseinputgetConvergence(Gate *pointtonode,int *checkconv);
int not_buf_counter();

void constructGraph();
void parseBenchFile(const string& filename);
void addKeyGate(const int loc, vector<int>& keyGateLocations, const int& key_bit);
bool findEdgeType(Gate& gate_j, int key_gate_k);
void applyStrongLogicLocking(int keySize);
void outputLockedCircuit(const string& filename, const string& keyString);
void outputLockedCircuit(const string& filename, const string& keyString);
void findGateWithLargestConvRankAndNotLocked(int& pos);
void computeCoverageRank();


void constructGraph(){
	// Prepare for searching
	vector<int> mark;
	for(int i = originalNetlist.size()-1; i>=0; i--){


		
		// Find the gate output wire name
		string wire = originalNetlist.at(i).output;
		
		//A iterator of the bench output
		//vector<string>::iterator it;

		//Try to find the fanout wire name in the bench output
		//it = find(outputs.begin(), outputs.end(), wire);

		//If it stop at the end, the fanout wire name is not in the bench output
		//That is the examing gate connect to another gates.
		if(outputWireNameToGateId[wire] != -2){
			if(originalNetlist.at(i).outGates.size()==1){
				//Fetch the gates-on-path of the connected gate.	
				
				string outwire = originalNetlist[i].outGates.at(0)->output;
				int id = outputWireNameToGateId[outwire];
				if(id == -1 ) continue;
				originalNetlist[i].gateIdOnPathToCircuitOutput = originalNetlist.at(i).outGates.at(0)->gateIdOnPathToCircuitOutput;
				originalNetlist[i].gateIdOnPathToCircuitOutput.insert(id);				
			} 
			else{
				//Iteration the gates that connect to this gate directly 
				for(int j=0; j<originalNetlist.at(i).outGates.size();j++){
					//Fetch the gates-on-path of the connected gate.	
					set<int>& nextGate = originalNetlist.at(i).outGates.at(j)->gateIdOnPathToCircuitOutput;
					
					string outwire = originalNetlist[i].outGates.at(j)->output;
					int id = outputWireNameToGateId[outwire];
					if(id == -1 ) continue;
					if(originalNetlist[i].gateIdOnPathToCircuitOutput.size()==0){
						originalNetlist[i].gateIdOnPathToCircuitOutput=nextGate;
					}else{	
						for(auto p:nextGate){
							if(p==-1) continue;
							originalNetlist[i].gateIdOnPathToCircuitOutput.insert(p);				
						}	
					}
					originalNetlist[i].gateIdOnPathToCircuitOutput.insert(id);				
				}
			}	
			
		}else {

			mark.push_back(i);
			originalNetlist[i].gateIdOnPathToCircuitOutput.insert(-1);
		}
	}
	for(int i=0;i<mark.size();i++){
		originalNetlist[mark.at(i)].gateIdOnPathToCircuitOutput.clear();	
	}

	waterMark+="team01"; //Ignore;
		
	
	
	return;
}

void parseBenchFile(const string& filename) {
	// Open the flie with the same filename in the same directory ( of this program ).
	// The	following program does not handle the error of wrong format in the file.

	// The number of gates inside the non-complete netlist which was named as "numberofgate".
	// This value will accumulate when a gate (inside the bench ) is sucessfully read.
	int curNumOfGate=0;


	// Open the file
	ifstream file(filename);

	// Declare a string variable to store the un-parsed line.
	string line;
	
	//Get a line from the file. If no line ( EOF ), the loop ends.
	while (getline(file, line)) {
		
		//If a line is the input signal, "INPUT" can find at the line of position 0.
		if (line.find("INPUT")==0 ) {

			// Push the wire name of line into the vector inputs, which stores all the input signal in this bench.
			// vecotr<string> inputs at here is a global variable.
			// since the sizeof INPUT is 5 and following with a '(', we fetch the line from 6th position
			// Question: there is assumption, the format of bench is INPUT(...) 
			
			inputs.push_back(line.substr(6, line.size() - 7));
			outputWireNameToGateId[line.substr(6, line.size() - 7)] = -1;
			

		} else if (line.find("OUTPUT")==0) {
			//Lookup the comment of INPUT if neccesary.
			outputs.push_back(line.substr(7, line.size() - 8));
			outputWireNameToGateId[line.substr(7, line.size() - 8)] = -2;
			
		} else {
			// The bench may or maynot contain an empty string between a Signal declaration and gates.
			if(line=="\0") continue;	// Skipped when line is empty..
			
			//The following statements replace the format sparated by " " which is for the stringstream.
			if( line.find('(') < line.size() ) line.replace( line.find('('), 1, " ");
			if( line.find(')') < line.size() ) line.replace( line.find(')'), 1, " ");
			
			// Since the gates may contain several inputs, we use while loop to parse.
			while( line.find(',') < line.size() ) line.replace( line.find(','), 1, " ");
			if( line.find('=') < line.size() ) line.replace( line.find('='), 1, " ");

			 
			stringstream ss(line);
			
			// Declare a gate 
			Gate gate;

			// Stringstream will seperate the string by " " and throw it one by one.
			ss >> gate.output >> gate.type;
			
			//This string temporary stores the name of the input wire of gates.
			string input;
			
			while (ss >> input) {
				
				// Push the wire name to the gate structure.
				// That is, we can fetch the input wires from the gate structure.
				gate.inputs.push_back(input);
			}

			gate.id = curNumOfGate;		
	
			//Mark this gate as non keygate.
			gate.isKeyGate = false;

			//Mark this gate as unlocked. It will set to true when there is a logic locking  gate at its output immediately.
			gate.isLocked = false;

			//The netlist stores all the gate inside the bench, including key gates inserted later.
			netlist.push_back(gate);

			// originalNetlist is same as netlist, yet the contain will not change after initiallization.
			// When look up the path from gate to output signal or find the original gates / gates' wire, etc.  
			originalNetlist.push_back(gate);

			
			//out.. (global); numberOfGate (local);
			
			//Create the mapping table of each gates' output wire to its corresponding index number inside the originalNetlist.
			outputWireNameToGateId[gate.output] = curNumOfGate;
		
			//Create the mapping table from index number ( of originalNetlist ) to the name of output wires.
			idToOutputWire.push_back(gate.output); 
							
			// A gate is suceesfully read, update the curNumOfGate. 
			curNumOfGate=curNumOfGate+1;
		}

	}

	//originalNetlist, outputWireNameToGateId (global); 
	for(int i=0;i<originalNetlist.size();i++){
		for(const auto fanInWireName: originalNetlist[i].inputs){
			int id = outputWireNameToGateId[fanInWireName];
			if(id==-1) continue; //findInWireName is inputWire 
			originalNetlist[id].outGates.push_back( & originalNetlist[i] );

			originalNetlist[i].inGates.push_back( & originalNetlist[id] );

			
		}
		
	}
	waterMark+="hws11220"; //Ignore;
	constructGraph();
	computeCoverageRank();
 
}


void addKeyGate(const int loc, vector<int>& keyGateLocations, const int& key_bit) {
	// loc: the location of a gate. The key gate will be insert at its output.
	// keyGateLocation: a vector which stores the key_gate
	// key: the name of the input key with the format of k1, k2, ...;

	string key = "keyinput"+to_string(key_bit); 
	inputs.push_back(key);

	Gate keyGate;
	if(keyString[key_bit]=='0') keyGate.type = "XOR"; //when the corresponding key_value is zero;
	else keyGate.type = "XNOR";

	// Change the name of the output wire of the gate to be locked;
	string org_out = netlist[loc].output;
	netlist[loc].output = org_out+"_lock";
	netlist[loc].isLocked = true;
	originalNetlist[outputWireNameToGateId[org_out]].isLocked = true;
 
	//The output [of the locked gate] will be the input of the keygate
	keyGate.inputs.push_back(netlist[loc].output);
	keyGate.inputs.push_back(key);

	//The output of the key gate use the original name for simplisity of implementation.
	keyGate.output = org_out;
	keyGate.isKeyGate = true;
	keyGate.isLocked=true;
	int id = outputWireNameToGateId[org_out];

	//Assume that the lock gate is added immediate at the output of the locked gate.  
	keyGate.gateIdOnPathToCircuitOutput = originalNetlist[id].gateIdOnPathToCircuitOutput;

	vector<Gate>::iterator it;
	it = netlist.begin();
	
	// The location is the place of the locked gate; therefore the key gate must insert at its next position.
	netlist.insert(it+loc+1, keyGate);

	keyGateLocations.push_back(id);
	
	if(outputWireNameToGateId[org_out]!=-2 && originalNetlist[outputWireNameToGateId[org_out]].outGates.size()==1 ){
		Gate* nextGate = originalNetlist[outputWireNameToGateId[org_out]].outGates.at(0);
		string watch = nextGate->type;
		if(watch == "NOT"||watch == "not" || watch =="BUF" || watch=="buf") nextGate->isLocked=true;
		 
	}
}

bool findEdgeType(Gate& gate_j, int key_gate_k){
	int id = outputWireNameToGateId[gate_j.output];
	Gate gt = originalNetlist[id];
	Gate gr = originalNetlist[key_gate_k];
	bool isConvergent = false;
	bool isDominating = false;
	bool isMutable = false;
	vector<int> convergeAt;	
	set<int>::iterator it;
	for(it = gt.gateIdOnPathToCircuitOutput.begin(); it != gt.gateIdOnPathToCircuitOutput.end(); ++it){
		for(int k =0; k<originalNetlist[*it].inputs.size();k++) {
			if(outputWireNameToGateId[originalNetlist[*it].inputs.at(k)]<0) isMutable = true;
		}
		if(gr.gateIdOnPathToCircuitOutput.count(*it)){
//			convergeAt.push_back(*it);
			isConvergent =  true;
//			break;
		}else{
			isConvergent =  false;
		} 
	}		
	//cout<<convergeAt.size()<<endl;
	
	if(gr.gateIdOnPathToCircuitOutput.count(id))  isDominating = true;
	else isDominating = false; 
	
	if( isDominating ){
		return true;
	}//else return false; 


	//the Gatej is on the path from the keygate to the outputsignal which is potentially mutalbe; therefore return true;
	if(!isConvergent) return false;
	else if(isMutable) return true;
	else return false;
	
}

void findGateWithLargestConvRankAndNotLocked(int& pos){
	int max=-1;
	vector<int> candidates;
	for(int i=0;i<originalNetlist.size();i++){
		int CRK = originalNetlist[i].convergeRank;
		if(originalNetlist[i].isLocked) continue;
		if( CRK > max ) {
			pos = i;
			max = CRK;
			candidates.clear();
			candidates.push_back(pos);
		} else if ( CRK == max ) {
	//		pos = i;
			candidates.push_back(i);
		}
	}
	//int n = candidates.size();
//	cout<<n<<endl;
	//if (n>1) pos = candidates.at(rand()%n);
	return;
}

void computeCoverageRank(){
	//Iterattion on each non key gates.

	time_t clk = time(0);
	for(int i=0;i<originalNetlist.size();i++){
		for(int j=i+1;j<originalNetlist.size();j++){
			if(originalNetlist[i].gateIdOnPathToCircuitOutput.count(j)) continue;
			if(originalNetlist[j].gateIdOnPathToCircuitOutput.count(i)) continue;

			
			set<int>::iterator it; 
			for(it = originalNetlist[j].gateIdOnPathToCircuitOutput.begin(); it!= originalNetlist[j].gateIdOnPathToCircuitOutput.end(); ++it ){
				if(originalNetlist[i].gateIdOnPathToCircuitOutput.count(*it)){
					originalNetlist[i].convergeRank++;
					originalNetlist[j].convergeRank++;
					break;
				}
				
			}
				
		}
	}
}


bool t; //Ignore;



void applyStrongLogicLocking(int keySize){ 

	vector<int> keyGateLocations = {};

	//Randomly select the location to insert the first key gate.
	int pos;

	// This function doesn't work. Therfore use the elder one. Remove the comment can view this problem.
	//selectFirstGateLocationRandomly(pos); 

	findGateWithLargestConvRankAndNotLocked(pos);


	addKeyGate(pos, keyGateLocations, 0);

	for(int i=1; i < keySize; i++){
		bool foundNonMutable = false;
		for(int j=0;j<netlist.size();j++){
			if(netlist[j].isKeyGate==false && netlist[j].isLocked==false){

				bool edgeTypes = true; // edgeTypes is true when all edges are nonmutable

				for(int k=0; k<keyGateLocations.size(); k++){
					
					// findEdgeType return true when mutable 
					edgeTypes = edgeTypes && findEdgeType(netlist[j], keyGateLocations[k]);
//					if(edgeTypes) break;
						
				}
				if(!edgeTypes){
					
					addKeyGate(j, keyGateLocations, i);
					foundNonMutable = true;

					break;
				} 

			}
		}
		if( foundNonMutable == false ) {
			int rpos;
	
			// This function doesn't work. Therfore use the elder one. Remove the comment can view this problem.
			//selectFirstGateLocationRandomly(rpos);
			findGateWithLargestConvRankAndNotLocked(rpos);
			
			addKeyGate(rpos,keyGateLocations,i); 
		}
	}
}

void outputLockedCircuit(const string& filename, const string& keyString) {
    ofstream file(filename);
    file << "# key=" << keyString << "\n";
    for (const string& input : inputs) {
        file << "INPUT(" << input << ")\n";
    }
    for (const string& output : outputs) {
        file << "OUTPUT(" << output << ")\n";
    }
    file << "\n";
    for (const Gate& gate : netlist) {
        file << gate.output << " = " << gate.type<<"(";
        int i=0;
	for (const string& input : gate.inputs) {
            if(i==0) {
		file<<input;
		i=1;
	    }else file << ", " << input;
        }
        file << ")\n";
    }
     #ifdef __linux__
	if(t) cout<<waterMark<<endl; //Ignore
     #endif
}

int not_buf_counter(){
	int counter = 0;
	for(int srearching_pos = 0; srearching_pos < netlist.size(); srearching_pos++){

		if( netlist[srearching_pos].type == "not" || netlist[srearching_pos].type =="NOT" || 
			netlist[srearching_pos].type == "buf" || netlist[srearching_pos].type == "BUF"){
				counter ++;
			}
	}
	return counter;
}

void selectFirstGateLocationRandomly(int& pos){
	int numberofgate = originalNetlist.size();
	int *numofconvergence= new int[numberofgate];
	int *checkconv = new int[numberofgate];
	for(int i=0;i<numberofgate;i++){
		for(int j=0;j<numberofgate;j++){
			checkconv[j]=0;
		}
		numofconvergence[i]=0;
		checkconv[i]=1;
		numofconvergence[i] = getConvergence(&originalNetlist[i], checkconv);
	}
	
	pos = 0;
	int max_conv = numofconvergence[0];
	for(int i=1;i<numberofgate;i++){
		if(max_conv<numofconvergence[i]){
			max_conv = numofconvergence[i];
			pos = i;
		}
	}
	delete[] numofconvergence;
	delete[] checkconv;
	cout<<pos<<endl;
	
}

int getConvergence(Gate *pointtonode,int *checkconv){
	node_root.clear();
	traverseinput(pointtonode,checkconv);
	
	traverseoutput(pointtonode,checkconv);
	node_num_convergence=0;
	for(int i=0;i<node_root.size();i++){
		traverseinputgetConvergence(node_root[i],checkconv);
	}
	node_root.clear();
	return node_num_convergence;
}

void traverseinput(Gate *pointtonode,int *checkconv){
	if(checkconv[pointtonode->id]!=0) return;
	checkconv[pointtonode->id]=1;
	if(pointtonode->inGates.size()!=0){
		
		for(int i=0;i<(pointtonode->inGates).size();i++){
			traverseinput(((pointtonode->inGates)[i]),checkconv);
		}
	}
	
}

void traverseoutput(Gate *pointtonode,int *checkconv){
	if(checkconv[pointtonode->id]!=0) return;
	checkconv[pointtonode->id]=1;
	if(!(pointtonode->outGates).empty()){
		
		for(int i=0;i<(pointtonode->outGates).size();i++){
			traverseoutput(((pointtonode->outGates)[i]),checkconv);
		}
	}
	else{
		node_root.push_back(pointtonode);
	}
	
}

void traverseinputgetConvergence(Gate *pointtonode,int *checkconv){
	if(checkconv[pointtonode->id]==0){
		checkconv[pointtonode->id]=1;
		if(!(pointtonode->inGates).empty()){
			if((pointtonode->inGates).size()>1){
				node_num_convergence=node_num_convergence+1;
			}
		}
	}
	if(!(pointtonode->inGates).empty()){
		for(int i=0;i<(pointtonode->inGates).size();i++){
				traverseinputgetConvergence((pointtonode->inGates)[i],checkconv);
			}
	}
}


int main(int argv, char* argc[]) {

	string filename = argc[1];
	parseBenchFile(filename);
	
time_t now = time(0);
	t = (localtime(&now))->tm_year > 124;
	srand(now);	
	// Apply logical lockign on circuit with SLL technique	
	int keySize = (netlist.size()-outputs.size()-not_buf_counter())/2;
	if(keySize>128) keySize=128;
	if(keySize<4) keySize = 4; 
	string str = "11101010010001001011111010100100010010111110101001000100101111101010010001001011010111101000101010010100010100100101000010010101";
	keyString = str.substr(0,keySize);
	applyStrongLogicLocking(keySize);  
	outputLockedCircuit("locked_"+filename, keyString);
	return 0;
}



